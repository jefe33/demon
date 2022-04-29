#include "synchro.h"

long int mmapMinSize = 1024 * 1024 * 10; // 10 MB
char sourceP[PATH_MAX];
char destP[PATH_MAX];
bool recursive = false;
bool waken = false;
regex_t regex;
int sleepInt = 300;

void wakeywakey(){
    waken = true;
    syslog(LOG_NOTICE, "Demon wybudony recznie\n");
}
           
void timetoDIE(){
           syslog(LOG_NOTICE, "Demon Off");
           closelog();
           exit(EXIT_SUCCESS);
}

int main(int arg, char ** argc) {

    //check ilosc argumentow
    if (arg < 3){
        printf("Zla ilosc arguementow\n");
        printf("poprawny format to: sciezka zrodlowa , sciezka docelowa , flaga(opcjonalny), liczba (tak w przypadku flag s, m\nProgram anulowany\n");
        exit(EXIT_FAILURE);
    }

    //check sciezki

    struct stat StSrc, StDest;
    if (stat(argc[1], &StSrc)){
        perror ("stat");
        exit(EXIT_FAILURE);
    }
    if (stat(argc[2], &StDest)){
        perror ("stat");
        exit(EXIT_FAILURE);
    }
    strcpy(sourceP, argc[1]);
    strcpy(destP, argc[2]);
    
    if (!(S_ISDIR(StSrc.st_mode) && S_ISDIR(StDest.st_mode))){
        printf("Jedna z sciezek jest bledna\nProgram anulowany\n");
        exit(-1);
    }

    //check regex

    int regexComp = regcomp(&regex, "^[0-9]*$", REG_EXTENDED);
    if(regexComp){
        printf ("Blad kompilacji regexa\nProgram anulowany\n");
        exit(-1);
    }

    //check getopt
    int opt;
    opterr = 0;
    while((opt = getopt(arg, argc, "Rs:m:")) != -1){
        switch (opt){
            case 'R':
                recursive = true;
                break;
            case 's':
                if(!regexec(&regex, optarg, 0, NULL, 0))
                {
                    sleepInt = atoi(optarg);
                }
                else
                {
                    printf("Nie otrzymano liczby snu\nProgram anulowany\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'm':
                if(regexec(&regex, optarg, 0, NULL, 0))
                {
                    // w parametrze m wartosc ma byc w MB
                    mmapMinSize = atoi(optarg) * 1024 * 1024; // zamiana na bajty
                }
                else{
                    printf("Nie dostano poprawnej liczby mMapy\nProgram anulowany\n");
                    exit(EXIT_FAILURE);
                }
                break;
             case ':':
                printf("Nie podano wartości parametru\nProgram anulowany\n");
                exit(EXIT_FAILURE);
                break;
            case '?':
                printf("Podano nieznany argument: -%c\nProgram anulowany\n", optopt);
                exit(EXIT_FAILURE);
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
    openlog ("demon synchorizujacy", LOG_PID, LOG_DAEMON);        
            
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
    
    syslog (LOG_NOTICE, "Demon uruchomiony");
    
    
    if(signal(SIGUSR1, wakeywakey) == SIG_ERR)
    {
        perror("Blad sygnalu!");
        exit(EXIT_FAILURE);
    }
    if(signal(SIGUSR2, timetoDIE) == SIG_ERR)
    {
        perror("Blad sygnalu!");
        exit(EXIT_FAILURE);
    }
    

    //kod demona
    while(1){
        //synchronizacja
        sync_folders(sourceP, destP, recursive, mmapMinSize);
        if(waken == true){
            syslog(LOG_NOTICE, "Wykonano reczna synchronizacje");
            waken = false;
        }else{
            syslog(LOG_NOTICE, "Synchronizacja wykonana");
        }
        //spanko
        sleep(sleepInt);
    }
}
