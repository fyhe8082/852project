/*************************************************************************
    > File Name: tfrc.c
    > Author: Xubin Zhuge
    > Mail: xzhuge@clemson.edu 
    > Created Time: Sun 15 Nov 2015 08:26:56 PM EST
 ************************************************************************/

#include "tfrc.h"

void DieWithError(char *errorMsg) {
	perror(errorMsg);
	exit(1);
}

