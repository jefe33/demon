#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <utime.h>
#include <sys/mman.h>
#include <syslog.h>
#include <signal.h>
#include <regex.h>
#include <linux/limits.h>

#ifndef SYNCHRO_H_   /* Include guard */
#define SYNCHRO_H_

int sync_folders(char *source, char *dest, bool flag, long int threshold);  

#endif

