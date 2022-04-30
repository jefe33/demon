#include "synchro.h"

struct info{
    char *f_path;
    char *name;
    struct stat stats;
};

int sync_folders(char *source, char *dest, bool flag, long int threshold); 
struct info *create_table(DIR *dir, int *size, char *path, bool flag);
char *full_path(char *path, char* name);
int copy_file(struct info *src, char *dst, long int threshold);
void free_memory(struct info *target, int len);
int destroy(struct info *target);

int sync_folders(char *source, char *dest, bool flag, long int threshold){
    DIR *src_dir, *dest_dir;
    struct info *src_info, *dest_info;
    int src_size, dest_size, i, j;
    bool found;
    char *src_name, *dest_name, *tmp;
    
    src_size = dest_size = 0; 
    errno = 0;

    
    src_dir = opendir(source);
    if (src_dir == NULL){
        syslog(LOG_ERR, "opendir src sp: %s", strerror(errno));
        return -1;
    }
    
    dest_dir = opendir(dest);
    if (dest_dir == NULL){
        syslog(LOG_ERR, "opendir dest sp: %s", strerror(errno));
        if(closedir(src_dir) == -1){
            syslog(LOG_ERR, "closedir src sp: %s", strerror(errno));
        }
        return -1;
    }

    src_info = create_table(src_dir, &src_size, source, flag);
    if (src_info == NULL){
        if(closedir(src_dir) == -1){
            syslog(LOG_ERR, "closedir src sp: %s", strerror(errno));
        }
        if(closedir(dest_dir) == -1){
            syslog(LOG_ERR, "closedir dest sp: %s", strerror(errno));
        }
        return  -1;
    }
    dest_info = create_table(dest_dir, &dest_size, dest, flag);
    if (dest_info == NULL){
        free_memory(src_info, src_size);
        if(closedir(src_dir) == -1){
            syslog(LOG_ERR, "closedir src sp: %s", strerror(errno));
        }
        if(closedir(dest_dir) == -1){
            syslog(LOG_ERR, "closedir dest sp: %s", strerror(errno));
        }
        return  -1;
    }
    
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
                if (flag && S_ISDIR(src_info[i].stats.st_mode)){
                    sync_folders(src_info[i].f_path, dest_info[j].f_path, flag, threshold);
                }
                else if (src_info[i].stats.st_mtime > dest_info[j].stats.st_mtime){
                    copy_file(&(src_info[i]), dest, threshold);
                }
                break;
            }
        }
        // jesli pliku z src niema w dest utworz
        if (!found) {
            if (S_ISREG(src_info[i].stats.st_mode)){
                copy_file(&(src_info[i]), dest, threshold);
            }else{
                tmp = full_path(dest, src_info[i].name);
                if (tmp == NULL) {
                    continue;
                }
                if (mkdir(tmp, src_info[i].stats.st_mode) == -1){
                    syslog(LOG_ERR, "mkdir: %s", strerror(errno));
                }else{
                    syslog (LOG_NOTICE, "Utworzono folder %s", tmp);
                    sync_folders(src_info[i].f_path, tmp, flag, threshold);
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
    if(closedir(src_dir) == -1){
        syslog(LOG_ERR, "closedir src sp: %s", strerror(errno));
    }
    if(closedir(dest_dir) == -1){
        syslog(LOG_ERR, "closedir dest sp: %s", strerror(errno));
    };
    return 0;
}

struct info *create_table(DIR *dir, int *size, char *path, bool flag){
    struct dirent *entry;
    struct stat stats;
    int ret;
    char *name;
    struct info *infos, *tmp;

    infos = (struct info *) malloc(sizeof(struct info));
    if (infos == NULL){
        syslog(LOG_ERR, "malloc ct: %s", strerror(errno));
        return NULL;
    }
    int upper_bound = 1;
    errno = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }
        name = full_path(path, entry->d_name);
        if (name == NULL){
            continue;
        }
        ret = stat(name, &stats);

        if (ret) {
            syslog(LOG_ERR, "stat: %s", strerror(errno));
            free(name);
        }else{
            if (S_ISREG(stats.st_mode) || (flag && S_ISDIR(stats.st_mode))) {

                if (*size >= upper_bound) {
                    upper_bound *= 2;
                    tmp = (struct info *) realloc(infos, upper_bound * sizeof(struct info));
                    if (tmp){
                        infos = tmp;
                    }else {
                        syslog(LOG_ERR, "realloc: %s", strerror(errno));
                        free(name);
                        continue;
                    }
                }

                infos[*size].f_path = name;
                infos[*size].name = entry->d_name;
                infos[(*size)++].stats = stats;

            }else {
                free(name);
            } 
        }

        errno = 0;
    }

    if (errno && !entry){
        syslog(LOG_ERR, "readdir: %s", strerror(errno));
    }

    return infos;
}

char *full_path(char *path, char *name){
    char *tmp;
    int path_len, name_len, i, offset, slash;

    slash = 1;
    path_len = strlen(path);
    name_len = strlen(name);
    if (path[path_len - 1] == '/'){
        slash = 0;
    }
    tmp = (char *) malloc((path_len + name_len + slash + 1) * sizeof(char));
    if (tmp == NULL){
        syslog(LOG_ERR, "malloc fp: %s", strerror(errno));
        return NULL;
    }
    offset = 0;

    for (i = 0; i < path_len; i++){
        tmp[offset++] = path[i];
    }
    if(slash){
        tmp[offset++] = '/';
    }

    for (i = 0; i < name_len; i++){
        tmp[offset++] = name[i];
    }

    tmp[offset] = '\0';

    return tmp;
}

// tworzy(jesli niema w folderze dst) i kopiuje zawartosc pliku z src oraz ustawia czas modyfikacji na ten z src 
int copy_file(struct info *src, char *dst, long int threshold){
    int fd_to, fd_from;
    char buf[8192];
    ssize_t nread;
    struct utimbuf new_times;
    char *tmp;
    int ret = -1;

    //otwarcie pliku z src
    fd_from = open(src->f_path, O_RDONLY);
    if (fd_from == -1){
        syslog(LOG_ERR, "src open cf: %s", strerror(errno));
        return -1;      
    }

    // zwraca pelno sciezke pliku ktory bedzie dodawany do folderu dst
    tmp = full_path(dst, src->name);
    if (tmp == NULL){
        fd_to = -1;
        goto close_files;
    }
    //utworzenie/wyczyszczenie pliku z dst
    fd_to = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, src->stats.st_mode);
    if (fd_to == -1){
        syslog(LOG_ERR, "dest open cf: %s", strerror(errno));
        goto close_files;
    }
    // jesli rozmiar pliku jest wiekszy od progu to kopiowanie bedzie przrz mmap
    if(src->stats.st_size > threshold){
        char *mem = (char *) mmap(0, src->stats.st_size, PROT_READ, MAP_SHARED, fd_from, 0);
        if(mem == MAP_FAILED){
            syslog(LOG_ERR, "mmap: %s", strerror(errno));
            goto close_files;
        }

        ssize_t nwritten = write(fd_to, mem, src->stats.st_size);
        if(nwritten < src->stats.st_size){
            syslog(LOG_ERR, "write: %s", strerror(errno));
            goto close_files;
        }
        munmap(mem, src->stats.st_size);
        ret = 0;
    }else {
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
                    syslog(LOG_ERR, "write: %s", strerror(errno));
                    goto close_files;

                }
            } while (nread > 0);
        }

        if (nread == 0)
        {
            // ustawienie czasu modyfikacji pliku dst na ten z pliku src
            new_times.actime = src->stats.st_atime;
            new_times.modtime = src->stats.st_mtime;
            if (utime(tmp, &new_times) < 0) {
                syslog(LOG_ERR, "%s: %s", tmp, strerror(errno));
                goto close_files;
            }
            syslog (LOG_NOTICE, "Skopiowano plik %s", src->f_path);
            ret = 0;
            goto close_files;   
        }
    }

    close_files:
        if (close(fd_from) == -1){
            syslog(LOG_ERR, "close src cf: %s", strerror(errno));
        }
        if (fd_to >= 0){
            if(close(fd_to) == -1){
                syslog(LOG_ERR, "close dest cf: %s", strerror(errno));
            }
        }
        if (tmp != NULL){
            free(tmp);
        }
        return ret;
        
}

void free_memory(struct info *target, int len){
    for (int i = 0; i < len; i++){
        free(target[i].f_path);
    }

    free(target);
}

int destroy(struct info *target){
    /* proba usuniecia pliku/folderu jesli to nie pusty folder to 
    *  errno zostanie ustawione na ENOTEMPTY i wykona sie dalszy kod
    * jesli usuniecie sie powiedzie lub wystapi inny blad to fukcja sie konczy
    */

    if(remove(target->f_path) == 0){
        syslog (LOG_NOTICE, "Usunieto %s", target->f_path);
        return 0;
    }else if (errno != ENOTEMPTY) { //jesli to blad inny niz nie pusty folder
        syslog(LOG_ERR, "remove: %s", strerror(errno));
        return -1;
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
    if (dir == NULL){
        syslog(LOG_ERR, "opendir d: %s", strerror(errno));
        return -1;
    }

    info = create_table(dir, &size, target->f_path, true); 
    if (info == NULL){
        if(closedir(dir) == -1){
            syslog(LOG_ERR, "closedir d: %s", strerror(errno));
        }
        return -1;
    }
    for (i = 0; i < size; i++){
        destroy(&info[i]); // rekurencyjne wywolanie funckji dla kazdego elementu folderu
        free(info[i].f_path);
    }

    free(info);

    if(remove(target->f_path)){  // usuniecie juz pustego folderu
        syslog(LOG_ERR, "remove: %s", strerror(errno));
    }

    if(closedir(dir) == -1){
        syslog(LOG_ERR, "closedir d: %s", strerror(errno));
        return -1;
    }
    return 0;
}