#include "config.h"
#define MAX_N 20
#define MAX_SEC 100
//globals
int numofproc;
int * pid_list = NULL;
FILE * masterlog = NULL;
//Semaphore
int sem_id;

void get_sem()
{
    //creates the sysv semaphores (or tries to at least)
    key_t key = ftok(".", 'a');
    //gets chared memory
    if ((sem_id = semget(key, 1, IPC_CREAT | 666)) == -1)
    {
        perror("master.c: semget failed:");
        cleanupSemaphore();
        exit(-1);
    }
    return;
}

void cc_handler()
{
    //Handler that catches ctrl-c, cleans up before closing
    error_cc();
}

void error_cc()
{ 
    // out of time
    //go through pid list and remove still running children
    fprintf(stderr, "Master recieved 'Ctrl-C', terminating...\n");

    for (int i = 0; i < numofproc; i++)
    {
        kill(pid_list[i], SIGTERM);
    }

    while(wait(NULL) > 0);

    cleanupSemaphore();

    if(masterlog != NULL)
    {
        fprintf(masterlog, "Master closing due to CTRL-C...\n");
        fclose(masterlog);
    }

    exit(-1);
}

void oot_handler()
{
    //handler to call the out of time error function 
    error_oot();
}

void error_oot()
{ 
    // out of time
    //go through pid list and remove still running children
    fprintf(stderr, "Master reached max time, terminating...\n");

    for (int i = 0; i < numofproc; i++)
    {
        kill(pid_list[i], SIGTERM);
    }

    while(wait(NULL) > 0);

    cleanupSemaphore();

    if(masterlog != NULL)
    {
        fprintf(masterlog, "Master closing due to Sigalrm...\n");
        fclose(masterlog);
    }

    exit(-1);
}

void error_fork()
{
    //go thorough the pid list and kill any process that is still running
    //idealy this should never be used
    fprintf(stderr, "Error forking, terminating...\n");

    cleanupSemaphore();

    for (int i = 0; i < numofproc; i++)
    {
        kill(pid_list[i], SIGTERM);
    }

    exit(-1);
}

void cleanupSemaphore()
{
    //Remove semaphore
    semctl(sem_id, 0, IPC_RMID, NULL);
}

int main(int argc, char *argv[])
{
    //This signal catches the alarm if time runs out
    signal(SIGALRM, oot_handler);
    signal(SIGINT, cc_handler);

    int pid, opt, ss = 100, n = 20, max_sec = 100, max_n = 20;

    time_t T;
    struct tm tm;

    T = time(NULL);
    tm = *localtime(&T);

    masterlog = fopen("logfile.master", "a");

    fprintf(masterlog, "Master process began...\n");

    while ((opt = getopt(argc, argv, "t:")) != -1)
    {
        switch (opt)
        {
        case 't':
            ss = atoi(optarg); // sets the max number of seconds the master will wait for the children to terminate normally
            break;
        default:
            printf("Error: invalid argument, exiting...\n\n");
            exit(-1);
        }
    }

    if (argc != 4)
    {
        printf("Incorrect call of master. Try ./master -t (Time) (Number of Processes)\n");
        exit(0);
    }
    else
    {
        n = atoi(argv[3]);
        //printf("N is: %d\n", n);
        if (n > max_n)
        {
            printf("N was too large and was set to default value of 20\n");
            n = max_n;
        }
    }
    numofproc = n;
    pid_list = malloc(sizeof(int) * n);

    //Initialize semaphore
    get_sem();
    semctl(sem_id, 0, SETVAL, 1);
    
    for (int i = 0; i < n; i++)
    {
        T = time(NULL);
        tm = *localtime(&T);

        fprintf(masterlog, "Forking child process %d\n", i + 1);

        pid = fork();

        if (pid == -1)
        {
            printf("Fork failed, program exiting early...\n");
            error_fork();
        }

        if (pid == 0)
        {
            char * numprocs = malloc(sizeof(char) * 3);
            sprintf(numprocs, "%d", n);
            char * pnum = malloc(sizeof(char) * 3);
            sprintf(pnum, "%d", i + 1);
            //Execl to call the slave process, I pass proc_id and n so its easy to look through the shared arrays
            execl("./slave", "./slave", pnum, numprocs, NULL); // replaces the child fork with an instance of child, with its process number passed to it
        }
        else
        {
            //this is what the parent will do
            pid_list[i] = pid; //add pid to a pid list
        }
    }

    //after all of the children are forked setup a signal handler for the alarm
    alarm(ss);

    //wait(NULL); // wait for all the children to exit
    while(wait(NULL) > 0);

    //Clean shared memory for choosing, then for number
    printf("Finished waiting for children, cleaning up memory and exiting...\n");

    cleanupSemaphore();

    T = time(NULL);
    tm = *localtime(&T);

    fprintf(masterlog, "Shared memory & Semaphore detached and deallocated, closing master process\n");

    fclose(masterlog);

    exit(0);
}
