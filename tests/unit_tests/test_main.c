#include "unity/unity.h"

void test_init_queue(void);
void test_delete_queue(void);
void test_register_producer(void);
void test_deregister_producer(void);
void test_increment_queue_capacity_limited(void);
void test_increment_queue_capacity_unlimited(void);
void test_enqueue(void);
void test_dequeue(void);
void test_enqueue_dequeue(void);
void test_stop_queue(void);
void test_resume_queue(void);
void test_is_queue_stopped_true();
void test_is_queue_stopped_false();

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_init_queue);
    RUN_TEST(test_delete_queue);
    RUN_TEST(test_register_producer);
    RUN_TEST(test_deregister_producer);
    RUN_TEST(test_increment_queue_capacity_limited);
    RUN_TEST(test_increment_queue_capacity_unlimited);
    RUN_TEST(test_enqueue);
    RUN_TEST(test_dequeue);
    RUN_TEST(test_enqueue_dequeue);
    RUN_TEST(test_stop_queue);    
    RUN_TEST(test_resume_queue);
    RUN_TEST(test_is_queue_stopped_true);
    RUN_TEST(test_is_queue_stopped_false);    

    return UNITY_END();
}