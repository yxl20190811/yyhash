#pragma once
#include <malloc.h>
#include <memory.h>
extern const  unsigned DirMask;
extern const  unsigned DirMov;
extern const  unsigned PageMask;

// Define hash node structure
typedef struct THashNode {
    char *m_name;    
    struct THashNode* m_HashNodeNext;   // Next node pointer in linked list
} THashNode;

typedef struct {
    THashNode* m_head;
    int m_lock;
}THashData;

typedef struct {
    THashData m_data[0];
}THashPage;

// Define hash table structure
typedef struct {
    THashPage** m_PageDir;                   // Page directory, stores pointers to each page
} THashTable;

THashTable* createHashTable();

THashNode* insertHashTable(THashTable* table, const char* name, THashNode** node, void** page) ;
THashNode* findHashTable(THashTable* table, const char* name);
void destroyHashTable(THashTable* table);