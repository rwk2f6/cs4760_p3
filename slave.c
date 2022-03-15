#include "config.h"

// Global IDs
bool *choosing_ptr;
int *numbers_ptr;
int choose_id, number_id;
// Make logfile and cstest globals
FILE *logfile = NULL;
FILE *cstest = NULL;
//Semaphore
sem_t * mutex;

void cleanup()
{
    if (cstest != NULL) fclose(cstest);
    if (logfile != NULL) fclose(logfile);

    // Detach shared memory
    shmdt(choosing_ptr);
    shmdt(numbers_ptr);
    // Remove shared memory
    shmctl(choose_id, IPC_RMID, NULL);
    shmctl(number_id, IPC_RMID, NULL);

    //Remove semaphore

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
    signal(SIGTERM, cleanup);
    int nproc = atoi(argv[2]);   // gets the number of processes
    int proc_id = atoi(argv[1]); // gets the process number
    // Attach shared memory
    printf("Inside slave process\n");
    key_t choosing_key = ftok("Makefile", 'a');
    choose_id = shmget(choosing_key, sizeof(bool) * nproc, IPC_EXCL);

    key_t numbers_key = ftok(".", 'a');
    number_id = shmget(numbers_key, sizeof(int) * nproc, IPC_EXCL);

    if (choose_id == -1)
    {
        perror("monitor.c: Error: Shared memory (buffer) could not be created");
        printf("exiting\n\n");
        // early cleanup
        exit(0);
    }

    if (number_id == -1)
    {
        perror("monitor.c: Error: Shared memory (buffer) could not be created");
        printf("exiting\n\n");
        // early cleanup
        exit(0);
    }

    // shm has been gotten
    choosing_ptr = (bool *)shmat(choose_id, 0, 0);
    numbers_ptr = (int *)shmat(number_id, 0, 0);

    //Attach semaphore
    mutex = sem_open("mutexSem", IPC_EXCL, 0644, 1);
    sem_unlink("mutexSem");

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
        //choosing_ptr[proc_id - 1] = 1;
        sem_wait(mutex);
        numbers_ptr[proc_id - 1] = 1 + maxValue(numbers_ptr, nproc);
        //choosing_ptr[proc_id - 1] = 0;
        sem_post(mutex);
        for (int j = 0; j < nproc; j++)
        {
            //while (choosing_ptr[j]);
            while (sem_wait(mutex));

            while ((numbers_ptr[j] != 0) && (numbers_ptr[j] < numbers_ptr[proc_id - 1] || (numbers_ptr[j] == numbers_ptr[proc_id - 1] && j < proc_id - 1)));
        }
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
        // Indicates that this process is complete
        numbers_ptr[proc_id - 1] = 0;
    }

    // remainder_section ??
    // Detach shared memory
    shmdt(choosing_ptr);
    shmdt(numbers_ptr);
    // Remove shared memory
    shmctl(choose_id, IPC_RMID, NULL);
    shmctl(number_id, IPC_RMID, NULL);

    //Deallocate semaphore
    sem_post(mutex);

    exit(0);
}