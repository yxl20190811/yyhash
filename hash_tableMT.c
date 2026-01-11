#include "hash_tableMT.h"
#include <malloc.h>
const unsigned v[32][2]={
    {0b0, 0b11111111111111111111111111111111},
    {0b1, 0b1111111111111111111111111111111},
    {0b11, 0b111111111111111111111111111111},
    {0b111, 0b11111111111111111111111111111},
    {0b1111, 0b1111111111111111111111111111},
    {0b11111, 0b111111111111111111111111111},
    {0b111111, 0b11111111111111111111111111},
    {0b1111111, 0b1111111111111111111111111},
    {0b11111111, 0b111111111111111111111111},
    {0b111111111, 0b11111111111111111111111},
    {0b1111111111, 0b1111111111111111111111},
    {0b11111111111, 0b111111111111111111111},
    {0b111111111111, 0b11111111111111111111},
    {0b1111111111111, 0b1111111111111111111},
    {0b11111111111111, 0b111111111111111111},
    {0b111111111111111, 0b11111111111111111},
    {0b1111111111111111, 0b1111111111111111},
    {0b11111111111111111, 0b111111111111111},
    {0b111111111111111111, 0b11111111111111},
    {0b1111111111111111111, 0b1111111111111},
    {0b11111111111111111111, 0b111111111111},
    {0b111111111111111111111, 0b11111111111},
    {0b1111111111111111111111, 0b1111111111},
    {0b11111111111111111111111, 0b111111111},
    {0b111111111111111111111111, 0b11111111},
    {0b1111111111111111111111111, 0b1111111},
    {0b11111111111111111111111111, 0b111111},
    {0b111111111111111111111111111, 0b11111},
    {0b1111111111111111111111111111, 0b1111},
    {0b11111111111111111111111111111, 0b111},
    {0b111111111111111111111111111111, 0b11},
    {0b1111111111111111111111111111111, 0b1}
    };
const unsigned DirMov = 4;
const unsigned DirMask = v[DirMov][1];
const unsigned PageMask = v[DirMov][0];

THashTable* createHashTable() {
    THashTable* tab = (THashTable*)malloc(sizeof(THashTable) + (DirMask+1) * sizeof(void*));
    tab->m_PageDir =  (THashPage**)((char*)tab+ sizeof(THashTable));
    memset(tab->m_PageDir, 0, (DirMask+1) * sizeof(void*));
    return tab;
}

THashNode* insertHashTable(THashTable* table, const char* name, THashNode** node, void** page) {
    //count hash id
    unsigned int hashID = 5381;
    {
        const char* str = name;
        int c;
        while ((c = *str++)) {
            hashID = ((hashID << 5) + hashID) + c; /* hash * 33 + c */
        }
    }

    unsigned int pageCount = (hashID >> DirMov);
    unsigned int pageOffset = hashID & PageMask;

    THashPage** pagePtr = &(table->m_PageDir[pageCount]);
    if (0  == *page) {
        *page = (THashPage*)malloc(sizeof(THashPage)+(PageMask+1)*sizeof(THashData));
        memset(*page, 0, sizeof(THashPage)+(PageMask+1)*sizeof(THashData));
    }
    void* tmp = *pagePtr;
    int r = __sync_bool_compare_and_swap(pagePtr, 0, *page);
    //if(0 == *pagePtr){*pagePtr = *page;}
    if(*pagePtr == *page){
        *page = 0;
    }
    

    THashData* data = &((*pagePtr)->m_data[pageOffset]);
    if (0 == *node) {
        *node = (THashNode*)malloc(sizeof(THashNode));
    }
    if(data->m_lock != 0){
        data->m_lock = 0;
    }
    while (!__sync_bool_compare_and_swap(&(data->m_lock), 0, 1));
    THashNode* cur = data->m_head;

    static int old_count = 0;
    int count = 0;
    while (NULL != cur) {
        if (0 == strcmp(cur->m_name, name)) {
            // Node already exists, update value and return
            data->m_lock = 0;
            return cur;
        }
        cur = cur->m_HashNodeNext;
        ++count;
    }
    if(count > old_count){
        old_count = count;
        printf("%U, %u\r\n", old_count, hashID);
    }
    (*node)->m_name = strdup(name);
    (*node)->m_HashNodeNext = data->m_head;
    (*node) = 0;

    data->m_lock = 0;

    return data->m_head;
}


