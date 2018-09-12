
#include "test_atiny.h"
#include "liblwm2m.h"
#include "los_task.h"
#include "test_atiny_stub.h"


void lwm2m_initBsCtrlStat(lwm2m_context_t *contextP, lwm2m_bootstrap_type_e bs_type);
void lwm2m_setBsCtrlStat(lwm2m_context_t *contextP, lwm2m_client_state_t state);
lwm2m_client_state_t lwm2m_getBsCtrlStat(const lwm2m_context_t *contextP);
bool lwm2m_delayBs(lwm2m_context_t *contextP);
bool lwm2m_isBsCtrlInServerInitiatedBs(const lwm2m_context_t *contextP);

DECLARE_STUB_R(lwm2m_is_sec_obj_uri_valid, bool, true, (uint16_t secObjInstID, void *userData),
                    secObjInstID, userData)

static bool lwm2m_is_sec_obj_uri_valid_stub(uint16_t secObjInstID, void *userData)
{
    (void)secObjInstID;
    (void)userData;
    return false;
}







typedef struct
{
    lwm2m_context_t context;
    lwm2m_server_t server;
    lwm2m_server_t bs_server;
}test_bs_ctrl_s;

static test_bs_ctrl_s g_test;


static void setup()
{
    TEST_LOG("ver03");
    memset(&g_test, 0, sizeof(g_test));
    g_test.context.serverList = &g_test.server;
    g_test.context.bootstrapServerList = &g_test.bs_server;
}

static void setupCase()
{

}


static uint32_t test_get_delay()
{
    uint32_t start_time = lwm2m_gettime();
    while(!lwm2m_delayBs(&g_test.context))
    {
        LOS_TaskDelay(1000);
    }
    return lwm2m_gettime() - start_time;
}

#define FBS_TIME(n) (FACTORY_BS_DELAY_BASE + (n)*FACTORY_BS_DELAY_INTERVAL)
#define CIBS_TIME(n) (CLIENT_INITIATED_BS_DELAY_BASE + (n)*CLIENT_INITIATED_BS_DELAY_INTERVAL)

static void test_factory_bs_mode()
{
    lwm2m_initBsCtrlStat(&g_test.context, BOOTSTRAP_FACTORY);

    for (uint32_t i = 0; i < MAX_FACTORY_BS_RETRY_CNT; ++i)
    {
        lwm2m_setBsCtrlStat(&g_test.context, STATE_REGISTER_REQUIRED);
        uint32_t delay = test_get_delay();
        TEST_LOG("i %ld,delay %ld,cnt %ld", i, delay, g_test.context.bsCtrl.cnt);
        EXPECT_TRUE(delay >= FBS_TIME(i) && delay <= FBS_TIME(i + 1));
        EXPECT_TRUE(lwm2m_getBsCtrlStat(&g_test.context) == STATE_REGISTER_REQUIRED);
    }

    for (uint32_t i = 0; i < MAX_FACTORY_BS_RETRY_CNT; ++i)
    {
        lwm2m_setBsCtrlStat(&g_test.context, STATE_REGISTER_REQUIRED);
        uint32_t delay = test_get_delay();
        EXPECT_TRUE(delay >= FBS_TIME(i) && delay <= FBS_TIME(i + 1));
        EXPECT_TRUE(lwm2m_getBsCtrlStat(&g_test.context) == STATE_REGISTER_REQUIRED);
    }

}

static void test_client_initiated_bs_mode()
{
    lwm2m_initBsCtrlStat(&g_test.context, BOOTSTRAP_CLIENT_INITIATED);

    for (uint32_t i = 0; i < MAX_CLIENT_INITIATED_BS_RETRY_CNT + 1; ++i)
    {
        lwm2m_setBsCtrlStat(&g_test.context, STATE_BOOTSTRAP_REQUIRED);
        uint32_t delay = test_get_delay();
        TEST_LOG("i %ld,delay %ld,cnt %ld", i, delay, g_test.context.bsCtrl.cnt);
        if (i != MAX_CLIENT_INITIATED_BS_RETRY_CNT)
        {
            EXPECT_TRUE(delay >= CIBS_TIME(i) && delay <= CIBS_TIME(i + 1));
            EXPECT_TRUE(lwm2m_getBsCtrlStat(&g_test.context) == STATE_BOOTSTRAP_REQUIRED);
            EXPECT_TRUE(!lwm2m_isBsCtrlInServerInitiatedBs(&g_test.context));
        }
        else
        {
            EXPECT_TRUE(delay >= FBS_TIME(i - MAX_CLIENT_INITIATED_BS_RETRY_CNT) && delay <= FBS_TIME((i + 1) - MAX_CLIENT_INITIATED_BS_RETRY_CNT));
            EXPECT_TRUE(lwm2m_getBsCtrlStat(&g_test.context) == STATE_REGISTER_REQUIRED);
        }


    }

    for (uint32_t i = 1; i < MAX_FACTORY_BS_RETRY_CNT + 1; ++i)
    {
        lwm2m_setBsCtrlStat(&g_test.context, STATE_REGISTER_REQUIRED);
        uint32_t delay = test_get_delay();
        if (i != MAX_FACTORY_BS_RETRY_CNT)
        {
            EXPECT_TRUE(delay >= FBS_TIME(i) && delay <= FBS_TIME(i + 1));
            EXPECT_TRUE(lwm2m_getBsCtrlStat(&g_test.context) == STATE_REGISTER_REQUIRED);
        }
        else
        {
            EXPECT_TRUE(delay >= CIBS_TIME(i - MAX_FACTORY_BS_RETRY_CNT) && delay <= CIBS_TIME((i + 1) - MAX_FACTORY_BS_RETRY_CNT));
            EXPECT_TRUE(lwm2m_getBsCtrlStat(&g_test.context) == STATE_BOOTSTRAP_REQUIRED);
            EXPECT_TRUE(!lwm2m_isBsCtrlInServerInitiatedBs(&g_test.context));
        }

    }


}

static void test_client_initiated_bs_mode_no_iot_server()
{

    g_test.context.serverList = NULL;
    lwm2m_initBsCtrlStat(&g_test.context, BOOTSTRAP_CLIENT_INITIATED);

    for (uint32_t i = 0; i < MAX_CLIENT_INITIATED_BS_RETRY_CNT + 1; ++i)
    {
        lwm2m_setBsCtrlStat(&g_test.context, STATE_BOOTSTRAP_REQUIRED);
        uint32_t delay = test_get_delay();
        TEST_LOG("i %ld,delay %ld,cnt %ld", i, delay, g_test.context.bsCtrl.cnt);
        EXPECT_TRUE(lwm2m_getBsCtrlStat(&g_test.context) == STATE_BOOTSTRAP_REQUIRED);
        EXPECT_TRUE(!lwm2m_isBsCtrlInServerInitiatedBs(&g_test.context));
        if (i != MAX_CLIENT_INITIATED_BS_RETRY_CNT)
        {
            EXPECT_TRUE(delay >= CIBS_TIME(i) && delay <= CIBS_TIME(i + 1));
        }
        else
        {
            EXPECT_TRUE(delay >= FBS_TIME(i - MAX_CLIENT_INITIATED_BS_RETRY_CNT) && delay <= FBS_TIME((i + 1) - MAX_CLIENT_INITIATED_BS_RETRY_CNT));
        }

    }
    g_test.context.serverList = &g_test.server;

}


static void test_server_initiated_bs_mode()
{
    lwm2m_initBsCtrlStat(&g_test.context, BOOTSTRAP_SEQUENCE);


    for (uint32_t i = 0; i < MAX_FACTORY_BS_RETRY_CNT + 1; ++i)
    {
        lwm2m_setBsCtrlStat(&g_test.context, STATE_REGISTER_REQUIRED);
        uint32_t delay = test_get_delay();
        if (i != MAX_FACTORY_BS_RETRY_CNT)
        {
            EXPECT_TRUE(delay >= FBS_TIME(i) && delay <= FBS_TIME(i + 1));
            EXPECT_TRUE(lwm2m_getBsCtrlStat(&g_test.context) == STATE_REGISTER_REQUIRED);
        }
        else
        {
            EXPECT_TRUE(delay >= CIBS_TIME(i - MAX_FACTORY_BS_RETRY_CNT) && delay <= CIBS_TIME((i + 1) - MAX_FACTORY_BS_RETRY_CNT));
            EXPECT_TRUE(lwm2m_getBsCtrlStat(&g_test.context) == STATE_BOOTSTRAP_REQUIRED);
            EXPECT_TRUE(lwm2m_isBsCtrlInServerInitiatedBs(&g_test.context));
        }

    }

    for (uint32_t i = 0; i < MAX_CLIENT_INITIATED_BS_RETRY_CNT + 1; ++i)
    {
        lwm2m_setBsCtrlStat(&g_test.context, STATE_BOOTSTRAP_REQUIRED);
        uint32_t delay = test_get_delay();
        TEST_LOG("i %ld,delay %ld,cnt %ld", i, delay, g_test.context.bsCtrl.cnt);
        if (i != MAX_CLIENT_INITIATED_BS_RETRY_CNT)
        {
            EXPECT_TRUE(delay >= CIBS_TIME(i) && delay <= CIBS_TIME(i + 1));
            EXPECT_TRUE(lwm2m_getBsCtrlStat(&g_test.context) == STATE_BOOTSTRAP_REQUIRED);
            EXPECT_TRUE(!lwm2m_isBsCtrlInServerInitiatedBs(&g_test.context));
        }
        else
        {
            EXPECT_TRUE(delay >= FBS_TIME(i - MAX_CLIENT_INITIATED_BS_RETRY_CNT) && delay <= FBS_TIME((i + 1) - MAX_CLIENT_INITIATED_BS_RETRY_CNT));
            EXPECT_TRUE(lwm2m_getBsCtrlStat(&g_test.context) == STATE_REGISTER_REQUIRED);
        }

    }

}

static void test_server_initiated_bs_mode_no_iot_server()
{
     g_test.context.serverList = NULL;

    lwm2m_initBsCtrlStat(&g_test.context, BOOTSTRAP_SEQUENCE);

    lwm2m_setBsCtrlStat(&g_test.context, STATE_BOOTSTRAP_REQUIRED);
    uint32_t i = 0;
    uint32_t delay = test_get_delay();
    TEST_LOG("i %ld,delay %ld,cnt %ld", i, delay, g_test.context.bsCtrl.cnt);
    EXPECT_TRUE(delay >= CIBS_TIME(i) && delay <= CIBS_TIME(i + 1));
    EXPECT_TRUE(lwm2m_getBsCtrlStat(&g_test.context) == STATE_BOOTSTRAP_REQUIRED);
    EXPECT_TRUE(lwm2m_isBsCtrlInServerInitiatedBs(&g_test.context));

    for (i = 0; i < MAX_CLIENT_INITIATED_BS_RETRY_CNT + 1; ++i)
    {
        lwm2m_setBsCtrlStat(&g_test.context, STATE_BOOTSTRAP_REQUIRED);
        delay = test_get_delay();
        TEST_LOG("i %ld,delay %ld,cnt %ld", i, delay, g_test.context.bsCtrl.cnt);
        EXPECT_TRUE(lwm2m_getBsCtrlStat(&g_test.context) == STATE_BOOTSTRAP_REQUIRED);
        if (i != MAX_CLIENT_INITIATED_BS_RETRY_CNT)
        {
            EXPECT_TRUE(delay >= CIBS_TIME(i) && delay <= CIBS_TIME(i + 1));
            EXPECT_TRUE(!lwm2m_isBsCtrlInServerInitiatedBs(&g_test.context));
        }
        else
        {
            EXPECT_TRUE(delay >= FBS_TIME(i - MAX_CLIENT_INITIATED_BS_RETRY_CNT) && delay <= FBS_TIME((i + 1) - MAX_CLIENT_INITIATED_BS_RETRY_CNT));
            EXPECT_TRUE(lwm2m_isBsCtrlInServerInitiatedBs(&g_test.context));
        }
    }
    g_test.context.serverList = &g_test.server;
}

static void test_server_initiated_bs_mode_no_iot_server_and_no_bs_ip()
{
     STUB_FUN(lwm2m_is_sec_obj_uri_valid, lwm2m_is_sec_obj_uri_valid_stub);
     g_test.context.serverList = NULL;

    lwm2m_initBsCtrlStat(&g_test.context, BOOTSTRAP_SEQUENCE);

    lwm2m_setBsCtrlStat(&g_test.context, STATE_BOOTSTRAP_REQUIRED);
    uint32_t i = 0;
    uint32_t delay = test_get_delay();
    TEST_LOG("i %ld,delay %ld,cnt %ld", i, delay, g_test.context.bsCtrl.cnt);
    EXPECT_TRUE(delay >= CIBS_TIME(i) && delay <= CIBS_TIME(i + 1));
    EXPECT_TRUE(lwm2m_getBsCtrlStat(&g_test.context) == STATE_BOOTSTRAP_REQUIRED);
    EXPECT_TRUE(lwm2m_isBsCtrlInServerInitiatedBs(&g_test.context));

    lwm2m_setBsCtrlStat(&g_test.context, STATE_BOOTSTRAP_REQUIRED);
    delay = test_get_delay();
    TEST_LOG("i %ld,delay %ld,cnt %ld", i, delay, g_test.context.bsCtrl.cnt);
    EXPECT_TRUE(delay >= CIBS_TIME(i) && delay <= CIBS_TIME(i + 1));
    EXPECT_TRUE(lwm2m_getBsCtrlStat(&g_test.context) == STATE_BOOTSTRAP_REQUIRED);
    EXPECT_TRUE(lwm2m_isBsCtrlInServerInitiatedBs(&g_test.context));

    g_test.context.serverList = &g_test.server;
    UNSTUB_FUN(lwm2m_is_sec_obj_uri_valid);
}

const uint32_t TEST_INTERVAL = 3;

typedef struct
{
    util_timer_t timer;
    uint32_t call_cnt;
}test_timer_s;
static test_timer_s g_test_timer;

static void timer_callback(void *param)
{
    test_timer_s *test = (test_timer_s *)param;
    EXPECT_TRUE(test);
    test->call_cnt++;
}


static void test_timer()
{
    const uint32_t CNT = 2;
    memset(&g_test_timer, 0, sizeof(g_test_timer));


    timer_init(&g_test_timer.timer, TEST_INTERVAL, timer_callback, &g_test_timer);

    for (uint32_t i = 0; i < TEST_INTERVAL * CNT ; ++i)
    {
        LOS_TaskDelay(1000);
        timer_step(&g_test_timer.timer);
    }

    EXPECT_TRUE(g_test_timer.call_cnt == 0);

    timer_start(&g_test_timer.timer);

    for (uint32_t i = 0; i < TEST_INTERVAL * CNT ; ++i)
    {
        LOS_TaskDelay(1000);
        timer_step(&g_test_timer.timer);
    }

    EXPECT_TRUE(g_test_timer.call_cnt == CNT);

    timer_stop(&g_test_timer.timer);
    g_test_timer.call_cnt = 0;

    for (uint32_t i = 0; i < TEST_INTERVAL * CNT ; ++i)
    {
        LOS_TaskDelay(1000);
        timer_step(&g_test_timer.timer);
    }

    EXPECT_TRUE(g_test_timer.call_cnt == 0);

}



void test_init(void)
{

    test_class_s * test_class;

    TEST_DECLARE_CLASS(test_class, "test_bs_control");
    test_class_set(test_class, setup, NULL, setupCase, NULL);
    TEST_ADD_CASE(test_class, test_factory_bs_mode);
    TEST_ADD_CASE(test_class, test_client_initiated_bs_mode);
    TEST_ADD_CASE(test_class, test_client_initiated_bs_mode_no_iot_server);
    TEST_ADD_CASE(test_class, test_server_initiated_bs_mode);
    TEST_ADD_CASE(test_class, test_server_initiated_bs_mode_no_iot_server);
    TEST_ADD_CASE(test_class, test_server_initiated_bs_mode_no_iot_server_and_no_bs_ip);


    TEST_DECLARE_CLASS(test_class, "test_timer");
    test_class_set(test_class, NULL, NULL, NULL, NULL);
    TEST_ADD_CASE(test_class, test_timer);

}


