#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash_table.h"
#include <time.h>


const int max = 100000000;
int main() {
    printf("Open chain hash table demonstration program\n");
    THashNode* data = (THashNode*)malloc(max*sizeof(THashNode));
    for (int i = 0; i < max; ++i) {
        snprintf(data[i].m_name, 20, "%d_%d", i, i);
    }
    // Create hash table
    THashTable* table = createHashTable();
    if (!table) {
        printf("Failed to create hash table\n");
        return -1;
    }
    printf("createHashTable success.\n");
    clock_t start_time = clock();
    // Test insert operation
    for (int i = 0; i < max; ++i) {
        insertHashTable(table, data+i);
    }
    clock_t end_time = clock();
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("insert used time:%.2f second\n", elapsed_time);
    printf("\n--- Test find operation ---\n");
    start_time = clock();

    // Test find operation
   
    for (int i = 0; i < max; ++i) {
        THashNode* node = findHashTable(table, data[i].m_name);
        if (NULL == node) {
            printf("Not found: %s\n", data[i].m_name);
        }
    }
    end_time = clock();
    elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("find used time: %.2f second\n", elapsed_time);

    start_time = clock();
    destroyHashTable(table);
    end_time = clock();
    elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("destroy hashtabl used time: %.2f second\n", elapsed_time);

    return 0;
}