#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fs.h"


const char local[] = ".";
const char parent[] = "..";

void find(char *path,char* target) {
    int fd;
    char buf[512], *p;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0) {
        fprintf(2,"find:cannot open %s\n",path);
        return;
    }
    if(fstat(fd,&st) < 0) {
        fprintf(2,"find:cannot fstat %s\n",path);
        return;
    }
    switch (st.type)
    {
    case T_FILE:
        fprintf(2,"1th argument must be directory");
        break;
    case T_DIR:
        if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
            printf("find: path too long\n");
            break;
        }
        strcpy(buf,path);
        p = buf + strlen(buf);
        *p++ = '/';

        while(read(fd, &de, sizeof(de)) == sizeof(de)) {
            //if is "." and ".."
            if(de.inum == 0 || strcmp(de.name,local) == 0 || strcmp(de.name,parent) == 0)
                continue;
            memmove(p, de.name,sizeof(de.name));
            if(stat(buf,&st) < 0) {
                fprintf(2,"find: cannot stat %s\n", buf);
                continue;
            }
            if(st.type == T_FILE && strcmp(de.name,target)==0) {
                fprintf(1,"%s\n",buf);
            }else if (st.type == T_DIR)
            {
                find(buf,target);
            }
        }
        break;
    default:
        break;
    }
}

int main(int argc, char *argv[])
{
    if(argc < 2) {
        fprintf(2,"Three parameters are required, but only two\n");
        exit(1);
    }
    for(int i = 2;i < argc;i++) {
        find(argv[1],argv[i]);
    }
    exit(0);
}