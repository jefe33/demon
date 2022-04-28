#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <stdbool.h>
#include "synchro.h"

long int mmapMinSize = 1024 * 1024 * 10;
char sourceP[50];
char destP[50];
bool recursive = false;
bool waken = false;
regex_t regex;
int sleepInt;

void wakeywakey (){
    waken = true;
    syslog(LOG_NOTICE("Demon wybudony recznie\n");
}
           
void timetoDIE(){
           syslog(LOG_NOTICE,"Demon Off");
           closelog();
           exit(EXIT_SUCCESS);
}

int main(int arg,char ** argc) {

//check ilosc argumentow
if (arg < 3){
	printf("Zla ilosc arguementow\n");
	printf("poprawny format to: sciezka zrodlowa , sciezka docelowa , flaga(opcjonalny), liczba (tak w przypadku flag s, m\n");
	exit(-1);
}

//check sciezki

    struct stat StSrc, StDest;
    stat(argc[1], &StSrc);
    stat(argc[2], &StDest);
    strcpy(sourceP,argc[2]);
    strcpy(destP,argc[3])
    
    if (!(S_ISDIR(StSrc.st_mode)&& S_ISDIR(StDest))){
        printf("Jedna z sciezek jest bledna");
        exit(-1);
    }

//check getopt

    int opt;
    while((opt = getopt(arg,argc, "Rs:m:")) != -1){
        switch (opt){
            case 'R':
                recursive = 1;
                break;
            case 's':
                if(!regexec(&regex,optarg, 0 , NULL , 0))
                {
                    sleepInt = atoi(optarg);
                }
                else
                {
                    printf("Nie otrzymano liczby\n")
                    exit(-1)
                }
            case 'm':
                if(regexec(&regex,optarg,0, NULL,0))
                {
                    mmapMinSize = atoi(optarg);
                }
                else{
                    printf("nie dostano poprawnej liczby mMapy\n");
                    exit(-1);
                }
                break;
             case ':':
                printf("Nie podano wartości parametru\n");
                break;
            case "?":
                printf("Nieznany\n");
                break;
        }
    }
    
                           
//zamiana w demona

	pid_t pid,sid;
	pid = fork ();
	if (pid < 0){
		exit(EXIT_FAILURE);
	}
	if (pid > 0){
		exit(EXIT_SUCCESS);
	}
        /* Change the file mode mask */
        umask(0);
                
        /* Open any logs here */
        openlog ("mylibrary", LOG_DAEMON + LOG_PID, 0);        
                
        /* Create a new SID for the child process */
        sid = setsid();
        if (sid < 0) {
                /* Log the failure */
                exit(EXIT_FAILURE);
        }
        
        /* Change the current working directory */
        if ((chdir("./")) < 0) {
                /* Log the failure */
                exit(EXIT_FAILURE);
        }
        
        /* Close out the standard file descriptors */
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        
        syslog (LOG_NOTICE,"Demon uruchomiony");
        
        signal(SIGUSR1,wakeywakey):
        signal(SIGUSR2,timetoDIE);
//kod demona

while(1){
    //spanko
        sleep(SleepInt);
    //synchronizacja
        sync_folder(sourceP,destP, recusive, mmapMinSize);
    if(waken == true){
        syslog(LOG_NOTICE,"Wykonano reczna synchronizacje");
        waken = false;
    }else{
        syslog(LOG_NOTICE, "Synchronizacja wykonana");
     }

   }
}



