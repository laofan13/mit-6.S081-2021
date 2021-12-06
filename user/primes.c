#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int buf[512];

void prime(int p[]) {
	int primes;
	int new_p[2];
	
	close(p[1]);
	if(read(p[0],&primes,4) <= 0) {
		fprintf(2,"%d process failed to read from the pipe\n",getpid());
		exit(1);
	}
	fprintf(1,"primes %d \n",primes);

	int x;
	if(read(p[0],&x,4) == 4) {
		if(pipe(new_p) < 0){	
			fprintf(2,"pipe failed to create\n");
			exit(1);
		}

		if(fork() == 0) {
			prime(new_p);
		}else{
			if(x%primes) 
				write(new_p[1],&x,4);

			close(new_p[0]);
			while(read(p[0],&x,4) == 4)
				if(x%primes) write(new_p[1],&x,4);
			close(p[0]);
			close(new_p[1]);
			wait(0);
		}
	}

	
	exit(0);
}

int main(int argc, char const *argv[])
{
	int p[2];
	if(pipe(p) < 0){	
		fprintf(2,"pipe failed to create\n");
		exit(1);
    }

	if(fork() == 0) {
		prime(p);
	}else{
		close(p[0]);
		for(int i = 2;i <=35;i++) {
			if(write(p[1],&i,sizeof(i)) <= 0 ){
				fprintf(2,"first process failed to write %d into the pipe\n",i);
				exit(1);
			}
		}
		close(p[1]);
		wait(0);
	}
	exit(0);
}


/* 

int
main(int argc,char *args[]) {
    int p[2];
    if(pipe(p) < 0){	
		fprintf(2,"pipe error");
		exit(1);
    }

	//写数据到管道中
    for(int i = 2;i <=35;i++) {
    	write(p[1],&i,sizeof(i));
    }

	while(1) {
		int pid = fork();
		if(pid > 0) {
			int n = read(p[0],buf,sizeof(buf));
			if(n == 0) {
				close(p[0]);
				break;
			}
			//输出素数
			int prime = buf[0];
			fprintf(1,"primes %d \n",prime);

			//长度
			int len = n / sizeof(int);
			for(int i = 1;i < len;i++) {
				if(buf[i] % prime != 0){
					write(p[1],&buf[i],sizeof(int));
				}
			}

			if(len <= 1) {
				close(p[1]);
				break;
			}
		}else {
			sleep(10);
		}
   } */
//     while(1) {
// 	    int n = read(p[0],buf,sizeof(buf));
// 	    if(n == 0) {
// 	    	close(p[0]);
// 			break;
// 	    }
// 	    //输出素数
// 	    int prime = buf[0];
// 	    fprintf(1,"primes %d \n",prime);

// 	    int len = n / sizeof(int);

// 	    for(int i = 1;i < len;i++) {
// 			if(buf[i] % prime != 0){
// 				write(p[1],&buf[i],sizeof(int));
// 			}
// 		}
// 	    if(len <= 1) {
// 			close(p[1]);
// 			break;
// 		}else{
// 			int pid = fork();
// 			if(pid > 0) {
// 				break;
// 			}
// 		}

//    }
//    exit(0);
// }
