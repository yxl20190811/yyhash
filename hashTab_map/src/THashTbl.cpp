#include "../../public/THashTbl.h"
#include "../../public/TMemoryPool.h"
#include <map>
#include <cstring>
#include "../../public/TMyAllocator.h"


struct CStrLess {
	bool operator()(const char* a, const char* b) const {
		return strcmp(a, b) < 0;
	}
};

typedef std::map<char*, int, CStrLess, MyAllocator<std::pair<char* const, int>>>  TMap;
typedef struct THashTab{
    TMap* m_map;
    TMemoryPool m_pool;
    int m_next_id;
}THashTab;

/* ================================================================
 *  C API 实现
 * ================================================================ */
extern "C" {

THashTab* createHashTbl(int size) {
    THashTab* tbl = new THashTab();
    createMemoryPool(&tbl->m_pool, size);
    tbl->m_map = new TMap();
    tbl->m_next_id = 0;
    return tbl;
}

int insertHashTbl(THashTab* tbl, const char* name) {
    std::pair<TMap::iterator, bool> ret =
        tbl->m_map->insert(std::make_pair((char*)name, 0));
    if(ret.second){
        char* name_copy = strdupFromMemoryPool(&tbl->m_pool,  name);
        char*const* tmp = &(ret.first->first);
        *(char**)tmp = name_copy;
        ret.first->second = ++(tbl->m_next_id);
    }
    return ret.first->second;
}

int findHashTbl(THashTab* tbl, const char* name) {
    TMap::iterator it = tbl->m_map->find((char*)name);
    if (it != tbl->m_map->end())
        return it->second;
    return 0;
}

void destroyHashTbl(THashTab* tbl) {
    if (!tbl) return;
    destroyMemoryPool(&tbl->m_pool);
    delete tbl->m_map;
    delete tbl;
}

}  //extern "C" 
