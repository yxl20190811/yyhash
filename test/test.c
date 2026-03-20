#include <stdio.h>
#include <stdlib.h>
void test2(const char* dir, unsigned N){
    char buf[100];
    sprintf(buf,"echo \"%s\" >>  ./1.txt",dir);
    system(buf);

    sprintf(buf,"./%s/bin/main.exe %u >> ./1.txt",dir,  N);
    system(buf);
}
void test1(unsigned N){
    test2("hashTab_2Levelcache", N);
    //test2("hashTab_map", N);
    //test2("hashTab_swissTab", N);
    //test2("hashTab_utHash", N);
}
int report(const char* FileName);
void main(){
    
    system("echo \"\" > ./1.txt");
    test1(1000);
    test1(1000000);
    test1(100000000);
    
    report("./1.txt");
}