#include <stdio.h>
#include <map>
#include <memory.h>
#include <malloc.h>
#include <time.h>

typedef struct THashNode {
    char m_name[20];                      // Stored string key
    struct THashNode* m_HashNodeNext;   // Next node pointer in linked list
} THashNode;

const int max = 100000000;

int main() {
    printf("Open chain hash table demonstration program\n");
    THashNode* data = (THashNode*)malloc(max*sizeof(THashNode));
    for (int i = 0; i < max; ++i) {
        snprintf(data[i].m_name, 20, "%d_%d", i, i);
    }
    clock_t start_time = clock();
    std::map<const char*, void*> map;
    for (int i = 0; i < max; ++i) {
        map[(const char*)(data[i].m_name)] = (void*)(data + i);
    }
    clock_t end_time = clock();
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("insert used time:%.2f second\n", elapsed_time);

    start_time = clock();
    for (int i = 0; i < max; ++i) {
        void* obj = map[(const char*)(data[i].m_name)];
    }
    end_time = clock();
    elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("find used time:%.2f second\n", elapsed_time);
}