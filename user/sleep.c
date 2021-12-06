#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"



char buf[512];

int main(int argc,char* args[]) {
   if(argc <= 1) {
	fprintf(2,"sleep:missing argument");
        exit(1);
   }

   if(argc <= 2) {
   	int second = 0;
       	if((second = atoi(args[1])) <=0) {
	    fprintf(2,"sleep:argument must be integers");
	    exit(1);
	}
	sleep(second);
	exit(0);
   }else {
   	fprintf(2,"sleep:too many argument");
	exit(1);
   }
   exit(0);
}

