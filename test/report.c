#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ALGO 32
#define MAX_N 32

typedef struct
{
    double init_t;
    double create_t;
    double insert_t;
    double mult_t;
    double find_t;
    double destroy_t;

    long init_m;
    long create_m;
    long insert_m;
    long mult_m;
    long destroy_m;

} Result;

typedef struct
{
    char name[64];
    Result result[MAX_N];
} Algo;

typedef struct
{
    long value;
} NValue;

static Algo algos[MAX_ALGO];
static NValue nvalues[MAX_N];

static int algo_count = 0;
static int n_count = 0;

int find_algo(const char *name)
{
    for(int i=0;i<algo_count;i++)
        if(strcmp(algos[i].name,name)==0)
            return i;

    strcpy(algos[algo_count].name,name);
    return algo_count++;
}

int find_n(long n)
{
    for(int i=0;i<n_count;i++)
        if(nvalues[i].value==n)
            return i;

    nvalues[n_count].value=n;
    return n_count++;
}

void print_tm(double t,long m)
{
    printf("%8.2f(%ld)",t,m);
}

int report(const char* FileName)
{
    FILE *fp=fopen(FileName,"r");
    if(!fp)
    {
        perror("open");
        return 1;
    }

    char line[512];

    int cur_algo=-1;
    int cur_n=-1;

    while(fgets(line,sizeof(line),fp))
    {
        if(strncmp(line,"hashTab_",8)==0)
        {
            line[strcspn(line,"\n")]=0;
            cur_algo=find_algo(line);
            continue;
        }

        long n;
        if(sscanf(line,"=== Hash Table Test : N = %ld ===",&n)==1)
        {
            cur_n=find_n(n);
            continue;
        }

        double t;
        long m;

        // 新增 init node name
        if(sscanf(line,"[init node name ] time: %lf ms  mem: +%ld",&t,&m)==2)
        {
            algos[cur_algo].result[cur_n].init_t=t;
            algos[cur_algo].result[cur_n].init_m=m;
            continue;
        }

        if(sscanf(line,"[create ] time: %lf ms  mem: +%ld",&t,&m)==2)
        {
            algos[cur_algo].result[cur_n].create_t=t;
            algos[cur_algo].result[cur_n].create_m=m;
            continue;
        }

        if(sscanf(line,"[insert ] time: %lf ms  mem: +%ld",&t,&m)==2)
        {
            algos[cur_algo].result[cur_n].insert_t=t;
            algos[cur_algo].result[cur_n].insert_m=m;
            continue;
        }

        if(sscanf(line,"[mult insert ] time: %lf ms  mem: +%ld",&t,&m)==2)
        {
            algos[cur_algo].result[cur_n].mult_t=t;
            algos[cur_algo].result[cur_n].mult_m=m;
            continue;
        }

        if(sscanf(line,"[find   ] time: %lf",&t)==1)
        {
            algos[cur_algo].result[cur_n].find_t=t;
            continue;
        }

        if(sscanf(line,"[destroy] time: %lf ms  mem: +%ld",&t,&m)==2)
        {
            algos[cur_algo].result[cur_n].destroy_t=t;
            algos[cur_algo].result[cur_n].destroy_m=m;
            continue;
        }
    }

    fclose(fp);

    printf("\n");
    printf("%12s %-20s %15s %15s %15s %15s %15s %15s\n",
           "N","Algorithm",
           "init(t/m)",
           "create(t/m)",
           "insert(t/m)",
           "multi(t/m)",
           "find(t)",
           "destroy(t/m)");

    printf("-----------------------------------------------------------------------------------------------------------------------------\n");

    for(int n=0;n<n_count;n++)
    {
        for(int a=0;a<algo_count;a++)
        {
            Result *r=&algos[a].result[n];

            printf("%12ld %-20s ",
                   nvalues[n].value,
                   algos[a].name);

            print_tm(r->init_t,r->init_m); printf(" ");
            print_tm(r->create_t,r->create_m); printf(" ");
            print_tm(r->insert_t,r->insert_m); printf(" ");
            print_tm(r->mult_t,r->mult_m); printf(" ");

            printf("%10.2f ",r->find_t);

            print_tm(r->destroy_t,r->destroy_m);

            printf("\n");
        }

        printf("\n");
    }

    return 0;
}