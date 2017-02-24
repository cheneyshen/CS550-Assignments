/*************************************************************************
	> File Name: strtok.c
	> Author: 
	> Mail: 
	> Created Time: Tue 03 Nov 2015 09:02:49 PM CST
 ************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char *argv[])
{
    char *str1, *str2, *token, *subtoken;
    char *saveptr1, *saveptr2;
    int j, n=0;

    if (argc != 4) {
        fprintf(stderr, "Usage: %s string delim subdelim\n",
               argv[0]);
        exit(EXIT_FAILURE);

    }

    for (j = 1, str1 = argv[1]; ; j++, str1 = NULL) {
        token = strtok_r(str1, argv[2], &saveptr1);
        if (token == NULL)
        break;
        n=0;
        for (str2 = token; ; str2 = NULL) {
            n++;
            subtoken = strtok_r(str2, argv[3], &saveptr2);
            if (subtoken == NULL)
            break;
            if (1==n) printf("%s\t", subtoken);
            else printf("%s\n", subtoken);

        }

    }

    exit(EXIT_SUCCESS);

}
