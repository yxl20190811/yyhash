#ifndef __TMEMORY_POOL_H__
#define __TMEMORY_POOL_H__

#include <stdlib.h>
#include <string.h>

typedef struct TMemoryPoolBlock {
    struct TMemoryPoolBlock* m_next;
    char   m_buf[1];
}TMemoryPoolBlock;

typedef struct TMemoryPool{
    size_t m_block_size;   // 默认大块大小
    TMemoryPoolBlock* m_head;      // 链表头
    char* m_current;       // 当前可分配地址
    size_t m_remain;       // 当前块剩余大小

}TMemoryPool;

void createMemoryPool(TMemoryPool* pool, size_t size);
void destroyMemoryPool(TMemoryPool* pool);
char* mallocFromMemoryPool(TMemoryPool* pool, size_t size);
char* strdupFromMemoryPool(TMemoryPool* pool, const char* text);
#endif //__TMEMORY_POOL_H__