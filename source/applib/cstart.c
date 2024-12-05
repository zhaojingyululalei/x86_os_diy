
#include "stdlib.h"

int main (int argc, char ** argv);

void cstart (int argc, char ** argv) {
    int ret;
    ret = main(argc,argv);
    exit(ret);
    
    
}