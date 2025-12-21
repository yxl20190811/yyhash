#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash_tableMT.h"
#include <time.h>
#include <pthread.h>

THashNode* data = NULL;
const int num_threads = 100;          // 线程数
const int max = 100000000;  // 总条目数（为了测试先减少）
const int batch = max/num_threads;
THashTable* table = NULL;

// 线程工作函数 - 执行插入操作
void* insert_worker(void* arg) {
    unsigned* start1 = (unsigned*)&arg;
    unsigned start = *start1;
    //printf("Thread %d-%d: begin inserting\n", start,start+batch);
    void* page = NULL;
    for (unsigned i = 0; i < batch; i++) {
        insertHashTable(table,data + i + start, &page);
        //printf("\r\n%d",i);
    }
    if(NULL != page){
        free(page);
    }
    //printf("Thread %d-%d: Finished inserting\n", start,start+batch);
    return NULL;
}
void* find_worker(void* arg) {
    unsigned* start1 = (unsigned*)&arg;
    unsigned start = *start1;
    for (int i = 0; i < batch; i++) {
        findHashTable(table, data[i + start].m_name);
    }
    
    //printf("Thread %d-%d: Finished inserting\n", start,start+batch);
    return NULL;
}

int main() {
    // 创建数据
    data = (THashNode*)malloc(max*sizeof(THashNode));
    for (int i = 0; i < max; i++) {
        snprintf(data[i].m_name, 20, "key_%d_%d", i, i);
    }
    
    // 创建哈希表
    table = createHashTable();
    if (!table) {
        printf("Failed to create hash table\n");
        return -1;
    }
    printf("createHashTable success.\n");
    
    // 创建线程数组
    pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));

    clock_t start_time, end_time;
    double elapsed_time;
    
    printf("\n--- Multi-threaded Insert Operation ---\n");
    start_time = clock();
    
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
    end_time = clock();
    elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Multi-threaded insert used time: %.2f seconds\n", elapsed_time);
    
 
    start_time = clock();
    
    // 启动查找线程
    for (int i = 0; i < num_threads; i++) {
        int ret = pthread_create(&threads[i], NULL, find_worker, (void*)(i*batch));
        if (ret) {
            printf("Error creating thread %d\n", i);
            return -1;
        }
    }
    
    // 等待所有查找线程完成
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    end_time = clock();
    elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Multi-threaded find used time: %.2f seconds\n", elapsed_time);
    
    // 清理资源
    free(data);
    free(threads);


     start_time = clock();
    destroyHashTable(table);
     end_time = clock();
    elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Multi-threaded  destroyHashTable used time: %.2f seconds\n", elapsed_time);

    printf("\nAll operations completed successfully.\n");
    
    return 0;
}