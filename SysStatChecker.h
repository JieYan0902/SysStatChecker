#ifndef SYS_STAT_CHECK
#define SYS_STAT_CHECK

#include "stdio.h"

//type defines
typedef void (*handler_t)(void* args);
typedef int (*standard_t)(void* args);
typedef void (*callback_t)(long*);

typedef struct entry_t{
    const char * name;
    standard_t std;
    void* std_arg;
    handler_t handler;
    void*  handler_arg;
    struct entry_t* next;
}Entry_t;

typedef enum res_code{
    RES_OK = 0,
    RES_EXIST = -1,
    RES_NONEXIST = -2,
    RES_NOENTRY = -3,
    RES_RUNNING = -4,
    RES_THREAD = -5,
    RES_NOTRUNNING = -6,
    RES_EREGISTER = -7,
    RES_INVENTRY = -8,
    RES_NOMEM = -9,
}res_code;

res_code registerEntry(Entry_t* entry);
res_code startScanning(callback_t callback,int time);
res_code stopScanning(void);

#endif

