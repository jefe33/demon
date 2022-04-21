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
char *full_path(char *path, char* name);

int main(){
    synchro_pliki(".", "..");
}

void synchro_pliki(char *source, char *dest){
    DIR *src_dir, *dest_dir; 
    int src_size, dest_size, i;

    src_size = dest_size = 0; 
    errno = 0;

    src_dir = opendir(source);
    dest_dir = opendir(dest);

    char **source_paths = create_table(src_dir, &src_size, source);
    char **destination_paths = create_table(dest_dir, &dest_size, dest);

    // for (i=0; i<src_size; i++) {
    //     printf("nazwa: %s\n", source_paths[i]);
    // }
    // printf("\n");
    // for (i=0; i<dest_size; i++) {
    //     printf("nazwa: %s\n", destination_paths[i]);
    // }

    for (i = 0; i < src_size; i++) {
        free(source_paths[i]);
    }
    for (i = 0; i < dest_size; i++) {
        free(destination_paths[i]);
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
    char *tmp;

    int upper_bound = 1;
    char **paths = (char **) malloc(sizeof(char *));

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }
        tmp = full_path(path, entry->d_name);
        ret = stat(tmp, &stats);

        if (ret) {
            perror ("stat");
        }else{
            if ((stats.st_mode & S_IFMT) == S_IFREG) {
                if (*size < upper_bound) {
                    paths[(*size)++] = tmp;
                }else {
                    upper_bound *= 2;
                    paths = (char **) realloc(paths, upper_bound * sizeof(char *));
                    paths[(*size)++] = tmp;
                }
            }else {
                free(tmp);
            } 
        }
    }

    if (errno && !entry){
        perror("readdir");
    }

    return paths;
}

char *full_path(char *path, char *name){
    char *tmp;
    unsigned int path_len, name_len, i, offset;

    path_len = strlen(path);
    name_len = strlen(name);
    tmp = (char *) malloc((path_len + name_len + 2) * sizeof(char));
    offset = 0;
    
    for (i = 0; i < path_len; i++){
        tmp[offset++] = path[i];
    }

    tmp[offset++] = '/';

    for (i = 0; i < name_len; i++){
        tmp[offset++] = name[i];
    }

    tmp[offset] = '\0';

    return tmp;
}