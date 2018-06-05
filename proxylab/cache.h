/*
 * cache.h - prototypes and definitions for cache.c
 * 
 */
/* $begin cache.h */
#ifndef __CACHE_H__
#define __CACHE_H__
#include "csapp.h"
#include <limits.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define LRU_MAGIC_NUMBER 9999
#define MAX_LINE_CNT 10
#define MAX_TIME ULLONG_MAX

typedef struct {
    char block[MAX_OBJECT_SIZE];
    char urltag[MAXLINE];
    int valid;
    long long unsigned time;    // record current time 
    int rcnt; 
    int wcnt;
    sem_t wmutex;   // mutex able to write
    sem_t rcntmutex;
    sem_t wcntmutex;
    sem_t rmutex;  // mutex able to read
}CacheLine;

typedef struct {
    CacheLine lines[MAX_LINE_CNT]; 
    int size;
}ProxyCache;


void init_cache();
int find_cache(char *urltag, int fd);
void add_cache(char *urltag,char *buf);
void updata_time(int index);

void before_r(int index);   // func before read
void after_r(int index);    // func after read
void before_w(int index);    // func before writer
void after_w(int index);     // func after writer

#endif /* __CACHE_H__ */
/* $end cache.h */