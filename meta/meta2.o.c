#define GENSYM #A##__COUNTER__

/*test string*/
void tst(char * str);

/*copy literal*/
void cl(char * str);

int switch_flag();



void out1() {
    tst("*1");
    if(!switch_flag()) goto A0;


}
