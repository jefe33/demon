#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "synchro.h"

void synchro_pliki(char *source, char *dest); 
char **create_table(DIR *dir, int *size, char *path);

// int main(){
//     synchro_pliki(".", "..");
// }

void synchro_pliki(char *source, char *dest){
    DIR *src_dir, *dest_dir; 
    int src_size, dest_size;

    src_size = dest_size = 0; 
    errno = 0;

    src_dir = opendir(source);
    dest_dir = opendir(dest);

    char **source_paths = create_table(src_dir, &src_size, source);
    char **destination_paths = create_table(dest_dir, &dest_size, dest);

    for (int i=0; i<src_size; i++) {
        printf("nazwa: %s\n", source_paths[i]);
    }
    printf("\n");
    for (int i=0; i<dest_size; i++) {
        printf("nazwa: %s\n", destination_paths[i]);
    }
    
    free(source_paths);
    free(destination_paths);
    closedir(src_dir);
    closedir(dest_dir);
}

char **create_table(DIR *dir, int *size, char *path){
    struct dirent *entry;
    struct stat stats;
    int ret;
    char tmp[0];

    int upper_bound = 1;
    char **paths = (char **) malloc(sizeof(char *));

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }
        strcpy(tmp, path);
        strcat(tmp, "/");
        strcat(tmp, entry->d_name);
        ret = stat(tmp, &stats);

        if (ret) {
            perror ("stat");
        }else{
            if ((stats.st_mode & S_IFMT) == S_IFREG) {
                if (*size < upper_bound) {
                    paths[(*size)++] = entry->d_name;
                }else {
                    upper_bound *= 2;
                    paths = (char **) realloc(paths, upper_bound * sizeof(char *));
                    paths[(*size)++] = entry->d_name;
                }
            } 
        }
    }

    if (errno && !entry){
        perror("readdir");
    }

    return paths;
}