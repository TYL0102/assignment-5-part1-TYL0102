#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<syslog.h>

int main(int argc, char *argv[]) {
    // check arguments
    if (argc < 3) {
        fprintf(stderr, "too few arguments\n");
        exit(1);
    }

    const char* filename = argv[1];
    const char* writestring = argv[2];

    // open system log
    openlog("writer.c", LOG_PID | LOG_CONS, LOG_USER);

    // log debug message
    syslog(LOG_DEBUG, "Writing %s to %s", writestring, filename);

    // open file
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        syslog(LOG_ERR, "fail to open file %s", filename);
        exit(1);
    }

    // write file
    fprintf(file, "%s", writestring);

    // close file
    fclose(file);

    // close log
    closelog();

    return 0;
}