//
// Created by huj4 on 2/4/23.
//
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <sys/stat.h>

int isDir(const char *path){
    struct stat *statBuffer = NULL;
    if (stat(path, statBuffer) != 0)
        return 0;
    return S_ISDIR(statBuffer->st_mode);
}

int main( int argc , char * argv[] )
{
    if (argc != 3){
        syslog(LOG_USER, "invalid argument, should receive filesdir and searchstr");
        return 1;
    }

    syslog(LOG_DEBUG, "Writing %s to %s", argv[2], argv[1]);

    if (isDir(argv[1])){
        syslog(LOG_ERR, "%s is a directory", argv[1]);
        return 1;
    }

    FILE * fp;
    fp = fopen(argv[1], "w+");
    fputs(argv[2], fp);
    fclose(fp);

    return 0;

}