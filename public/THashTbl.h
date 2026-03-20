#ifndef THASHTBL_H
#define THASHTBL_H

#ifdef __cplusplus
extern "C" {
#endif

/* 不透明结构体前向声明，具体内容在 THashTbl.c 中定义 */
typedef struct THashTab THashTab;

THashTab* createHashTbl(int size);

/*
 * insertHashTbl:
 *   new key   -> return id (>= 1), id = sequential (1, 2, 3, ...)
 *   duplicate -> return -(existing_id) - 1  (< 0)
 */
int   insertHashTbl(THashTab* tbl, const char* name);
int   findHashTbl(THashTab* tbl, const char* name);
void  destroyHashTbl(THashTab* tbl);

#ifdef __cplusplus
}
#endif

#endif /* THASHTBL_H */
