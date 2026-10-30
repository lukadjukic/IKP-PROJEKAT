#include "unity/unity.h"
#include "../../src/common/queue.h"

static queue_t tq;
static bool tq_destroyed = false;

void setUp(void)
{
    init_queue(&tq, 10);
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
    TEST_ASSERT_EQUAL_size_t(10, tq.capacity);
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

void test_increment_queue_capacity(void){}
void test_enqueue(void){}
void test_dequeue(void){}
void test_stop_queue(void){}
void test_is_queue_stopped(){}


