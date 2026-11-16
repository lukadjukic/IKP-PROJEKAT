#include "server_helper.h"
#define N 1000000
bool primes[N];

typedef struct arguments_t {
    queue_t *        input;
    queue_t *        output;
    ssize_t          num;
    coordination_t * coordination;
} arguments_t;

#pragma region mthreads

typedef struct {
    pthread_t thread;
    bool      initialized;
} mthread_t;

void mthread_init(mthread_t * thread) {
    thread->initialized = false;
}

bool mthread_create(mthread_t * thread, void * routine(void *), void * arg) {
    if (pthread_create(&thread->thread, NULL, routine, arg)) {
        perror("[INIT] Thread creation failed.\n");
        thread->initialized = false;
        return false;
    }
    thread->initialized = true;
    return true;
}

void mthread_join(mthread_t * thread) {
    if (thread->initialized == false)
        return;
    pthread_join(thread->thread, NULL);
    thread->initialized = false;
}

#pragma endregion

#pragma region wrappers

void * w_reciever(void * arg) {
    struct arguments_t * arguments = (struct arguments_t *)arg;
    printf("[DEBUG] Reciever thread started.\n");
    reciever(arguments->output, arguments->coordination, (int)arguments->num);
    printf("[DEBUG] Reciever thread finished.\n");
    return NULL;
}

void * w_load_balancer(void * arg) {
    struct arguments_t * arguments = (struct arguments_t *)arg;
    printf("[DEBUG] Load balancer thread started.\n");
    load_balancer(arguments->input, arguments->output);
    printf("[DEBUG] Load balancer thread finished.\n");
    return NULL;
}

void * w_worker(void * arg) {
    struct arguments_t * arguments = (struct arguments_t *)arg;
    int                  i         = (int)arguments->num;
    printf("[DEBUG] Worker id: %d started.\n", i);
    worker((arguments->input) + i, arguments->output, i, primes, N - 1);
    printf("[DEBUG] Worker id: %d finished.\n", i);
    return NULL;
}

void * w_sender(void * arg) {
    struct arguments_t * arguments = (struct arguments_t *)arg;
    printf("[DEBUG] Sender thread started.\n");
    sender(arguments->input, (int)arguments->num);
    printf("[DEBUG] Sender thread finished.\n");
    return NULL;
}

void * w_admin(void * arg) {
    arguments_t * arguments = (arguments_t *)arg;
    printf("[DEBUG] Admin thread started.\n");
    admin(arguments->input, arguments->num, arguments->coordination);
    printf("[DEBUG] Admin thread finished.\n");
    return NULL;
}

#pragma endregion

void join_threads(mthread_t * reciever, mthread_t * load_balancer, mthread_t * sender, mthread_t * admin, mthread_t * workers, size_t worker_count) {
    mthread_join(reciever);
    mthread_join(load_balancer);
    mthread_join(sender);
    mthread_join(admin);

    for (size_t i = 0; i < worker_count; ++i) {
        mthread_join(&workers[i]);
    }
}

void free_resources(coordination_t * coordinate, queue_t * reciever_queue, queue_t * sender_queue, queue_t * worker_queues, size_t worker_count, int main_socket) {
    delete_coordination(coordinate);
    delete_queue(reciever_queue);
    delete_queue(sender_queue);
    for (size_t i = 0; i < worker_count; ++i) {
        delete_queue(worker_queues + i);
    }

    shutdown(main_socket, SHUT_RDWR);
    close(main_socket);
}

void init_args(arguments_t * reciever_args, arguments_t * load_balancer_args, arguments_t * worker_args, arguments_t * sender_args, arguments_t * admin_args, size_t worker_count, int main_socket, coordination_t * coordination, queue_t * reciever_queue, queue_t * sender_queue, queue_t * worker_queues) {
    arguments_t r_args = {NULL, reciever_queue, main_socket, coordination};
    arguments_t l_args = {reciever_queue, worker_queues, 0, NULL};
    arguments_t s_args = {sender_queue, NULL, main_socket, NULL};
    arguments_t a_args = {worker_queues, NULL, (ssize_t)worker_count, coordination};

    memcpy(reciever_args, &r_args, sizeof(arguments_t));
    memcpy(load_balancer_args, &l_args, sizeof(arguments_t));
    memcpy(sender_args, &s_args, sizeof(arguments_t));
    memcpy(admin_args, &a_args, sizeof(arguments_t));

    for (size_t i = 0; i < worker_count; i++) {
        worker_args[i].input        = worker_queues;
        worker_args[i].output       = sender_queue;
        worker_args[i].coordination = NULL;
        worker_args[i].num          = (ssize_t)i;
    }
}

bool spawn_threads(mthread_t * reciever_thread, mthread_t * load_balancer_thread, mthread_t * worker_threads, mthread_t * sender_thread, mthread_t * admin_thread, arguments_t * reciever_args, arguments_t * load_balancer_args, arguments_t * worker_args, arguments_t * sender_args, arguments_t * admin_args, size_t worker_count) {
    mthread_init(reciever_thread);
    mthread_init(load_balancer_thread);
    mthread_init(sender_thread);
    mthread_init(admin_thread);

    for (size_t i = 0; i < worker_count; i++) {
        mthread_init(&worker_threads[i]);
    }

    if (!mthread_create(reciever_thread, w_reciever, (void *)reciever_args)) {
        return false;
    }

    if (!mthread_create(load_balancer_thread, w_load_balancer, (void *)load_balancer_args)) {
        return false;
    }

    for (size_t i = 0; i < worker_count; i++) {
        if (!mthread_create(worker_threads + i, w_worker, (void *)(worker_args + i))) {
            return false;
        }
    }

    if (!mthread_create(sender_thread, w_sender, (void *)sender_args)) {
        return false;
    }

    if (!mthread_create(admin_thread, w_admin, (void *)admin_args)) {
        return false;
    }

    return true;
}

bool initialize(coordination_t * coordination, int * main_socket, queue_t * reciever_queue, queue_t * sender_queue, queue_t * worker_queues, arguments_t * reciever_args, arguments_t * load_balancer_args, arguments_t * worker_args, arguments_t * sender_args, arguments_t * admin_args, mthread_t * reciever_thread, mthread_t * load_balancer_thread, mthread_t * worker_threads, mthread_t * sender_thread, mthread_t * admin_thread, size_t worker_count) {
    *main_socket = init_socket(SOCK_DGRAM, 0, SERVER_PORT);
    if (*main_socket < 0) {
        perror("[INIT] Socket initialization failed.");
        return false;
    }

    init_coordination(coordination);

    init_queue(reciever_queue, QUEUE_CAPACITY_UNLIMITED);
    init_queue(sender_queue, QUEUE_CAPACITY_UNLIMITED);
    for (size_t i = 0; i < worker_count; ++i) {
        init_queue(worker_queues + i, QUEUE_CAPACITY_DEFAULT);
    }

    init_args(reciever_args, load_balancer_args, worker_args, sender_args, admin_args, worker_count, *main_socket, coordination, reciever_queue, sender_queue, worker_queues);
    sieve_of_erathostenes(primes, N - 1);
    int success = spawn_threads(reciever_thread, load_balancer_thread, worker_threads, sender_thread, admin_thread, reciever_args, load_balancer_args, worker_args, sender_args, admin_args, worker_count);

    if (!success) {
        handle_halt(coordination);
        join_threads(reciever_thread, load_balancer_thread, sender_thread, admin_thread, worker_threads, worker_count);
        free_resources(coordination, reciever_queue, sender_queue, worker_queues, worker_count, *main_socket);
        return false;
    }

    return true;
}

int main() {
    int            main_socket;
    coordination_t coordination;
    queue_t        reciever_queue, sender_queue, worker_queues[WORKER_NUMBER];
    mthread_t      reciever_thread, load_balancer_thread, sender_thead, admin_thread, worker_threads[WORKER_NUMBER];
    arguments_t    reciever_args, load_balancer_args, sender_args, admin_args, worker_args[WORKER_NUMBER];

    // Initializing resources. If anything fails, the function cleans up the rest.
    if (!initialize(&coordination, &main_socket, &reciever_queue, &sender_queue, worker_queues, &reciever_args, &load_balancer_args, worker_args, &sender_args, &admin_args, &reciever_thread, &load_balancer_thread, worker_threads, &sender_thead, &admin_thread, WORKER_NUMBER)) {
        return -1;
    }

    // Joining threads
    join_threads(&reciever_thread, &load_balancer_thread, &sender_thead, &admin_thread, worker_threads, WORKER_NUMBER);

    // Freeing resources
    free_resources(&coordination, &reciever_queue, &sender_queue, worker_queues, WORKER_NUMBER, main_socket);

    return 0;
}
