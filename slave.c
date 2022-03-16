#include "config.h"

// Global IDs
bool *choosing_ptr;
int *numbers_ptr;
int choose_id, number_id;
// Make logfile and cstest globals
FILE *logfile = NULL;
FILE *cstest = NULL;
//Semaphore
int sem_id;

void get_sem()
{
    //creates the sysv semaphores (or tries to at least)
    key_t key = ftok(".", 'a');
    //gets chared memory
    if ((sem_id = semget(key, 1, 0)) == -1)
    {
        perror("master.c: semget failed:");
        cleanupSlave();
        exit(-1);
    }
    return;
}

void sem_post()
{
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = 1;
    op.sem_flg = 0;
    semop(sem_id, &op, 1);
    return;
}

void sem_wait()
{
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = -1;
    op.sem_flg = 0;
    semop(sem_id, &op, 1);
    return;
}

void cleanupSlave()
{
    if (cstest != NULL) fclose(cstest);
    if (logfile != NULL) fclose(logfile);

    exit(0);
}

int maxValue(int *array, int arraySize)
{
    int max = 0;
    for (int k = 0; k < arraySize; k++)
    {
        if (array[k] > max)
        {
            max = array[k];
        }
    }
    return max;
}

void print_choosing(int n)
{
    printf("Choosing: { ");
    for (int i = 0; i < n; i++)
    {
        printf("%d ", choosing_ptr[i]);
    }
    printf("}\n");
}

void print_numbers(int n)
{
    printf("Numbers: { ");
    for (int i = 0; i < n; i++)
    {
        printf("%d ", numbers_ptr[i]);
    }
    printf("}\n");
}

int main(int argc, char *argv[])
{
    signal(SIGTERM, cleanupSlave);
    int nproc = atoi(argv[2]);   // gets the number of processes
    int proc_id = atoi(argv[1]); // gets the process number

    printf("Inside slave process\n");

    //Get semaphore
    get_sem();

    // Initialize srand for random sleep times
    srand(proc_id);
    int r = 0;

    // open child specific logfile ex. logfile.01, logfile.02 ... logfile.20 etc.
    char *logname = malloc(sizeof(char) * 11);
    sprintf(logname, "logfile.%d", proc_id);
    // Initialize time variables to output Systemtime in logfile
    time_t T;
    struct tm tm;

    for (int i = 0; i < 5; i++)
    { // loop with value of 5
      // loop will make it so each child will access the shared logfile 5 times or whatever you set this loop to
        sem_wait();
        // Critical Section, open logfile, write to it, and close
        // Open logfile
        logfile = fopen(logname, "a");
        if (logfile == NULL)
        {
            printf("Error opening logfile, exiting...\n");
            exit(-1);
        }
        // Open cstest
        cstest = fopen("cstest", "a");
        if (cstest == NULL)
        {
            printf("Error opening logfile, exiting...\n");
            exit(-1);
        }
        T = time(NULL);
        tm = *localtime(&T);
        fprintf(logfile, "Proc_Id: %d Inside critical section %02d:%02d:%02d\n", proc_id, tm.tm_hour, tm.tm_min, tm.tm_sec);
        printf("Proc_Id: %d Inside critical section\n", proc_id);
        r = (rand() % 5) + 1; // Plus 1 makes sure it never returns 0, range is 1-5
        sleep(r);
        // Initialize time variables to output Systemtime in logfile
        T = time(NULL);
        tm = *localtime(&T);
        fprintf(cstest, "%02d:%02d:%02d Queue %d File modified by process number %d\n", tm.tm_hour, tm.tm_min, tm.tm_sec, numbers_ptr[proc_id - 1], proc_id);
        fprintf(logfile, "Proc_Id: %d Writing to cstest %02d:%02d:%02d\n", proc_id, tm.tm_hour, tm.tm_min, tm.tm_sec);
        // Output choosing and numbers for debug
        // print_choosing(nproc);
        // print_numbers(nproc);

        r = (rand() % 5) + 1;
        sleep(r);
        T = time(NULL);
        tm = *localtime(&T);
        
        fprintf(logfile, "Proc_Id: %d Exiting critical section %02d:%02d:%02d\n", proc_id, tm.tm_hour, tm.tm_min, tm.tm_sec);
        fclose(logfile);
        fclose(cstest);
        logfile = NULL;
        cstest = NULL;
        sem_post();
    }

    // remainder_section

    exit(0);
}