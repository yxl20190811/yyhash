/*
 *  SwissTable hash table implementation (portable C with optional SSE2)
 */
#include "../../public/THashTbl.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ---- platform compat ---- */
#ifdef _MSC_VER
  #include <intrin.h>
  #define my_strdup  _strdup
  static __forceinline int my_ctz(uint32_t x)
  { unsigned long i; _BitScanForward(&i, x); return (int)i; }
#else
  #define my_strdup  strdup
  static inline int my_ctz(uint32_t x) { return __builtin_ctz(x); }
#endif

/* ---- SSE2 detection ---- */
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
  #define HT_SSE2 1
  #include <emmintrin.h>
#endif

/* ---- constants ---- */
#define GW          16          /* group width (matches SSE register) */
#define CTRL_EMPTY  ((uint8_t)0x80)
#define CTRL_DEL    ((uint8_t)0xFE)
#define LOAD_NUM    7           /* max load factor = 7/8 = 87.5 % */
#define LOAD_DEN    8

/* ---- types ---- */
typedef struct { char* key; int id; } Slot;

/* 具体实现结构体，与头文件中的前向声明 struct THashTab 对应 */
struct THashTab {
    uint8_t*  ctrl;         /* control-byte array  [cap + GW] */
    Slot*     slots;        /* parallel slot array [cap]      */
    uint32_t  cap;          /* always power-of-2              */
    uint32_t  size;
    int       next_id;
};

/* ---- FNV-1a 64-bit hash ---- */
static uint64_t hash_str(const char* s)
{
    uint64_t h = 0xcbf29ce484222325ULL;
    for (; *s; s++) {
        h ^= (uint8_t)*s;
        h *= 0x100000001b3ULL;
    }
    return h;
}

#define H1(h)  ((uint32_t)((h) >> 7))
#define H2(h)  ((uint8_t)((h) & 0x7F))

/* ======== group operations ======== */
#ifdef HT_SSE2

static uint32_t grp_match_h2(const uint8_t* g, uint8_t h2)
{
    __m128i v = _mm_loadu_si128((const __m128i*)g);
    return (uint32_t)_mm_movemask_epi8(_mm_cmpeq_epi8(v, _mm_set1_epi8((char)h2)));
}
static uint32_t grp_match_empty(const uint8_t* g)
{
    __m128i v = _mm_loadu_si128((const __m128i*)g);
    return (uint32_t)_mm_movemask_epi8(
               _mm_cmpeq_epi8(v, _mm_set1_epi8((char)CTRL_EMPTY)));
}
static uint32_t grp_match_avail(const uint8_t* g)
{
    /* EMPTY(0x80) and DEL(0xFE) both have MSB=1; valid H2 is 0x00-0x7F */
    return (uint32_t)_mm_movemask_epi8(_mm_loadu_si128((const __m128i*)g));
}

#else /* portable fallback */

static uint32_t grp_match_h2(const uint8_t* g, uint8_t h2)
{ uint32_t m=0; int i; for(i=0;i<GW;i++) if(g[i]==h2) m|=1u<<i; return m; }
static uint32_t grp_match_empty(const uint8_t* g)
{ uint32_t m=0; int i; for(i=0;i<GW;i++) if(g[i]==CTRL_EMPTY) m|=1u<<i; return m; }
static uint32_t grp_match_avail(const uint8_t* g)
{ uint32_t m=0; int i; for(i=0;i<GW;i++) if(g[i]&0x80) m|=1u<<i; return m; }

#endif

/* ======== internal helpers ======== */
static uint32_t round_up_pow2(uint32_t n)
{
    n--; n|=n>>1; n|=n>>2; n|=n>>4; n|=n>>8; n|=n>>16;
    return n+1;
}

static void set_ctrl(THashTab* t, uint32_t i, uint8_t v)
{
    t->ctrl[i] = v;
    if (i < GW) t->ctrl[t->cap + i] = v;   /* maintain mirror */
}

static void init_table(THashTab* t, uint32_t cap)
{
    t->cap   = cap;
    t->ctrl  = (uint8_t*)malloc(cap + GW);
    memset(t->ctrl, CTRL_EMPTY, cap + GW);
    t->slots = (Slot*)calloc(cap, sizeof(Slot));
    t->size  = 0;
}

static void grow(THashTab* t)
{
    uint32_t old_cap  = t->cap;
    uint8_t* old_ctrl = t->ctrl;
    Slot*    old_slot = t->slots;
    uint32_t i;

    init_table(t, old_cap * 2);

    for (i = 0; i < old_cap; i++) {
        if (old_ctrl[i] < 0x80) {                   /* occupied */
            uint64_t h    = hash_str(old_slot[i].key);
            uint32_t mask = t->cap - 1;
            uint32_t pos  = H1(h) & mask;
            uint32_t step = 0;
            for (;;) {
                uint32_t m = grp_match_avail(t->ctrl + pos);
                if (m) {
                    uint32_t idx = pos + (uint32_t)my_ctz(m);
                    if (idx >= t->cap) idx -= t->cap;
                    set_ctrl(t, idx, H2(h));
                    t->slots[idx] = old_slot[i];     /* move ownership */
                    t->size++;
                    break;
                }
                step += GW;
                pos = (pos + step) & mask;
            }
        }
    }
    free(old_ctrl);
    free(old_slot);
}

/* ======== public API ======== */

THashTab* createHashTbl(int size)
{
    THashTab* t;
    uint32_t cap;

    if (size < (int)GW) size = GW;
    cap = round_up_pow2((uint32_t)size);
    while ((uint64_t)cap * LOAD_NUM / LOAD_DEN < (uint32_t)size)
        cap *= 2;

    t = (THashTab*)calloc(1, sizeof(THashTab));
    if (!t) return NULL;
    init_table(t, cap);
    return t;
}

int insertHashTbl(THashTab* handle, const char* name)
{
    THashTab* t    = handle;
    uint64_t h;
    uint32_t mask, pos, step;
    uint8_t  tag;
    int32_t  avail = -1;

    if ((uint64_t)t->size * LOAD_DEN >= (uint64_t)t->cap * LOAD_NUM)
        grow(t);

    h    = hash_str(name);
    mask = t->cap - 1;
    tag  = H2(h);
    pos  = H1(h) & mask;
    step = 0;

    for (;;) {
        const uint8_t* g = t->ctrl + pos;
        uint32_t m;

        /* match existing key: return -(id)-1 to indicate duplicate */
        m = grp_match_h2(g, tag);
        while (m) {
            uint32_t idx = pos + (uint32_t)my_ctz(m);
            if (idx >= t->cap) idx -= t->cap;
            if (strcmp(t->slots[idx].key, name) == 0)
                return (t->slots[idx].id);
            m &= m - 1;
        }

        /* remember first available slot */
        if (avail < 0) {
            m = grp_match_avail(g);
            if (m) {
                avail = (int32_t)(pos + (uint32_t)my_ctz(m));
                if ((uint32_t)avail >= t->cap)
                    avail -= (int32_t)t->cap;
            }
        }

        /* empty in group ⇒ key absent */
        if (grp_match_empty(g))
            break;

        step += GW;
        pos = (pos + step) & mask;
    }

    {
        int id = ++t->next_id;
        set_ctrl(t, (uint32_t)avail, tag);
        t->slots[avail].key = my_strdup(name);
        t->slots[avail].id  = id;
        t->size++;
        return id;
    }
}

int findHashTbl(THashTab* handle, const char* name)
{
    THashTab* t    = handle;
    uint64_t h    = hash_str(name);
    uint32_t mask = t->cap - 1;
    uint8_t  tag  = H2(h);
    uint32_t pos  = H1(h) & mask;
    uint32_t step = 0;

    for (;;) {
        const uint8_t* g = t->ctrl + pos;
        uint32_t m = grp_match_h2(g, tag);
        while (m) {
            uint32_t idx = pos + (uint32_t)my_ctz(m);
            if (idx >= t->cap) idx -= t->cap;
            if (strcmp(t->slots[idx].key, name) == 0)
                return t->slots[idx].id;
            m &= m - 1;
        }
        if (grp_match_empty(g))
            return -1;
        step += GW;
        pos = (pos + step) & mask;
    }
}

void destroyHashTbl(THashTab* handle)
{
    THashTab* t = handle;
    uint32_t i;
    if (!t) return;
    for (i = 0; i < t->cap; i++) {
        if (t->ctrl[i] < 0x80)
            free(t->slots[i].key);
    }
    free(t->ctrl);
    free(t->slots);
    free(t);
}
