#include "TMemoryPool.h"
#define ALIGN(x) (((x) + ((sizeof(void*))-1)) & ~((size_t)((sizeof(void*))-1)))

void createMemoryPool(TMemoryPool* pool, size_t size) {
	pool->m_block_size = size;
	pool->m_head = NULL;
	pool->m_remain = 0;
	pool->m_current = NULL;
}
void destroyMemoryPool(TMemoryPool* pool) {
	TMemoryPoolBlock* cur = pool->m_head;
	TMemoryPoolBlock* tmp = NULL;
	while (NULL != cur) {
		tmp = cur;
		cur = cur->m_next;
		free(tmp);
	}
	pool->m_head = NULL;
	pool->m_current = NULL;
	pool->m_remain = 0;
}
char* mallocFromMemoryPool(TMemoryPool* pool, size_t size) {
	size = ALIGN(size);
	if (size > pool->m_block_size) {
		TMemoryPoolBlock* b = (TMemoryPoolBlock*)malloc(size+sizeof(TMemoryPoolBlock));
		if (NULL == b) { return NULL; }
		b->m_next = pool->m_head;
		pool->m_head = b;
		return b->m_buf;
	}
	else if (size > pool->m_remain) {
		TMemoryPoolBlock* b = (TMemoryPoolBlock*)malloc(pool->m_block_size + sizeof(TMemoryPoolBlock));
		if (NULL == b) { return NULL; }
		b->m_next = pool->m_head;
		pool->m_head = b;
		pool->m_current = b->m_buf;
		pool->m_remain = pool->m_block_size;
	}
	pool->m_remain -= size;
	char* ret = pool->m_current;
	pool->m_current += size;
	return ret;
}

char* strdupFromMemoryPool(TMemoryPool* pool, const char* text) {
	size_t len = strlen(text) + 1;
	char* buf = mallocFromMemoryPool(pool, len);
	memcpy(buf, text, len);
	return buf;
}