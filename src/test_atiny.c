/*
#include "atiny_rpt.h"
#include "test_atiny_rpt.h"

#include "atiny_log.h"
#include "liblwm2m.h"
#include "internals.h"*/
#include "test_atiny.h"
#include <string.h>
#include <stdio.h>
#include <agent_list.h>

#define LOG_ENTER() TEST_LOG("enter...")

typedef struct
{
    void(*test_func)(void);
    const char *name;
}test_case_s;

struct test_class_tag_s
{
    test_case_s *test_cases;
    int test_case_num;
    int current_num;
    const char *name;
    void(*setup)(void);
    void(*setdown)(void);
    void(*setup_case)(void);
    void(*setdown_case)(void);
};

test_class_s *test_class_create(const char *name)
{
    test_class_s * test_class = MALLOC(sizeof(test_class_s));
    EXPECT_TRUE(name != NULL);
    EXPECT_TRUE(test_class != NULL);
    memset(test_class, 0, sizeof(*test_class));
    test_class->test_case_num = 4;
    test_class->test_cases = (test_case_s *)MALLOC(sizeof(test_case_s) * test_class->test_case_num);
    EXPECT_TRUE(test_class->test_cases != NULL);
    memset(test_class->test_cases, 0, sizeof(test_case_s) * test_class->test_case_num);
    test_class->name = name;
    return test_class;
}

void test_class_destroy(test_class_s *test_class)
{
    EXPECT_TRUE(test_class != NULL);
    if(test_class->test_cases)
    {
        FREE(test_class->test_cases);
    }
    FREE(test_class);
}

static void test_class_run(test_class_s *test_class)
{
    TEST_LOG("runing test class %s", test_class->name);
    if(test_class->setup)
    {
        test_class->setup();
    }
    for(int i = 0; i < test_class->current_num; i++)
    {
        if(test_class->test_cases[i].test_func)
        {
            if(test_class->setup_case)
            {
                test_class->setup_case();
            }

            TEST_LOG("runing test case %s", test_class->test_cases[i].name);
            test_class->test_cases[i].test_func();

            if(test_class->setdown_case)
            {
                test_class->setdown_case();
            }
        }
    }
    if(test_class->setdown)
    {
        test_class->setdown();
    }
}

void test_class_add(test_class_s *test_class, const char *name, void(*test_case)(void))
{
    EXPECT_TRUE((name != NULL) && (test_case != NULL));
    if(test_class->current_num >= test_class->test_case_num)
    {
        test_case_s *tmp = (test_case_s *)MALLOC(test_class->test_case_num * 2 * sizeof(test_case_s));
        EXPECT_TRUE(tmp != NULL);
        memcpy(tmp, test_class->test_cases, test_class->test_case_num * sizeof(test_case_s));
        FREE(test_class->test_cases);
        test_class->test_cases = tmp;
        test_class->test_case_num = test_class->test_case_num * 2;
    }
    test_class->test_cases[test_class->current_num].test_func = test_case;
    test_class->test_cases[test_class->current_num].name = name;
    test_class->current_num++;
}

void test_class_set(test_class_s *test_class, void(*setup)(void),
                           void(*setdown)(void), void(*setup_case)(void), void(*setdown_case)(void))
{
    EXPECT_TRUE(test_class != NULL);
    test_class->setup = setup;
    test_class->setup_case = setup_case;
    test_class->setdown = setdown;
    test_class->setdown_case = setdown_case;
}


typedef struct
{
     atiny_dl_list list;
     test_class_s *test_class;
}test_node_s;

typedef struct
{
    atiny_dl_list list;
}test_entry_s;


void test_entry_init(test_entry_s *entry)
{
    atiny_list_init(&entry->list);
}


static void test_free_list(atiny_dl_list* list,  void(*free_data)(atiny_dl_list* node, void* param),  void* param)
{
    atiny_dl_list* item;
    atiny_dl_list* next;

    ATINY_DL_LIST_FOR_EACH_SAFE(item, next, list)
    {
        atiny_list_delete(item);

        if (free_data != NULL)
        {
            free_data(item, param);
        }
        FREE(item);
    }
}

static void test_free_data(atiny_dl_list* node, void* param)
{
    test_node_s *test_node = (test_node_s *)node;
    if(test_node->test_class)
    {
        test_class_destroy(test_node->test_class);
        test_node->test_class = NULL;
    }
}

void test_entry_destry(test_entry_s *entry)
{
    test_free_list(&entry->list, test_free_data, NULL);
}

void test_entry_add_class(test_entry_s *entry, test_class_s *test_class)
{
     test_node_s *test_node = MALLOC(sizeof(*test_node));
     EXPECT_TRUE(test_node != NULL);
     memset(test_node, 0, sizeof(*test_node));
     test_node->test_class = test_class;
     atiny_list_insert_tail(&entry->list, &test_node->list);
}

static void test_visit_list(atiny_dl_list* list,  void(*visit_data)(atiny_dl_list* node, void* param), void* param)
{
    atiny_dl_list* item;
    atiny_dl_list* next;

    ATINY_DL_LIST_FOR_EACH_SAFE(item, next, list)
    {
        visit_data(item, param);
    }
}
static void test_visit_data(atiny_dl_list* node, void* param)
{
    test_node_s *test_node = (test_node_s *)node;
    if(test_node->test_class)
    {
        test_class_run(test_node->test_class);
    }
}


void test_entry_run(test_entry_s *entry)
{
    test_visit_list(&entry->list, test_visit_data, NULL);
}




static test_entry_s g_test_entry;

void test_add_class(test_class_s *test_class)
{
    test_entry_add_class(&g_test_entry,  test_class);
}



void test_main(void)
{
    TEST_LOG("test begin");
    test_entry_init(&g_test_entry);
    test_init();
    test_entry_run(&g_test_entry);
    test_entry_destry(&g_test_entry);
    TEST_LOG("test finish");
    SLEEP_FOREVER();
}




