#include <stdio.h>
#include <sys/stat.h>
int main(int argc, char * argv[]) {
    int i;
    struct stat buf;
    char * ptr;
    char filesize[20] = {0};
    for (i=1; i<argc; i++) {
        if (lstat(argv[i], &buf) < 0) {
            perror("Error\n");
            continue;
        }
        if (S_ISREG(buf.st_mode))
            ptr = "Normal File";
        if (S_ISDIR(buf.st_mode))
            ptr = "Dir";
        // and so on............
        snprintf(filesize, sizeof(filesize), " %ld", buf.st_size);
        printf("filesize %s\n", filesize);
        printf("Args: %s is %s\n", argv[i], ptr);
    }
    return 0;
}