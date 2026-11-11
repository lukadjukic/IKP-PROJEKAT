#include <stdlib.h>
#include "unity/unity.h"
#include "../../src/common/queue.h"

#define QUEUE_TEST_INITIAL_CAPACITY 10
static queue_t tq;
static bool tq_destroyed = false;

void setUp(void)
{
    init_queue(&tq, QUEUE_TEST_INITIAL_CAPACITY);
}

void tearDown(void)
{
    if (!tq_destroyed)
        delete_queue(&tq);
}

void test_init_queue(void)
{
    TEST_ASSERT_EQUAL_PTR(NULL, tq.first);
    TEST_ASSERT_EQUAL_PTR(NULL, tq.last);
    TEST_ASSERT_EQUAL_size_t(QUEUE_TEST_INITIAL_CAPACITY, tq.capacity);
    TEST_ASSERT_EQUAL_size_t(0, tq.size);
    TEST_ASSERT_EQUAL_size_t(0, tq.producer_count);
    TEST_ASSERT_EQUAL_INT32(QUEUE_INITIALIZED, tq.flags);
}

void test_delete_queue(void)
{
    delete_queue(&tq);
    tq_destroyed = true;
    TEST_ASSERT_EQUAL_PTR(NULL, tq.first);
    TEST_ASSERT_EQUAL_PTR(NULL, tq.last);
    TEST_ASSERT_EQUAL_size_t(0, tq.size);    
}

void test_register_producer(void)
{
    register_producer(&tq);
    TEST_ASSERT_EQUAL_size_t(1, tq.producer_count);
    TEST_ASSERT_EQUAL_INT16(QUEUE_STARTED, tq.flags);
}

void test_deregister_producer(void)
{
    tq.producer_count = 1;
    deregister_producer(&tq);
    TEST_ASSERT_EQUAL_size_t(0, tq.producer_count);
}

void test_increment_queue_capacity_limited(void)
{
    const size_t increment = 10;
    increment_queue_capacity(&tq, increment);
    TEST_ASSERT_EQUAL_size_t(QUEUE_TEST_INITIAL_CAPACITY + increment, tq.capacity);
}

void test_increment_queue_capacity_unlimited(void)
{
    delete_queue(&tq);
    init_queue(&tq, QUEUE_CAPACITY_UNLIMITED);
    const size_t increment = 10;
    increment_queue_capacity(&tq, increment);
    TEST_ASSERT_EQUAL_size_t(QUEUE_CAPACITY_UNLIMITED, tq.capacity);
}

void test_enqueue(void)
{
    const size_t n = 10;
    request_t * request = malloc(sizeof(request_t));
    register_producer(&tq);
    for (size_t i = 0; i < n; i++)
        enqueue(&tq, request);
    free(request);

    TEST_ASSERT_EQUAL_size_t(n, tq.size);
    TEST_ASSERT_NOT_EQUAL_PTR(NULL, tq.first);
    
    for (size_t i = 0; i < n; i++)
        free(dequeue(&tq));
    deregister_producer(&tq);
}

void test_dequeue(void)
{
    const size_t n = 10;
    request_t * request = malloc(sizeof(request_t));

    register_producer(&tq);
    for (size_t i = 0; i < n; i++)
        enqueue(&tq, request);
    
    free(request);

    for (size_t i = 0; i < n; i++)
        free(dequeue(&tq));
    
    TEST_ASSERT_EQUAL_size_t(0, tq.size);
    TEST_ASSERT_EQUAL_PTR(NULL, tq.first);
    deregister_producer(&tq);
}

void test_enqueue_dequeue(void)
{
    const size_t n = 10;
    request_t * request = malloc(sizeof(request_t));
    size_t ids[n];
    size_t expected[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};  
    
    register_producer(&tq);
    for (size_t i = 0; i < n; i++){
        request->id = i;
        enqueue(&tq, request);
    }

    for (size_t i = 0; i < n; i++){
        request_t * r = dequeue(&tq);
        ids[i] = r->id;
        free(r);
    }
    deregister_producer(&tq);

    TEST_ASSERT_EQUAL_size_t_ARRAY(expected, ids, n);
}

void test_stop_queue(void)
{
    stop_queue(&tq);
    TEST_ASSERT_TRUE((tq.flags & QUEUE_STOPPED) != 0);
}

void test_resume_queue(void)
{
    stop_queue(&tq);
    resume_queue(&tq);
    TEST_ASSERT_TRUE((tq.flags & QUEUE_STOPPED) == 0);
}

void test_is_queue_stopped_true()
{
    stop_queue(&tq);
    TEST_ASSERT_TRUE(is_queue_stopped(&tq) != 0);
}

void test_is_queue_stopped_false()
{
    TEST_ASSERT_TRUE(is_queue_stopped(&tq) == false);
}