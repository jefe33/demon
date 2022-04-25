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

void synchro_pliki(char *source, char *dest, bool flag); 
struct info *create_table(DIR *dir, int *size, char *path, bool flag);
char *full_path(char *path, char* name);
void copy_file(struct info *src, char *dst);
void free_memory(struct info *target, int len);
void destroy(struct info *target);

struct info{
    char *f_path;
    char *name;
    struct stat stats;
};

int main(){
    synchro_pliki("/opt/zadania/test/src", "/opt/zadania/test/dest", true);
}

void synchro_pliki(char *source, char *dest, bool flag){
    DIR *src_dir, *dest_dir;
    struct info *src_info, *dest_info;
    int src_size, dest_size, i, j;
    bool found;
    char *src_name, *dest_name, *tmp;
    
    src_size = dest_size = 0; 
    errno = 0;
    
    src_dir = opendir(source);
    dest_dir = opendir(dest);

    src_info = create_table(src_dir, &src_size, source, flag);
    dest_info = create_table(dest_dir, &dest_size, dest, flag);

    bool exists[dest_size];

    for (i = 0; i < dest_size; i++) {
        exists[i] = false;
    }

    for (i = 0; i < src_size; i++) {
        found = false;
        for (j = 0; j < dest_size; j++) {
            if (strcmp(src_info[i].name, dest_info[j].name) == 0) {
                exists[j] = true;
                found = true;
                if (flag && ((src_info[i].stats.st_mode & S_IFMT) == S_IFDIR)){
                    synchro_pliki(src_info[i].f_path, dest_info[j].f_path, flag);
                }
                else if (src_info[i].stats.st_mtime > dest_info[j].stats.st_mtime){
                    copy_file(&(src_info[i]), dest);
                }
                break;
            }
        }
        // jesli pliku z src niema w dest utworz
        if (!found) {
            if ((src_info[i].stats.st_mode & S_IFMT) == S_IFREG){
                copy_file(&(src_info[i]), dest);
            }else{
                tmp = full_path(dest, src_info[i].name);
                if (mkdir(tmp, src_info[i].stats.st_mode) == -1){
                    perror("mkdir");
                }else{
                    synchro_pliki(src_info[i].f_path, tmp, flag);
                }
                free(tmp);
            }
        }
    }

    //usuwanie plikow z dest jesli niema w src
    for (i = 0; i < dest_size; i++) {
        if(!exists[i]){
            destroy(&dest_info[i]);
        }
    }

    free_memory(src_info, src_size);
    free_memory(dest_info, dest_size);
    closedir(src_dir);
    closedir(dest_dir);
}

struct info *create_table(DIR *dir, int *size, char *path, bool flag){
    struct dirent *entry;
    struct stat stats;
    int ret;
    char *tmp;

    struct info *infos = (struct info *) malloc(sizeof(struct info));
    int upper_bound = 1;
    errno = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }
        tmp = full_path(path, entry->d_name);
        ret = stat(tmp, &stats);

        if (ret) {
            perror ("stat");
        }else{
            if (((stats.st_mode & S_IFMT) == S_IFREG) || (flag && (stats.st_mode & S_IFMT) == S_IFDIR)) {

                if (*size >= upper_bound) {
                    upper_bound *= 2;
                    infos = (struct info *) realloc(infos, upper_bound * sizeof(struct info));
                }

                infos[*size].f_path = tmp;
                infos[*size].name = entry->d_name;
                infos[(*size)++].stats = stats;

            }else {
                free(tmp);
            } 
        }

    }

    if (errno && !entry){
        perror("readdir");
    }

    return infos;
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
void copy_file(struct info *src, char *dst){
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;
    struct utimbuf new_times;
    char *tmp;

    //otwarcie pliku z src
    fd_from = open(src->f_path, O_RDONLY);
    if (fd_from == -1){
        perror("src open");
        return;      
    }

    // zwraca pelno sciezke pliku ktory bedzie dodawany do folderu dst
    tmp = full_path(dst, src->name);
    //utworzenie/wyczyszczenie pliku z dst
    fd_to = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, src->stats.st_mode);

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
        new_times.actime = src->stats.st_atime;
        new_times.modtime = src->stats.st_mtime;
        if (utime(tmp, &new_times) < 0) {
            perror(tmp);
        }
    }
}

void free_memory(struct info *target, int len){
    for (int i = 0; i < len; i++){
        free(target[i].f_path);
    }

    free(target);
}

void destroy(struct info *target){
    /* proba usuniecia pliku/folderu jesli to nie pusty folder to 
    *  errno zostanie ustawione na ENOTEMPTY i wykona sie dalszy kod
    * jesli usuniecie sie powiedzie lub wystapi inny blad to fukcja sie konczy
    */
    int ret = remove(target->f_path); 

    if(ret == 0){
        return;
    }else if (errno != ENOTEMPTY) { //jesli to blad inny niz nie pusty folder
        perror("remove");
        return;
    }

    /* jesli target to nie pusty folder to wykona sie kod ponizej
    *  ktory tworze tablice elementow i wywoluje rekurencyjnie ta 
    *  funkcje dla kazdego elementu a na koniec usuwa sam folder
    */
    DIR *dir;
    struct info *info;
    int size, i;
    
    size = 0; 

    dir = opendir(target->f_path);

    info = create_table(dir, &size, target->f_path, true); 

    for (i = 0; i < size; i++){
        destroy(&info[i]);  // rekurencyjne wywolanie funckji dla kazdego elementu folderu
        free(info[i].f_path);
    }

    free(info);

    if(remove(target->f_path)){  // usuniecie juz pustego folderu
        perror("remove");
    }

    closedir(dir);
}