//
// Created by huj4 on 2/4/23.
//
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>

int main( int argc , char * argv[] )
{
    if (argc != 3){
        errno = EINVAL;
        syslog()
    }

}