#ifndef __DEBUG_H__
#define __DEBUG_H__
#include<stdlib.h>
#include<stdio.h>
#define PRINT_LOCAT \
    printf("[%s:%s:%d]:", __FILE__, __func__, __LINE__);

#define LOG(str, ...)           \
    printf("\033[20;35mLOG: "); \
    PRINT_LOCAT                 \
    printf(str, __VA_ARGS__);   \
    printf("\033[0m\n");
    
#define SHOW(str)  \
    printf("\033[20;35mLOG: "); \
    PRINT_LOCAT                 \
    printf(str);   \
    printf("\033[0m\n");
#define assert(a)                                                                 \
    if (!(a))                                                                     \
    {                                                                             \
        printf("\033[20;33massert failed at %s:%d\033[0m\n", __FILE__, __LINE__); \
        exit(-1);                                                                 \
    }
#define picnic(str)       \
    printf("\033[20;33m"); \
    PRINT_LOCAT           \
    printf(str);          \
    printf("\033[0m\n");  \
    assert(0);

#endif