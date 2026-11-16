#include "server_helper.h"

static inline bool sealing_exeeded(const ssize_t * arguments, size_t argument_count, size_t n) {
    for (size_t i = 0; i < argument_count; i++) {
        if (arguments[i] > 0 && (size_t)arguments[i] > n)
            return true;
    }

    return false;
}

static inline void eliminate(const ssize_t * arguments, bool * is_eliminated, size_t argument_count, const bool * primes) {
    for (size_t i = 0; i < argument_count; i++) {
        if (arguments[i] < 0) {
            is_eliminated[i] = true;
            continue;
        }

        is_eliminated[i] = !primes[arguments[i]];
    }
}

static inline ssize_t find_largest(const ssize_t * arguments, bool * is_eliminated, size_t argument_count) {
    ssize_t index = -1;
    ssize_t max   = -1;

    for (size_t i = 0; i < argument_count; i++) {
        if (!is_eliminated[i] && arguments[i] > max) {
            max   = arguments[i];
            index = (ssize_t)i;
        }
    }

    return index;
}

//  Returns the index of the largest prime or -1 if there are none or -2 if the largest prime is larger than the upper sealing N
ssize_t find_largest_prime(const ssize_t * const arguments, const bool * primes, size_t n) {
    size_t argument_count               = MAX_ARGUMENTS;
    bool   is_eliminated[MAX_ARGUMENTS] = {false};

    if (sealing_exeeded(arguments, argument_count, n)) {
        return -2;
    }

    eliminate(arguments, is_eliminated, argument_count, primes);
    return find_largest(arguments, is_eliminated, argument_count);
}

static inline ssize_t operation_0(const ssize_t * const arguments) {
    ssize_t argument = (ssize_t)arguments[0];
    return (argument * (argument + 1)) >> 1;
}

static inline ssize_t operation_1(const ssize_t * const arguments) {
    ssize_t       argument = arguments[0];
    const ssize_t mask     = argument >> (sizeof(ssize_t) * 0x08 - 1);
    return (argument + mask) ^ mask;
}

static inline ssize_t operation_2(const ssize_t * const arguments) {
    ssize_t result = 0;
    for (int i = 0; i < MAX_ARGUMENTS; ++i) {
        result ^= arguments[i];
    }
    return result;
}

static void process(queue_t * queue, request_t * request, const bool * primes, size_t n) {
    if (is_queue_stopped(queue)) {
        request->flags.stopped = 1;
        return;
    } else if (request->operation > 3) {
        request->flags.invalid = 1;
        return;
    }

    switch (request->operation) {
    case 0:
        request->result = operation_0(request->arguments);
        break;
    case 1:
        request->result = operation_1(request->arguments);
        break;
    case 2:
        request->result = operation_2(request->arguments);
        break;
    case 3:
        request->result        = find_largest_prime(request->arguments, primes, n);
        request->flags.invalid = request->result == -2;
        break;
    default:
        break;
    }

    request->flags.handled = 1;
}

void worker(queue_t * const input, queue_t * const output, const int id, const bool * primes, size_t n) {
    register_producer(output);
    while (true) {
        request_t * request = dequeue(input);

        if (!request) {
            break;
        }

        process(input, request, primes, n);
        printf("[Worker %d] Worker handled request id: %lu.\n", id, request->id);

        enqueue(output, request);
    }
    deregister_producer(output);
}

void sieve_of_erathostenes(bool * primes, size_t n) {
    for (size_t i = 2; i <= n; ++i)
        primes[i] = true;

    primes[0] = primes[1] = false;

    for (size_t prime = 2; prime <= n; ++prime) {
        if (!primes[prime])
            continue;
        for (size_t num = prime * prime; num <= n; num += prime)
            primes[num] = false;
    }
}
