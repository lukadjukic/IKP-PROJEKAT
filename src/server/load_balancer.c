#include "server_helper.h"

static inline void register_to_worker_queues(queue_t * const queues, const size_t workers_count) {
    for (size_t i = 0; i < workers_count; i++) {
        register_producer(queues + i);
    }
}

static inline void deregister_to_worker_queues(queue_t * const queues, const size_t workers_count) {
    for (size_t i = 0; i < workers_count; i++) {
        deregister_producer(queues + i);
    }
}

size_t select_worker(queue_t * worker_queues, const size_t workers_count) {
    while (true) {
        for (size_t i = 0; i < workers_count; i++) {
            pthread_mutex_lock(&(worker_queues[i].lock));
            size_t capacity = worker_queues[i].capacity;
            size_t size     = worker_queues[i].size;
            pthread_mutex_unlock(&(worker_queues[i].lock));
            if ((!capacity || size < capacity) && !is_queue_stopped(worker_queues + i)) {
                return i;
            }
        }
        sleep(1);
    }
}

static inline void balance(queue_t * const input, queue_t * const worker_queues, const size_t workers_count) {
    while (true) {
        request_t * request = dequeue(input);
        if (!request) {
            return;
        }

        size_t selected_worker = select_worker(worker_queues, workers_count);

        printf("[Load Balancer] Request id: %lu assigned to thread %lu.\n", request->id, selected_worker);
        enqueue(worker_queues + selected_worker, request);
    }
}

void load_balancer(queue_t * const input, queue_t * const worker_queues, const size_t worker_count) {
    register_to_worker_queues(worker_queues, worker_count);
    balance(input, worker_queues, worker_count);
    deregister_to_worker_queues(worker_queues, worker_count);
}
