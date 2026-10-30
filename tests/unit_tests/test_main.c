#include "unity/unity.h"

void test_init_queue(void);
void test_delete_queue(void);
void test_register_producer(void);
void test_deregister_producer(void);

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_init_queue);
    RUN_TEST(test_delete_queue);
    RUN_TEST(test_register_producer);
    RUN_TEST(test_deregister_producer);

    return UNITY_END();
}