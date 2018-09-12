#ifndef TEST_ATINY_STUB_H
#define TEST_ATINY_STUB_H

#include "agenttiny.h"
#include "atiny_log.h"
#include "atiny_adapter.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#define __WEAK__ __attribute__((weak))

#define STUB_FUNCTION(name) __stub_fun__##name
#define DECLARE_STUB_R(name, ret, ret_value, args, ...)\
static ret (*STUB_FUNCTION(name)) args = NULL;\
ret name args\
{\
    if(STUB_FUNCTION(name))\
    {\
        return STUB_FUNCTION(name)(__VA_ARGS__);\
    }\
    return ret_value;\
}

#define DECLARE_STUB_V(name, args, ...)\
static void (*STUB_FUNCTION(name)) args = NULL;\
void name args\
{\
    if(STUB_FUNCTION(name))\
    {\
        STUB_FUNCTION(name)(__VA_ARGS__);\
    }\
}


#define STUB_FUN(name, func)\
do\
{\
    STUB_FUNCTION(name) = (func);\
}while(0)

#define UNSTUB_FUN(name)\
do\
{\
    STUB_FUNCTION(name) = NULL;\
}while(0)




#ifdef __cplusplus
extern "C" {
#endif



#ifdef __cplusplus
}
#endif

#endif


