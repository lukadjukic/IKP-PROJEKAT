#include "unity/unity.h"
#include "../../src/common/queue.h"

static queue_t tq;

void setUp(void)
{
    init_queue(&tq, 10);
}

void tearDown(void)
{
    delete_queue(&tq);
}

void test_init_queue(void)
{
    TEST_ASSERT_EQUAL_PTR(tq.first, NULL);
    TEST_ASSERT_EQUAL_PTR(tq.last, NULL);
    TEST_ASSERT_EQUAL_size_t(tq.capacity, 10);
    TEST_ASSERT_EQUAL_size_t(tq.size, 0);
    TEST_ASSERT_EQUAL_size_t(tq.producer_count, 0);
    TEST_ASSERT_EQUAL_INT32(tq.flags, QUEUE_INITIALIZED);
}

void test_delete_queue(void){}
void test_register_producer(void){}
void test_deregister_producer(void){}
void test_increment_queue_capacity(void){}
void test_enqueue(void){}
void test_dequeue(void){}
void test_stop_queue(void){}
void test_is_queue_stopped(){}


