#include "../../public/THashTbl.h"
#include "../../public/uthash.h"
#include <stdlib.h>
#include <string.h>
#include "../../public/TMemoryPool.h"
/* 哈希表节点结构 - uthash要求包含UT_hash_handle字段 */
typedef struct HashNode {
    char* key;           /* 动态分配的键副本 */
    int id;              /* 顺序分配的ID (1,2,3,...) */
    UT_hash_handle hh;   /* uthash必须的句柄，必须命名为hh */
} HashNode;

/* THashTab结构体定义 - 对应头文件中的前向声明 */
struct THashTab {
    HashNode* head;      /* uthash表头指针 */
    int next_id;         /* 下一个要分配的ID */
    int size_hint;       /* 初始大小提示（用于预分配） */
    TMemoryPool m_pool;
};

/**
 * 创建哈希表
 * @param size 预估元素数量（用于优化，uthash会自动扩容）
 * @return 哈希表指针，失败返回NULL
 */
THashTab* createHashTbl(int size)
{
    THashTab* tbl = (THashTab*)malloc(sizeof(THashTab));
    if (!tbl) {
        return NULL;
    }
    createMemoryPool(&(tbl->m_pool), 1000*1000*100);
    tbl->head = NULL;       /* uthash要求初始化为NULL */
    tbl->next_id = 1;       /* ID从1开始 */
    tbl->size_hint = size > 0 ? size : 0;
    
    return tbl;
}

/**
 * 插入键到哈希表
 * @param tbl 哈希表
 * @param name 要插入的键（字符串）
 * @return 新键返回正ID(1,2,3...)，重复键返回 -(existing_id) - 1
 */
int insertHashTbl(THashTab* tbl, const char* name)
{
    HashNode* node = NULL;
    
    if (!tbl || !name) {
        return 0;
    }
    
    /* 先查找是否已存在 - 使用uthash的HASH_FIND_STR宏 */
    HASH_FIND_STR(tbl->head, name, node);
    
    if (node != NULL) {
        /* 已存在，返回编码的负值: -(existing_id) - 1 */
        return node->id;
    }
    
    /* 不存在，创建新节点 */
    node = (HashNode*)malloc(sizeof(HashNode));
    if (!node) {
        return 0;  /* 内存分配失败 */
    }
    
    /* 复制键字符串（uthash只存储指针，不复制内容） */
    node->key = strdupFromMemoryPool(&(tbl->m_pool), name);
    if (!node->key) {
        free(node);
        return 0;
    }
    
    /* 分配顺序ID */
    node->id = tbl->next_id;
    tbl->next_id++;
    
    /* 添加到uthash表 - 使用HASH_ADD_STR自动处理字符串key */
    HASH_ADD_STR(tbl->head, key, node);
    
    return node->id;
}

/**
 * 在哈希表中查找键
 * @param tbl 哈希表
 * @param name 要查找的键
 * @return 找到返回正ID，未找到返回0
 */
int findHashTbl(THashTab* tbl, const char* name)
{
    HashNode* node = NULL;
    
    if (!tbl || !name) {
        return 0;
    }
    
    /* 使用uthash查找 */
    HASH_FIND_STR(tbl->head, name, node);
    
    if (node != NULL) {
        return node->id;
    }
    
    return 0;  /* 未找到 */
}

/**
 * 销毁哈希表，释放所有内存
 * @param tbl 哈希表
 */
void destroyHashTbl(THashTab* tbl)
{
    HashNode* current = NULL;
    HashNode* tmp = NULL;
    
    if (!tbl) {
        return;
    }
    
    /* 使用uthash的迭代宏遍历并删除所有节点 */
    HASH_ITER(hh, tbl->head, current, tmp) {
        /* 从哈希表中删除 */
        HASH_DEL(tbl->head, current);
        /* 释放键的内存（strdup分配的） */
        //free(current->key);
        /* 释放节点内存 */
        free(current);
    }
    destroyMemoryPool(&(tbl->m_pool));
    /* 释放表结构本身 */
    free(tbl);
}