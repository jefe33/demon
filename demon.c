#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <regex.h>
#include "synchro.c"

long int mmapMinSize = 1024 * 1024 * 10;
char sourceP[100];
char destP[100];
bool recursive = false;
bool waken = false;
regex_t regex;
int sleepInt = 60;

void wakeywakey (){
    waken = true;
    syslog(LOG_NOTICE,"Demon wybudzony recznie\n");
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
	printf("poprawny format to: sciezka zrodlowa , sciezka docelowa , flaga(opcjonalny), liczba (tak w przypadku flag s, m\nProgram anulowany\n");
	exit(-1);
}
//printf("check1\n");
//check sciezki

    struct stat StSrc, StDest;
    stat(argc[1], &StSrc);
    stat(argc[2], &StDest);
    //printf("check11\n");
    strcpy(sourceP,argc[1]);
    strcpy(destP,argc[2]);
    
    //printf("check12\n");
    if (!(S_ISDIR(StSrc.st_mode)&& S_ISDIR(StDest.st_mode))){
        printf("Jedna z sciezek jest bledna\nProgram anulowany\n");
        exit(-1);
    }

//printf("check2\n");
//check regex

int regexComp = regcomp(&regex, "^[0-9]*$",REG_EXTENDED);
if(regexComp){
printf ("Blad kompilacji regexa\nProgram anulowany\n");
exit(-1);
}
//check getopt


    int opt;
    opterr = 0;
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
                    printf("Nie otrzymano liczby snu\nProgram anulowany\n");
                    exit(-1);
                }
                break;
            case 'm':
                if(!regexec(&regex,optarg,0, NULL,0))
                {
                    mmapMinSize = atoi(optarg);
                }
                else{
                    printf("Nie dostano poprawnej liczby mMapy\nProgram anulowany\n");
                    exit(-1);
                }
                break;
             case ':':
                printf("Nie podano wartości parametru\nProgram anulowany\n");
                exit(-1);
                break;
            case '?':
                printf("Podano nieznany argument: -%c\nProgram anulowany\n",optopt);
                exit(-1);
                break;
        }
    }
    
//printf("check3\n");                     
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
        
        signal(SIGUSR1,wakeywakey);
        signal(SIGUSR2,timetoDIE);
//kod demona
//printf("check4\n");
while(1){
    //spanko
        sleep(sleepInt);
    //synchronizacja
        sync_folders(sourceP,destP, recursive, mmapMinSize);
    if(waken == true){
        syslog(LOG_NOTICE,"Wykonano reczna synchronizacje");
        waken = false;
    }else{
        syslog(LOG_NOTICE, "Synchronizacja wykonana");
     }

   }
}
