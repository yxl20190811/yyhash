#define _CRT_SECURE_NO_WARNINGS
#include "hash_table.h"

// Create hash table
THashTable* createHashTable() {
    THashTable* table = (THashTable*)malloc(sizeof(THashTable));
    if (!table) {
        return NULL;
    }
    
    // Initialize page directory, supports up to 2^12 = 4096 pages (using high 12 bits as page index)
    table->PageDir = (THashNode***)malloc((0xFFFFF+1)*sizeof(void*));
    if (!table->PageDir) {
        free(table);
        return NULL;
    }
    memset(table->PageDir, 0, (0xFFFFF+1) * sizeof(void*));
    return table;
}

// Destroy hash table
void destroyHashTable(THashTable* table) {
    if (!table) return;
    
    if (table->PageDir) {
        // Traverse all pages and free memory
        for (int i = 0; i < (0xFFFFF+1); i++) {  // 0 到 0xFFFFF 包含 = (0xFFFFF+1) 个页面
            if (NULL != table->PageDir[i]) {
                THashNode** page = table->PageDir[i];
                
                // Traverse each slot in the page
                for (int j = 0; j < (0xFFF+1); j++) {  // 0 到 0xFFF slots per pag
                    THashNode* cur = page[j];
                    while (cur) {
                        THashNode* temp = cur;
                        cur = cur->m_HashNodeNext;
                        free(temp);
                    }
                }
                
                // Free the page itself
                free(table->PageDir[i]);
            }
        }
        
        free(table->PageDir);
    }
    
    free(table);
}

// String hash function
unsigned int hashString(const char* str) {
    unsigned int hash = 5381;
    int c;
    
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    
    return hash;
}

// Insert or update hash table item
THashNode* insertHashTable(THashTable* table, const char* name) {
    if (!table || !name) return NULL;
    
    unsigned int hashID = hashString(name);
    
    // According to original document logic: pageCount = hashID >> 20; pageOffset = hashID & 0xFFF; (low 12 bits)
    // Changed to use high 20 bits as page index, low 12 bits as page offset, so each page has only 4096 slots
    int pageCount = (hashID >> 12);  // Use high 20 bits as page index
    unsigned int pageOffset = hashID & 0xFFF;   // Use low 12 bits as page offset
    
    // Limit page index within valid range
    pageCount = pageCount & 0xFFFFF;  // Ensure not exceeding 20 bits
    
    // Get page pointer
    THashNode*** pagePtr = &(table->PageDir[pageCount]);
    
    // If page is empty, create a new page
    if (NULL == *pagePtr) {
        *pagePtr = (THashNode**)malloc((0xFFF+1) * sizeof(THashNode*));  // 每页 2^12=4096 个槽位
        if (!*pagePtr) {
            return NULL;
        }
        memset(*pagePtr, 0, (0xFFF+1) * sizeof(THashNode*));
    }
    
    // Get node slot pointer
    THashNode** node = &((*pagePtr)[pageOffset]);
    
    // Check if a node with the same name already exists
    THashNode* cur = *node;
    while (NULL != cur) {
        if (0 == strcmp(cur->m_name, name)) {
            // Node already exists, update value and return
            return cur;
        }
        cur = cur->m_HashNodeNext;
    }
    
    // Create new node
    THashNode* newNode = (THashNode*)malloc(sizeof(THashNode)+ strlen(name) + 1);
    if (!newNode) {
        return NULL;
    }

    newNode->m_name = (char*)newNode + sizeof(THashNode);
    strcpy(newNode->m_name, name);
    newNode->m_HashNodeNext = *node;
    *node = newNode;

    return newNode;
}

// Find in hash table
THashNode* findHashTable(THashTable* table, const char* name) {
    if (!table || !name) return NULL;
    
    unsigned int hashID = hashString(name);
    
    // According to original document logic: pageCount = hashID >> 20; pageOffset = hashID & 0xFFF; (low 12 bits)
    int pageCount = (hashID >> 12);  // Use high 20 bits as page index
    unsigned int pageOffset = hashID & 0xFFF;   // Use low 12 bits as page offset
    
    // Limit page index within valid range
    pageCount = pageCount & 0xFFFFF;  // Ensure not exceeding 20 bits
    
    
    // Get page pointer
    THashNode** page = table->PageDir[pageCount];
    
    // If page does not exist, there is no corresponding node
    if (NULL == page) {
        return NULL;
    }
    
    // Get node slot pointer
    THashNode* node = page[pageOffset];
    
    // Traverse linked list to find matching node
    while (NULL != node) {
        if (0 == strcmp(node->m_name, name)) {
            return node; // Found matching node
        }
        node = node->m_HashNodeNext;
    }
    
    return NULL; // No matching node found
}

// Remove element from hash table
int removeHashTable(THashTable* table, const char* name) {
    if (!table || !name) return 0;
    
    unsigned int hashID = hashString(name);
    
    // According to original document logic: pageCount = hashID >> 20; pageOffset = hashID & 0xFFF; (low 12 bits)
    int pageCount = (hashID >> 12);  // Use high 20 bits as page index
    unsigned int pageOffset = hashID & 0xFFF;   // Use low 12 bits as page offset
    
    // Limit page index within valid range
    pageCount = pageCount & 0xFFFFF;  // Ensure not exceeding 20 bits
    

    // Get page pointer
    THashNode** page = table->PageDir[pageCount];
    
    // If page does not exist, there is no corresponding node
    if (NULL == page) {
        return 0;
    }
    
    // Get node slot pointer
    THashNode** node = &(page[pageOffset]);
    
    // Traverse linked list to find the node to delete
    while (NULL != *node) {
        if (0 == strcmp((*node)->m_name, name)) {
            THashNode* toDelete = *node;
            *node = (*node)->m_HashNodeNext; // Bypass the node to delete
            
            // Free node memory
            if (toDelete->m_name) {
                free(toDelete->m_name);
            }
            free(toDelete);
            return 1; // Successfully deleted
        }
        node = &((*node)->m_HashNodeNext);
    }
    
    return 0; // No node to delete found
}