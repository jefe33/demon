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
#include "synchro.h"

void synchro_pliki(char *source, char *dest); 
char **create_table(DIR *dir, int *size, char *path);
char *full_path(char *path, char* name);
void copy_file(char *src, char *dst);
bool compare_timestamps(char *src, char *dst);

struct info{
    char *f_path;
    char *name;
    struct stat stats;
};

// int main(){
//     synchro_pliki("/opt/zadania/test/src", "/opt/zadania/test/dest");
// }

void synchro_pliki(char *source, char *dest){
    DIR *src_dir, *dest_dir;
    struct info *src_info, *dest_info;
    int src_size, dest_size, i, j;
    bool found;
    char *src_name, *dest_name;
    
    src_size = dest_size = 0; 
    errno = 0;

    src_dir = opendir(source);
    dest_dir = opendir(dest);

    char **source_paths = create_table(src_dir, &src_size, source);
    char **destination_paths = create_table(dest_dir, &dest_size, dest);

    src_info = (struct info *) malloc(src_size * sizeof(struct info *));
    dest_info = (struct info *) malloc(dest_size * sizeof(struct info *));

    bool exists[dest_size];

    for (i = 0; i < dest_size; i++) {
        exists[i] = false;
    }

    for (i = 0; i < src_size; i++) {
        found = false;
        src_name = strrchr(source_paths[i], '/');
        src_name++;
        for (j = 0; j < dest_size; j++) {
            dest_name = strrchr(destination_paths[j], '/');
            if (strcmp(src_name, ++dest_name) == 0) {
                exists[j] = true;
                found = true;
                if (!compare_timestamps(source_paths[i], destination_paths[j])){
                    copy_file(source_paths[i], dest);
                }
                break;
            }
        }
        // jesli pliku z src niema w dest utworz
        if (!found) {
            copy_file(source_paths[i], dest);
        }
    }

    //usuwanie plikow z dest jesli niema w src
    for (i = 0; i < dest_size; i++) {
        if(!exists[i]){
            if(remove(destination_paths[i])){
                perror("remove");
            }
        }
    }

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

// tworzy(jesli niema w folderze dst) i kopiuje zawartosc pliku z src oraz ustawia czas modyfikacji na ten z src 
void copy_file(char *src, char *dst){
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;
    struct stat stats;
    struct utimbuf new_times;
    int saved_errno;
    char *tmp;

    //otwarcie pliku z src
    fd_from = open(src, O_RDONLY);
    if (fd_from == -1){
        perror("src open");
        return;      
    }

    //pobranie statystyk pliku z src
    if(stat(src, &stats)){
        perror ("stat create file");
    }

    //ze sciezki do pliku src np. /home/user/plik ustawia wskaznik na nazwe pliku /plik z ukosnikiem
    tmp = strrchr(src, '/');
    // przejscie o jeden w prawo zeby pozbyc sie ukosnika z nazwy pliku /plik -> plik
    tmp++;
    // zwraca pelno sciezke pliku ktory bedzie dodawany do folderu dst
    tmp = full_path(dst, tmp);
    //utworzenie/wyczyszczenie pliku z dst
    fd_to = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, stats.st_mode);

    if (fd_to == -1){
        perror("dest open");
        close(fd_from);
        return;
    }
    // odczytanie czesci(rozmiar bufora) pliku src a nastepnie zapisanie tej czesci do pliku dest az odczyta i zapisze caly plik
    while (nread = read(fd_from, buf, sizeof buf), nread > 0)
    {
        //ustawienie wskaznika na poczatek bufera bo po zapisie wskaznik sie przesuwa
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0)
            {
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR)
            {
                perror("write");
            }
        } while (nread > 0);
    }

    if (nread == 0)
    {
        if (close(fd_to) < 0)
        {
            fd_to = -1;
        }
        close(fd_from);
        // ustawienie czasu modyfikacji pliku dst na ten z pliku src
        new_times.actime = stats.st_atime;
        new_times.modtime = stats.st_mtime;
        if (utime(tmp, &new_times) < 0) {
            perror(tmp);
        }
    }
}

//jesli pliki maja taki sam czas modyfikacji zwroc true
bool compare_timestamps(char *src, char *dst){
    struct stat attr1, attr2;
    if (stat(src, &attr1) != 0 || stat(dst, &attr2) != 0)
    {
        perror("timestamp");
        return NULL;
    }
    if(attr1.st_mtime != attr2.st_mtime){
        return false;
    }
    return true;
}