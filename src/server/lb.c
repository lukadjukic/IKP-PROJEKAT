#include "server_helper.h"

#pragma region load_balancer

static inline unsigned char is_valid_id(const int id, const size_t workers_count) {
    return (id >= 0) && ((size_t)id < workers_count);
}

static void register_to_worker_queues(queue_t * const queues, const size_t workers_count) {
    for (size_t i = 0; i < workers_count; ++i) {
        register_producer(queues + i);
    }
}

static void deregister_to_worker_queues(queue_t * const queues, const size_t workers_count) {
    for (size_t i = 0; i < workers_count; ++i) {
        deregister_producer(queues + i);
    }
}

int find_first_free_worker(queue_t * worker_queues, const size_t workers_count) {
    size_t capacity, size, i;

    for (i = 0; i < workers_count; ++i) {
        pthread_mutex_lock(&(worker_queues[i].lock));
        capacity = worker_queues[i].capacity;
        size     = worker_queues[i].size;
        pthread_mutex_unlock(&(worker_queues[i].lock));
        if ((!capacity || size < capacity) && !is_queue_stopped(worker_queues + i)) {
            return (int)i;
        }
    }
    return -1;
}

int select_worker(queue_t * worker_queues, const size_t workers_count) {
    int selected_worker = -1;

    while (1) {
        selected_worker = find_first_free_worker(worker_queues, workers_count);

        if (is_valid_id(selected_worker, workers_count)) {
            return selected_worker;
        }

        sleep(1);
    }
}

static void balance(queue_t * const input, queue_t * const worker_queues, const size_t workers_count) {
    while (1) {
        request_t * request = dequeue(input);
        if (!request) {
            return;
        }

        int selected_worker = select_worker(worker_queues, workers_count);

        printf("[Load Balancer] Request id: %lu assigned to thread %d.\n", request->id, selected_worker);
        enqueue(worker_queues + selected_worker, request);
    }
}

void load_balancer(queue_t * const input, queue_t * const worker_queues) {
    register_to_worker_queues(worker_queues, WORKER_NUMBER);
    balance(input, worker_queues, WORKER_NUMBER);
    deregister_to_worker_queues(worker_queues, WORKER_NUMBER);
}

#pragma endregion
