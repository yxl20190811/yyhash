#include <cstdio>
#include <time.h>
#include <map>
#include <string>
#include <string.h>

typedef struct THashNode {
    char *m_name;
    struct THashNode* m_HashNodeNext;
} THashNode;

int main()
{
    const int max = 100'000'000;

    std::map<std::string, THashNode*>*  m = new std::map<std::string, THashNode*>();

    /* 插入 */
    timespec ts1, ts2;
    clock_gettime(CLOCK_MONOTONIC, &ts1);

    char name[64];
    for (int i = 0; i < max; ++i) {
        snprintf(name, sizeof(name), "%d_%d", i, i);
        THashNode** node = &((*m)[name]);
        if(NULL == *node){
            *node = new THashNode();
            (*node)->m_name = strdup(name);
            (*node)->m_HashNodeNext = NULL;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &ts2);
    printf("insert time: %.2f s\n",
           (ts2.tv_sec - ts1.tv_sec) +
           (ts2.tv_nsec - ts1.tv_nsec) / 1e9);

    /* 查找 */
    clock_gettime(CLOCK_MONOTONIC, &ts1);
    for (int i = 0; i < max; ++i) {
        snprintf(name, sizeof(name), "%d_%d", i, i);
        auto it = m->find(name);
        if (it == m->end())
        {
            printf("not found: %s\n", name);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &ts2);

    printf("find time: %.2f s\n",
           (ts2.tv_sec - ts1.tv_sec) +
           (ts2.tv_nsec - ts1.tv_nsec) / 1e9);

    /* 释放 */
    clock_gettime(CLOCK_MONOTONIC, &ts1);
    std::map<std::string, THashNode*>::iterator it = m->begin();
    unsigned size = m->size();
    for(int i = 0; i < size; ++i,++it){
        THashNode* node = it->second;
       free(node->m_name);
       free(node);
    }
    delete m;
    clock_gettime(CLOCK_MONOTONIC, &ts2);

    printf("destroy time: %.2f s\n",
           (ts2.tv_sec - ts1.tv_sec) +
           (ts2.tv_nsec - ts1.tv_nsec) / 1e9);

    return 0;
}
