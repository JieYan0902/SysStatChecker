#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <errno.h>
#include "SysStatChecker.h"

#define REP_TIME 3
#define MAX_ITEMS 8
#define LOG_ERR 2

//private variables
static Entry_t* head = NULL;
static pthread_t tid = 0;
static pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
static long flag_variable = 0;
static long flag_variable_pre = 0;
static long report_variable = 0;
static long report_variable_saved = ~0L;
static callback_t overall_handler = NULL;
static char rep_cnt[MAX_ITEMS] = { 0 };
static int isReport;

//private function prototypes
static void*  thread_func(void* arg);
static void   thread_func_cleaner(void* arg);

//public functions
res_code registerEntry(Entry_t* ent){
    Entry_t* entry = NULL;
    Entry_t* ptr = NULL;
    Entry_t* pre = NULL;
    if(ent == NULL){
        fprintf(stderr,"the entry to be registered is NULL!\n");
        return RES_INVENTRY;  
    }
    entry = malloc(sizeof(Entry_t));
    if(entry == NULL){
        fprintf(stderr,"CANNOT allocate memory for a new entry!\n");
        return RES_NOMEM;
    }else{
        entry->name = ent->name;
        entry->std = ent->std;
        entry->std_arg = ent->std_arg;
        entry->handler = ent->handler;
        entry->handler_arg = ent->handler_arg;
        entry->next = ent->next;

    }
    if(head == NULL){
        pthread_rwlock_wrlock(&rwlock);
        head = entry;
        pthread_rwlock_unlock(&rwlock);
    }else{
        pthread_rwlock_rdlock(&rwlock);
        ptr = head;
        while(ptr){
            if(strcmp(ptr->name,entry->name) == 0){
                return RES_EXIST;
            }
            pre = ptr;
            ptr = ptr-> next;
        }
        pthread_rwlock_unlock(&rwlock);
        pthread_rwlock_wrlock(&rwlock);
        pre->next = entry;
        pthread_rwlock_unlock(&rwlock);
    }
    
    return RES_OK;
}

res_code startScanning(callback_t callback,int time){
    pthread_attr_t attr;
    int err;
    if(head == NULL){
        return RES_NOENTRY;
    }
    if(tid){
        return RES_RUNNING;
    }
    if(callback != NULL){
        overall_handler = callback;
    }else{
        printf("No callback handler specified!\n");
    }
    
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
    err = pthread_create(&tid,&attr,thread_func,(void*)time);
    if(err != 0){
        perror("startScanning:");
        tid = 0;
        pthread_attr_destroy(&attr);
        return RES_THREAD;
    }
    pthread_attr_destroy(&attr);
    
    return RES_OK;
}

res_code stopScanning(void){
    void** rval_ptr;
    int res = -1;
    if(!tid){
        return RES_NOTRUNNING;
    }
    pthread_cancel(tid);
    pthread_join(tid,rval_ptr);
    tid = 0;
    return RES_OK;
}

//private functions
static void* thread_func(void* arg){
    int time = (int)arg;
    int old;
    int i = 0;
    char cont;
    Entry_t *ptr,*tmp;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&old);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,&old);
    pthread_cleanup_push(thread_func_cleaner,NULL);
    while(true){
        pthread_testcancel();
        pthread_rwlock_rdlock(&rwlock);
        ptr = head;
        i = 0;
        while(ptr){
            if(ptr->std && ptr->std(ptr->std_arg)){
                flag_variable |= 1 << i;
                if(ptr->handler)
                    ptr->handler(ptr->handler_arg);
            }
            ++i;
            ptr = ptr ->next;
        }
        pthread_rwlock_unlock(&rwlock);

        report_variable = 0;
        isReport = 0;
        for(i = 0 ; i < MAX_ITEMS; i++){
            if(((flag_variable ^ flag_variable_pre) & (1<< i)) == 0){
                if(rep_cnt[i] != -1){
                    ++rep_cnt[i];
                }
            }else{
                rep_cnt[i] = 1;
            }
            
            if(rep_cnt[i] == REP_TIME || rep_cnt[i] == -1){
                if(rep_cnt[i] == REP_TIME){
                    if(((flag_variable^report_variable_saved)&(1<<i)) > 0){
                        isReport = 1;
                    }else{
                        rep_cnt[i] = -1;
                    }
                }
                if((flag_variable & 1<< i) > 0){
                    report_variable |= 1 << i;
                    rep_cnt[i] = -1;
                }else{
                    report_variable &= ~(1 << i); //topple alarm state
                    rep_cnt[i] = -1;  
                } 
            }
                        
        }
        if(isReport){
            if(overall_handler != NULL){
                overall_handler(&report_variable);
            }
            report_variable_saved = report_variable;
        }
        flag_variable_pre = flag_variable;
        flag_variable = 0x0;
        pthread_testcancel();
        sleep(time);
        pthread_testcancel();
    }
    
    //NEVER REACH HERE
    pthread_cleanup_pop(0);
    return NULL;
}

static void   thread_func_cleaner(void* arg){
    Entry_t *ptr = NULL,*tmp = NULL;
    if(head != NULL){
        pthread_rwlock_wrlock(&rwlock);
        ptr = head;
        while(ptr != NULL){
            tmp = ptr->next;
            free(ptr);
            ptr = tmp;
        }
        head = NULL;
        pthread_rwlock_unlock(&rwlock);
    }
}
