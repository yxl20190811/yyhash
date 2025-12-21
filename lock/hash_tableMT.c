#define _CRT_SECURE_NO_WARNINGS
#include "hash_tableMT.h"

const unsigned DirMask = 0xFFFFFFF;
const unsigned DirMov = 4;
const unsigned PageMask = 0xF;
// Create hash table
THashTable* createHashTable() {
    THashTable* table = (THashTable*)malloc(sizeof(THashTable));
    if (!table) {
        return NULL;
    }
    
    // Initialize page directory, supports up to 2^12 = 4096 pages (using high 12 bits as page index)
    table->PageDir = (THashNode***)malloc((DirMask +1)*sizeof(void*));
    if (!table->PageDir) {
        free(table);
        return NULL;
    }
    memset(table->PageDir, 0, (DirMask + 1) * sizeof(void*));
    pthread_mutex_init(&(table->pageCreationLock), NULL);
    return table;
}

// Destroy hash table
void destroyHashTable(THashTable* table) {
    if (!table) return;
    pthread_mutex_destroy(&(table->pageCreationLock));

    if (table->PageDir) {
        // Traverse all pages and free memory
        for (int i = 0; i < DirMask; i++) {
            if (table->PageDir[i]) {
                THashNode** page = table->PageDir[i];
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
THashNode* insertHashTable(THashTable* table, THashNode* newNode) {
    if (!table || !newNode || !newNode->m_name) return NULL;
    
    unsigned int hashID = hashString(newNode->m_name);
    
    // According to original document logic: pageCount = hashID >> 20; pageOffset = hashID & 0xFFF; (low 12 bits)
    // Changed to use high 20 bits as page index, low 12 bits as page offset, so each page has only 4096 slots
    int pageCount = (hashID >> DirMov);  // Use high 20 bits as page index
    
    unsigned int pageOffset = hashID & PageMask;   // Use low 12 bits as page offset
    
    // Limit page index within valid range
    
    // Get page pointer
    THashNode*** pagePtr = &(table->PageDir[pageCount]);
    bool isLock = false;
    // If page is empty, create a new page
    if (NULL == *pagePtr) {
        isLock = true;
        pthread_mutex_lock(&(table->pageCreationLock));
        if (NULL == *pagePtr) {
            THashNode** tmp = (THashNode**)malloc((PageMask +1) * sizeof(THashNode*));  // 2^12=4096 slots per page
            if (tmp) {
                pthread_mutex_unlock(&(table->pageCreationLock));
                return NULL;
            }
            memset(tmp, 0, (PageMask + 1) * sizeof(THashNode*));
            *pagePtr = tmp;
        }
    }
    
    // Get node slot pointer
    THashNode** node = &((*pagePtr)[pageOffset]);
    if(!isLock){
        pthread_mutex_lock(&(table->pageCreationLock));
    }
    // Check if a node with the same name already exists
    THashNode* cur = *node;
    while (NULL != cur) {
        if (0 == strcmp(cur->m_name, newNode->m_name)) {
            // Node already exists, update value and return
            return cur;
        }
        cur = cur->m_HashNodeNext;
    }
    
    // Create new node
    newNode->m_HashNodeNext = *node;
    *node = newNode;
    pthread_mutex_unlock(&(table->pageCreationLock));
    return newNode;
}

// Find in hash table
THashNode* findHashTable(THashTable* table, const char* name) {
    if (!table || !name) return NULL;
    
    unsigned int hashID = hashString(name);
    
    // According to original document logic: pageCount = hashID >> 20; pageOffset = hashID & 0xFFF; (low 12 bits)
    int pageCount = (hashID >> DirMov);  // Use high 20 bits as page index
    unsigned int pageOffset = hashID & PageMask;   // Use low 12 bits as page offset
    
    
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
    int pageCount = (hashID >> DirMov);  // Use high 20 bits as page index
    unsigned int pageOffset = hashID & PageMask;   // Use low 12 bits as page offset
    


    // Get page pointer
    THashNode** page = table->PageDir[pageCount];
    
    // If page does not exist, there is no corresponding node
    if (NULL == page) {
        return 0;
    }
    
    // Get node slot pointer
    THashNode** node = &(page[pageOffset]);
    
    // Traverse linked list to find the node to delete
    THashNode* prev = NULL;
    pthread_mutex_lock(&(table->pageCreationLock));
    THashNode* cur = *node;
    while (cur) {
        if (0 == strcmp(cur->m_name, name)) {
            if(prev){
                prev->m_HashNodeNext = cur->m_HashNodeNext;
            }
            else{
                *node = cur->m_HashNodeNext;
            }
            pthread_mutex_unlock(&(table->pageCreationLock));
            return 1; // Successfully deleted
        }
        prev = cur;
        cur = cur->m_HashNodeNext;
    }
    pthread_mutex_unlock(&(table->pageCreationLock));
    return 0; // No node to delete found
}