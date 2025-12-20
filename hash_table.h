#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define hash node structure
typedef struct THashNode {
    char m_name[20];                      // Stored string key
    struct THashNode* m_HashNodeNext;   // Next node pointer in linked list
} THashNode;


// Define hash table structure
typedef struct {
    THashNode*** PageDir;                   // Page directory, stores pointers to each page
} THashTable;

// Hash table operation function declarations
THashTable* createHashTable();
void destroyHashTable(THashTable* table);
unsigned int hashString(const char* str);
THashNode* insertHashTable(THashTable* table, THashNode* node);
THashNode* findHashTable(THashTable* table, const char* name);
int removeHashTable(THashTable* table, const char* name);

#endif // HASH_TABLE_H