#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

void synchro_pliki(char *sauce, char *dest); 


int main(){
    synchro_pliki(".", "twoja stara sra do gara");
}

void synchro_pliki(char *sauce, char *dest){
    struct dirent *entry;
    DIR *dir;
    struct lista *tmp;

    char ** source_paths = malloc(sizeof(char *));
    int src_amount = 0;
    int src_upper_bound = 1;

    errno = 0;
    dir = opendir(sauce);

    while ((entry = readdir(dir)) != NULL) {
        //printf("nazwa pliku: %s\n", entry->d_name);
        if (src_amount < src_upper_bound) {
            source_paths[src_amount++] = entry->d_name;
        }else {
            src_upper_bound *= 2;
            source_paths = realloc(source_paths, src_upper_bound * sizeof(char *));
            source_paths[src_amount++] = entry->d_name;
        }
        // if (source_paths_head == NULL){
        //     source_paths_head = (struct lista *) malloc(sizeof(struct lista));
        //     source_paths_head->nazwa_pliku = entry->d_name;
        //     source_paths_head->next = NULL;
        //     source_paths_end = source_paths_head;
        // }else {
        //     tmp = (struct lista *) malloc(sizeof(struct lista));
        //     tmp->nazwa_pliku = entry->d_name;
        //     tmp->next = NULL;
        //     source_paths_end->next = tmp;
        //     source_paths_end = tmp;
        // }
    }

    if (errno && !entry){
        perror("readdir");
    }

    printf("c: %d, max: %d\n", src_amount, src_upper_bound);
    for (int i=0; i<src_amount; i++) {
        printf("nazwa: %s\n", source_paths[i]);
    }

    // tmp = source_paths_head;
    // while (tmp != NULL) {
    //     printf("nazwa pliku: %s\n", tmp->nazwa_pliku);
    //     source_paths_end = tmp;
    //     tmp = tmp->next;
    //     free(source_paths_end);
    // }
    
    free(source_paths);
    closedir(dir);
}