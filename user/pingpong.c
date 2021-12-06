#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int
main(int argc,char *args[]) {
    int pid;
    int p[2];
    pipe(p);

    if(fork() == 0) {
        pid = getpid();
        char msg[] = "son";
        char buf[10];
        while(read(p[0],buf,sizeof(buf)) <= 0) {
            fprintf(2,"failed to read in child\n");
            exit(1);
        }
        close(p[0]);
        fprintf(1,"%d: received pong\n",pid);

        while(write(p[1],msg,sizeof(msg)) <= 0) {
            fprintf(2,"failed to write in child\n");
            exit(1);
        }
        close(p[1]);
    }else{
        pid = getpid();
        char msg[] = "baba";
        char buf[10];

        while(write(p[1],msg,sizeof(msg)) <= 0) {
            fprintf(2,"failed to write in parent\n");
            exit(1);
        }
        close(p[1]);
        wait(0);
        while(read(p[0],buf,sizeof(buf)) <= 0) {
            fprintf(2,"failed to write in parent\n");
            exit(1);
        }
        close(p[0]);
        fprintf(1,"%d: received ping\n",pid);

    }

    exit(0);
}
