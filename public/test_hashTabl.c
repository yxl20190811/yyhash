#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#else
#include <sys/time.h>
#endif

#include "THashTbl.h"

#define NAME_LEN 20

static double get_time_ms(void)
{
#ifdef _WIN32
    LARGE_INTEGER freq, cnt;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&cnt);
    return (double)cnt.QuadPart / freq.QuadPart * 1000.0;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
#endif
}

static long get_mem_kb(void)
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return (long)(pmc.WorkingSetSize / 1024);
    return 0;
#else
    long rss = 0;
    FILE* fp = fopen("/proc/self/status", "r");
    if (fp) {
        char buf[256];
        while (fgets(buf, sizeof(buf), fp)) {
            if (strncmp(buf, "VmRSS:", 6) == 0) {
                sscanf(buf + 6, "%ld", &rss);
                break;
            }
        }
        fclose(fp);
    }
    return rss;
#endif
}

int main(int argc, char* argv[])
{
    double t0, t1;
    long mem0, mem1;

    mem0 = get_mem_kb();
   

    int N = 10;
    if(argc > 1){
        N = atoi(argv[1]);
    }
    if(N < 10 || N > 1000000000){
        N = 10;
    }

    int i, id, err_cnt;
    char (*names)[NAME_LEN];
    THashTab* tbl;


    printf("=== Hash Table Test : N = %d ===\n\n", N);

    /* --- allocate names --- */
    names = (char(*)[NAME_LEN])malloc((size_t)N * NAME_LEN);
    if (!names) {
        fprintf(stderr, "malloc names failed\n");
        return 1;
    }
    t0 = get_time_ms();
    for (i = 0; i < N; i++)
        sprintf(names[i], "node_%d", i);
    t1 = get_time_ms();
    mem1 = get_mem_kb();
    printf("[init node name ] time: %8.2f ms  mem: +%ld KB\n", t1 - t0, mem1 - mem0);
    /* --- create --- */
    

    t0 = get_time_ms();
    tbl = createHashTbl(N);
    t1 = get_time_ms();
    mem1 = get_mem_kb();
    printf("[create ] time: %8.2f ms  mem: +%ld KB\n", t1 - t0, mem1 - mem0);
    if (!tbl) {
        fprintf(stderr, "createHashTbl failed\n");
        free(names);
        return 1;
    }

    /* --- insert --- */
    //mem0 = get_mem_kb();
    t0 = get_time_ms();

    char buf[100];
    for (i = 0; i < N; i++) {
        sprintf(buf, "node_%d", i);
        id = insertHashTbl(tbl, buf);
        if (id != i+1) {
            fprintf(stderr, "insertHashTbl id mismatch: i=%d got id=%d\n", i, id);
            break;
        }
    }

    t1 = get_time_ms();
    mem1 = get_mem_kb();
    printf("[insert ] time: %8.2f ms  mem: +%ld KB  total: %d\n",
           t1 - t0, mem1 - mem0, i);

    //重复插入测试
    t0 = get_time_ms();
    for (i = 0; i < N; i++) {
        sprintf(buf, "node_%d", i);
        id = insertHashTbl(tbl, buf);
        if (id != i+1) {
            fprintf(stderr, "insertHashTbl id mismatch: i=%d got id=%d\n", i, id);
        }
    }
    t1 = get_time_ms();
    mem1 = get_mem_kb();
    printf("[mult insert ] time: %8.2f ms  mem: +%ld KB  total: %d\n",
           t1 - t0, mem1 - mem0, i);

    /* --- find --- */
    err_cnt = 0;
    t0 = get_time_ms();

    for (i = 0; i < N; i++) {
        sprintf(buf, "node_%d", i);
        id = findHashTbl(tbl, buf);
        if (id != i+1) {
            if (err_cnt < 10)
                fprintf(stderr, "  findHashTbl NOT FOUND: %s\n", names[i]);
            err_cnt++;
        }
    }

    t1 = get_time_ms();
    printf("[find   ] time: %8.2f ms  errors = %d, total = %d\n",
           t1 - t0, err_cnt, N);

    /* --- destroy --- */
    //mem0 = get_mem_kb();
    t0 = get_time_ms();
    destroyHashTbl(tbl);
    free(names);

    t1 = get_time_ms();
    mem1 = get_mem_kb();
    printf("[destroy] time: %8.2f ms  mem: +%ld KB\n", t1 - t0, mem1 - mem0);

   
    printf("\ndone.\n");
    return 0;
}
