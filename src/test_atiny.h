#ifndef TEST_ATINY_H
#define TEST_ATINY_H

#include "agenttiny.h"
#include "atiny_log.h"
#include "atiny_adapter.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>


#ifndef MALLOC
#define MALLOC atiny_malloc
#endif

#ifndef FREE
#define FREE atiny_free
#endif

#define TEST_OK 0

#define SLEEP_FOREVER()\
    while(1)atiny_usleep(0xffffffff)

#define TEST_LOG(arg...) ATINY_LOG(LOG_INFO, arg)
#define EXPECT_TRUE(condition) \
do\
{\
    if(!(condition))\
    {\
        TEST_LOG("EXPECT_TRUE:"#condition);\
        SLEEP_FOREVER();\
    }\
}while(0)

#define TEST_DECLARE_CLASS(test_class, name) \
do{\
    test_class = test_class_create(name);\
    test_add_class(test_class);\
}while(0)

#define TEST_ADD_CASE(test_class, func) test_class_add(test_class, #func, func)

struct test_class_tag_s;
typedef struct test_class_tag_s test_class_s;


#ifdef __cplusplus
extern "C" {
#endif

test_class_s *test_class_create(const char *name);
void test_class_destroy(test_class_s *test_class);
void test_class_add(test_class_s *test_class, const char *name, void(*test_case)(void));
void test_class_set(test_class_s *test_class, void(*setup)(void),
                           void(*setdown)(void), void(*setup_case)(void), void(*setdown_case)(void));


void test_main(void);
void test_init(void);
void test_add_class(test_class_s *test_class);

#ifdef __cplusplus
}
#endif

#endif


