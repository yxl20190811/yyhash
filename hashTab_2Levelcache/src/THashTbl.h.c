#define _CRT_SECURE_NO_WARNINGS
#include "../../public/THashTbl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 常量定义 - 参考 hash_table.c */
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
const unsigned DirMov = 10; 			//右移4位，取高28位 
const unsigned DirMask = v[DirMov][1];     //28位页目录掩码 
const unsigned PageMask = v[DirMov][0];   //4位页内偏移掩码，每页16个槽 

    

/* 哈希节点结构 */
typedef struct THashNode {
    char* m_name;                       /* 动态分配的字符串键 */
    int   m_id;                         /* 顺序分配的ID (>=1) */
    struct THashNode* m_HashNodeNext;   /* 链表下一个节点 */
} THashNode;

/* 哈希表结构体定义（原 opaque 类型） */
struct THashTab {
    THashNode*** PageDir;   /* 页目录：指向页的指针数组 */
    int nextId;             /* 下一个分配的ID，从1开始递增 */
};

/* 字符串哈希函数 - djb2 算法 */
static unsigned int hashString(const char* str) {
    unsigned int hash = 5381;
    int c;
    
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    
    return hash;
}

/* 创建哈希表 */
THashTab* createHashTbl(int size) {
    (void)size; /* size参数保留但不使用，采用动态页分配策略 */
    
    THashTab* tbl = (THashTab*)malloc(sizeof(THashTab));
    if (!tbl) {
        return NULL;
    }
    
    /* 初始化页目录，支持 2^28 个页 */
    tbl->PageDir = (THashNode***)malloc((DirMask + 1) * sizeof(void*));
    if (!tbl->PageDir) {
        free(tbl);
        return NULL;
    }
    
    memset(tbl->PageDir, 0, (DirMask + 1) * sizeof(void*));
    tbl->nextId = 1; /* ID从1开始 */
    
    return tbl;
}

/* 插入键到哈希表
 * 新键 -> 返回 id (>= 1)，id 按顺序分配 (1, 2, 3, ...)
 * 重复键 -> 返回 -(existing_id) - 1 (< 0)
 */
int insertHashTbl(THashTab* tbl, const char* name) {
    if (!tbl || !name) return 0;
    
    unsigned int hashID = hashString(name);
    
    /* 二级页表索引计算 */
    unsigned pageCount = (hashID >> DirMov);      /* 高28位作为页目录索引 */
    unsigned int pageOffset = hashID & PageMask;  /* 低4位作为页内偏移 */
    
    /* 获取页指针 */
    THashNode*** pagePtr = &(tbl->PageDir[pageCount]);
    
    /* 延迟分配：如果页不存在则创建 */
    if (NULL == *pagePtr) {
        *pagePtr = (THashNode**)malloc((PageMask + 1) * sizeof(THashNode*));
        if (!*pagePtr) {
            return 0; /* 内存分配失败 */
        }
        memset(*pagePtr, 0, (PageMask + 1) * sizeof(THashNode*));
    }
    
    /* 获取槽位指针 */
    THashNode** slot = &((*pagePtr)[pageOffset]);
    
    /* 检查是否已存在相同键 */
    THashNode* cur = *slot;
    while (NULL != cur) {
        if (0 == strcmp(cur->m_name, name)) {
            /* 找到重复键，返回负值编码：-(existing_id) - 1 */
            return cur->m_id;
        }
        cur = cur->m_HashNodeNext;
    }
    
    /* 创建新节点 */
    THashNode* newNode = (THashNode*)malloc(sizeof(THashNode));
    if (!newNode) {
        return 0; /* 内存分配失败 */
    }
    
    /* 复制字符串键 */
    newNode->m_name = (char*)malloc(strlen(name) + 1);
    if (!newNode->m_name) {
        free(newNode);
        return 0;
    }
    strcpy(newNode->m_name, name);
    
    /* 分配顺序ID */
    newNode->m_id = tbl->nextId++;
    
    /* 头插法插入链表 */
    newNode->m_HashNodeNext = *slot;
    *slot = newNode;
    
    return newNode->m_id;
}

/* 查找键对应的ID
 * 找到 -> 返回 id (>= 1)
 * 未找到 -> 返回 0
 */
int findHashTbl(THashTab* tbl, const char* name) {
    if (!tbl || !name) return 0;
    
    unsigned int hashID = hashString(name);
    
    /* 二级页表索引计算 */
    int pageCount = (hashID >> DirMov);
    unsigned int pageOffset = hashID & PageMask;
    
    /* 获取页 */
    THashNode** page = tbl->PageDir[pageCount];
    
    /* 页不存在，键肯定不存在 */
    if (NULL == page) {
        return 0;
    }
    
    /* 遍历槽位链表查找 */
    THashNode* node = page[pageOffset];
    while (NULL != node) {
        if (0 == strcmp(node->m_name, name)) {
            return node->m_id; /* 找到 */
        }
        node = node->m_HashNodeNext;
    }
    
    return 0; /* 未找到 */
}

/* 销毁哈希表，释放所有内存 */
void destroyHashTbl(THashTab* tbl) {
    if (!tbl) return;
    
    if (tbl->PageDir) {
        /* 遍历所有页 */
        for (unsigned int i = 0; i <= DirMask; i++) {
            if (tbl->PageDir[i]) {
                THashNode** page = tbl->PageDir[i];
                
                /* 遍历页内所有槽位，释放链表节点 */
                for (unsigned int j = 0; j <= PageMask; j++) {
                    THashNode* node = page[j];
                    while (node) {
                        THashNode* toDelete = node;
                        node = node->m_HashNodeNext;
                        
                        /* 释放节点内存 */
                        if (toDelete->m_name) {
                            free(toDelete->m_name);
                        }
                        free(toDelete);
                    }
                }
                
                /* 释放页本身 */
                free(page);
            }
        }
        
        /* 释放页目录 */
        free(tbl->PageDir);
    }
    
    /* 释放表结构 */
    free(tbl);
}