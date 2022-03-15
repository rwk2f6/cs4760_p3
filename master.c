#include "config.h"

//globals
bool * choosing_ptr;
int * numbers_ptr;
int choosing_id, numbers_id, numofproc;
int * pid_list = NULL;
FILE * masterlog = NULL;
//Semaphore
sem_t mutex;

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

    cleanup_shm();

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

    cleanup_shm();

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

    cleanup_shm();

    for (int i = 0; i < numofproc; i++)
    {
        kill(pid_list[i], SIGTERM);
    }

    exit(-1);
}

void cleanup_shm()
{
    //Detach shared memory
    shmdt(choosing_ptr);
    shmdt(numbers_ptr);

    //Remove shared memory
    shmctl(choosing_id, IPC_RMID, NULL);
    shmctl(numbers_id, IPC_RMID, NULL);

    //Remove semaphore
    sem_close(&mutex);
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

    bool * choosing = malloc(sizeof(bool) * n);
    int * number = malloc(sizeof(int) * n);
    //Initialize both arrays
    key_t choosing_key = ftok("Makefile", 'a');
    choosing_id = shmget(choosing_key, sizeof(bool) * n, IPC_CREAT | 0666);

    key_t numbers_key = ftok(".", 'a');
    numbers_id = shmget(numbers_key, sizeof(int) * n, IPC_CREAT | 0666);

    if (choosing_id == -1){
        perror("monitor.c: Error: Shared memory (buffer) could not be created");
        printf("exiting\n\n");
        //early cleanup
        exit(0);
    }

    if (numbers_id == -1){
        perror("monitor.c: Error: Shared memory (buffer) could not be created");
        printf("exiting\n\n");
        //early cleanup
        exit(0);
    }

    //shm has been allocated, now we can attach
    choosing_ptr = (bool *)shmat(choosing_id, 0, 0);
    numbers_ptr = (int *)shmat(numbers_id, 0, 0);

    for (int i = 0; i < n; i++)
    {
        choosing_ptr[i] = false;
    }
    for (int i = 0; i < n; i++)
    {
        numbers_ptr[i] = 0;
    }

    //Initialize semaphore
    sem_init(&mutex, 1, 1);
    
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

    cleanup_shm();

    T = time(NULL);
    tm = *localtime(&T);

    fprintf(masterlog, "Shared memory & Semaphore detached and deallocated, closing master process\n");

    fclose(masterlog);

    exit(0);
}
