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
            __sync_lock_release(&data->m_lock);
            return cur;
        }
        cur = cur->m_HashNodeNext;
        ++count;
    }
    if(count > old_count){
        old_count = count;
        printf("冲突hashid：%u, %u\r\n", old_count, hashID);
    }
    (*node)->m_name = strdup(name);
    (*node)->m_HashNodeNext = data->m_head;
    data->m_head =  (*node) ;
    (*node) = 0;

    __sync_lock_release(&data->m_lock);

    return data->m_head;
}

THashNode* findHashTable(THashTable* table, const char* name)
{
    if (!table || !name) return NULL;

    /* 1. 计算 hash */
    unsigned int hashID = 5381;
    const char* str = name;
    int c;
    while ((c = *str++)) {
        hashID = ((hashID << 5) + hashID) + c;
    }

    /* 2. 拆分目录 / 页内偏移 */
    unsigned int pageCount  = (hashID >> DirMov) & DirMask;
    unsigned int pageOffset = hashID & PageMask;

    /* 3. 取得 page */
    THashPage* page = table->m_PageDir[pageCount];
    if (!page) return NULL;

    THashData* data = &page->m_data[pageOffset];

    /* 4. bucket 加锁 */
    //while (!__sync_bool_compare_and_swap(&data->m_lock, 0, 1));

    /* 5. 遍历链表 */
    THashNode* cur = data->m_head;
    while (cur) {
        if (strcmp(cur->m_name, name) == 0) {
            //data->m_lock = 0;
            return cur;
        }
        cur = cur->m_HashNodeNext;
    }

    /* 6. 解锁 */
    //data->m_lock = 0;
    return NULL;
}

void destroyHashTable(THashTable* table)
{
    if (!table) return;

    /* 遍历 Page Directory */
    for (unsigned i = 0; i <= DirMask; ++i) {
        THashPage* page = table->m_PageDir[i];
        if (!page) continue;

        /* 遍历 page 内所有 bucket */
        for (unsigned j = 0; j <= PageMask; ++j) {
            THashData* data = &page->m_data[j];

            /* bucket 加锁（防止并发访问） */
            while (!__sync_bool_compare_and_swap(&data->m_lock, 0, 1));

            THashNode* cur = data->m_head;
            while (cur) {
                THashNode* next = cur->m_HashNodeNext;

                if (cur->m_name)
                    free(cur->m_name);

                free(cur);
                cur = next;
            }

            data->m_head = NULL;
            data->m_lock = 0;
        }

        /* 释放 page */
        free(page);
        table->m_PageDir[i] = NULL;
    }

    /* 释放 table（包含 PageDir） */
    free(table);
}

