#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash_tableMT.h"
#include <time.h>
#include <pthread.h>

THashNode* data = NULL;
const int num_threads = 16;          // 线程数
const int max = 100000000;  // 总条目数（为了测试先减少）
const int batch = max/num_threads;
THashTable* table = NULL;

// 线程工作函数 - 执行插入操作
void* insert_worker(void* arg) {
    unsigned* start1 = (unsigned*)&arg;
    unsigned start = *start1;
    unsigned end = start + batch;
    //printf("Thread %d-%d: begin inserting\n", start,start+batch);
    void* page = NULL;
    THashNode* node = NULL;
    char name[1000];
    for (unsigned i = start; i < end; i++) {
        snprintf(name, 20, "%d_%d", i, i);
        insertHashTable(table,name, &node, &page);
        //printf("\r\n%d",i);
    }
    if(NULL != page){
        free(page);
    }
    //printf("Thread %d-%d: Finished inserting\n", start,start+batch);
    return NULL;
}

int main() {
    // 创建哈希表
    table = createHashTable();
    if (!table) {
        printf("Failed to create hash table\n");
        return -1;
    }
    printf("createHashTable success.\n");
    
    // 创建线程数组
    pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));

    
    printf("\n--- Multi-threaded Insert Operation ---\n");
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    // 启动插入线程
    for (int i = 0; i < num_threads; i++) {
        int ret = pthread_create(&threads[i], NULL, insert_worker, (void*)(i*batch));
        if (ret) {
            printf("Error creating thread %d\n", i);
            return -1;
        }
    }
    
    // 等待所有插入线程完成
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    //insert_worker((void*)0);
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    long elapsed_ns = (end_time.tv_sec - start_time.tv_sec) * 1000000000L
                    + (end_time.tv_nsec - start_time.tv_nsec);
    double elapsed_time = elapsed_ns / 1e9;

    printf("Multi-threaded insert used time: %.2f seconds\n", elapsed_time);
    char name[1000];
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    for (unsigned i = 0; i < max; i++) {
        snprintf(name, 20, "%d_%d", i, i);
        THashNode* node = findHashTable(table, name);
        if(NULL == node){
            printf("\r\n%s not find", name);
        }

    }
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    elapsed_ns = (end_time.tv_sec - start_time.tv_sec) * 1000000000L
                    + (end_time.tv_nsec - start_time.tv_nsec);
    elapsed_time = elapsed_ns / 1e9;

    printf("find used time: %.2f seconds\n", elapsed_time);

    clock_gettime(CLOCK_MONOTONIC, &start_time);
    destroyHashTable(table);
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    elapsed_ns = (end_time.tv_sec - start_time.tv_sec) * 1000000000L
                    + (end_time.tv_nsec - start_time.tv_nsec);
    elapsed_time = elapsed_ns / 1e9;
    
    printf("destroy used time: %.2f seconds\n", elapsed_time);

    return 0;
}