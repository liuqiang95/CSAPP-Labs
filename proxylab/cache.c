#include "cache.h"


ProxyCache cache;
static long long unsigned cache_time;
sem_t timemutex;

void init_cache(){
    cache.size = MAX_LINE_CNT;
    cache_time = 0;
    Sem_init(&timemutex, 0, 1);
    for(int i=0;i<cache.size; i++){
        cache.lines[i].valid = 0;
        cache.lines[i].rcnt = 0;
        cache.lines[i].wcnt = 0;
        Sem_init(&cache.lines[i].rmutex,0,1);
        Sem_init(&cache.lines[i].wmutex,0,1);
        Sem_init(&cache.lines[i].rcntmutex,0,1);
        Sem_init(&cache.lines[i].wcntmutex,0,1);    
    }
}

int find_cache(char *urltag, int fd){
    int i;
    for(i=0;i<cache.size;i++){
        before_r(i);
        if( cache.lines[i].valid && !strcmp(urltag,cache.lines[i].urltag) ){
            Rio_writen(fd, cache.lines[i].block, strlen(cache.lines[i].block));
            break;
        }
        after_r(i);
    }
    if(i == cache.size)
        return -1;
    before_w(i);
    update_time(i);
    after_w(i);
    return i;
}

void add_cache(char *urltag, char *buf){
    int mint = MAX_TIME, dest = -1;
    for(int i=0; i<cache.size; i++){
        before_r(i);
        if(!cache.lines[i].valid){
            dest = i;
            after_r(i);
            break;
        }
        if(cache.lines[i].time < mint){ 
            dest = i;
          //  mint = cache.lines[i].time;
        }
        after_r(i);
    }
    before_w(dest);
    strcpy(cache.lines[dest].block, buf);
    strcpy(cache.lines[dest].urltag, urltag);
    cache.lines[dest].valid = 1;
    update_time(dest);
    after_w(dest);
}

void update_time(int index){
    P(&timemutex); // avoid time race between different cache lines
    cache_time ++;
    cache.lines[index].time = cache_time;
    V(&timemutex);
}

void before_r(int index){
    P(&cache.lines[index].rmutex);
    P(&cache.lines[index].rcntmutex);
    cache.lines[index].rcnt++;
    if(cache.lines[index].rcnt==1)
        P(&cache.lines[index].wmutex);
    V(&cache.lines[index].rcntmutex);
    V(&cache.lines[index].rmutex);
}

void after_r(int index){
    P(&cache.lines[index].rcntmutex);
    cache.lines[index].rcnt--;
    if(cache.lines[index].rcnt==0) 
        V(&cache.lines[index].wmutex);
    V(&cache.lines[index].rcntmutex);
}

void before_w(int index){
    P(&cache.lines[index].wcntmutex);
    cache.lines[index].wcnt++;
    if(cache.lines[index].wcnt==1) 
        P(&cache.lines[index].rmutex);
    V(&cache.lines[index].wcntmutex);
    P(&cache.lines[index].wmutex);
}

void after_w(int index){
    V(&cache.lines[index].wmutex);
    P(&cache.lines[index].wcntmutex);
    cache.lines[index].wcnt--;
    if(cache.lines[index].wcnt==0) 
        V(&cache.lines[index].rmutex);
    V(&cache.lines[index].wcntmutex);
}