#include "server_helper.h"

static unsigned char process_single_request(queue_t * queue, request_t * request);
static void          proccess(request_t * request);
static unsigned char validate_request(request_t * request);
static ssize_t       operation_0(const ssize_t * const arguments);
static ssize_t       operation_1(const ssize_t * const arguments);
static ssize_t       operation_2(const ssize_t * const arguments);
static ssize_t       find_largest_prime(const ssize_t * const arguments);

#define N 1000000
mutex_t       mut = PTHREAD_MUTEX_INITIALIZER;
unsigned char primes[N];
size_t        max_prime = 0;

#pragma region worker

void worker(queue_t * const input, queue_t * const output, const int id) {
    register_producer(output);
    while (1) {
        request_t * request = dequeue(input);
        if (!process_single_request(input, request)) {
            break;
        }
        printf("[Worker %d] Worker handled request id: %lu.\n", id, request->id);
        enqueue(output, request);
    }
    deregister_producer(output);
}

unsigned char process_single_request(queue_t * queue, request_t * request) {
    if (!request) {
        return 0;
    }

    if (is_queue_stopped(queue)) {
        request->flags.stopped = 1;
        return 1;
    }

    proccess(request);
    return 1;
}

void proccess(request_t * request) {
    if (!validate_request(request)) {
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
        request->result        = find_largest_prime(request->arguments);
        request->flags.invalid = request->result == -2;
        break;
    default:
        break;
    }
    request->flags.handled = 1;
}

unsigned char validate_request(request_t * request) {
    // print_request(request);
    if (request->flags.stopped)
        return 0;
    if (request->operation > 3) {
        request->flags.invalid = 1;
        return 0;
    }

    return 1;
}

ssize_t operation_0(const ssize_t * const arguments) {
    ssize_t argument = (ssize_t)arguments[0];
    return (argument * (argument + 1)) >> 1;
}

ssize_t operation_1(const ssize_t * const arguments) {
    ssize_t       argument = arguments[0];
    const ssize_t mask     = argument >> (sizeof(ssize_t) * 0x08 - 1);
    return (argument + mask) ^ mask;
}

ssize_t operation_2(const ssize_t * const arguments) {
    ssize_t result = 0;
    for (int i = 0; i < MAX_ARGUMENTS; ++i) {
        result ^= arguments[i];
    }
    return result;
}

ssize_t find_largest_prime_index(const ssize_t * const arguments, const unsigned char * const is_eliminated, size_t argument_count) {
    ssize_t largest_prime_index = -1;
    for (size_t i = 0; i < argument_count; ++i)
        if (!is_eliminated[i] && (largest_prime_index == -1 || arguments[i] > arguments[largest_prime_index]))
            largest_prime_index = (ssize_t)i;

    return largest_prime_index;
}

void eliminate_negatives(const ssize_t * const arguments, unsigned char * is_eliminated, size_t argument_count) {
    for (size_t i = 0; i < argument_count; ++i)
        is_eliminated[i] = arguments[i] <= 0;
}

void eliminate_already_calculated(const ssize_t * const arguments, unsigned char * is_eliminated, size_t argument_count) {
    pthread_mutex_lock(&mut);
    for (size_t i = 0; i < argument_count; ++i)
        if (arguments[i] > 0)
            is_eliminated[i] = !primes[arguments[i]];

    pthread_mutex_unlock(&mut);
}

void sieve_of_erathostenes(ssize_t to, unsigned char * primes) {
    if (to > N || to < 0)
        return;

    for (ssize_t i = 2; i <= to; ++i)
        primes[i] = 1;

    primes[0] = primes[1] = 0;

    for (ssize_t prime = 2; prime <= to; ++prime) {
        if (!primes[prime])
            continue;
        for (ssize_t num = prime * prime; num <= to; num += prime)
            primes[num] = 0;
    }
}

ssize_t get_max(const ssize_t * const arguments, size_t argument_count) {
    ssize_t max = arguments[0];
    for (size_t i = 1; i < argument_count; ++i)
        if (max < arguments[i])
            max = arguments[i];
    return max;
}

int eliminate_positive_not_primes(const ssize_t * const arguments, unsigned char * is_eliminated, size_t argument_count) {
    ssize_t max = get_max(arguments, argument_count);
    if (max > N)
        return -1;
    if (max < 0)
        return 0;

    pthread_mutex_lock(&mut);
    if ((size_t)max > max_prime) {
        sieve_of_erathostenes(N, primes);
        max_prime = N;
    }
    pthread_mutex_unlock(&mut);

    eliminate_already_calculated(arguments, is_eliminated, argument_count);
    return 0;
}

//  Returns the index of the largest prime or -1 if there are none or -2 if the largest prime is larger than the upper sealing N

ssize_t find_largest_prime(const ssize_t * const arguments) {
    size_t        argument_count               = MAX_ARGUMENTS;
    unsigned char is_eliminated[MAX_ARGUMENTS] = {0};

    eliminate_negatives(arguments, is_eliminated, argument_count);
    if (eliminate_positive_not_primes(arguments, is_eliminated, argument_count) == -1)
        return -2;
    return find_largest_prime_index(arguments, is_eliminated, argument_count);
}

#pragma endregion
