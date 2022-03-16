Author: Ryan Kelly

Class: Computer Science 4760 - Operating Systems

School: University of Missouri - Saint Louis

Assignment: Project 3 

Due Date: 3/17/2022

Language Used: C

Description: A process named master forks a specified amount of children using getopt and commandline parameters. The master initializes a semaphore for the children to use. The master forks the children and waits for them to finish. The children use a binary semaphore to take turns accessing cstest, the file they write to. Each child also writes to its own logfile. After each child has written to cstest 5 times, they close naturally. The children can be forced to close early if an alarm goes off, or if the master recieves a CTRL-C. The master also creates a logfile to log when it begins, when it forks children, when it cleans up the semaphore and files, and when it exits. Timeout will default to 100, and number of processes will default to 20 if you enter a higher amount.

Build Instructions (Linux Terminal): make
Delete master and slave Executables, Logfiles, and Cstest: make clean
How to Invoke: ./master -t (Seconds until Timeout) (Number of Processes)
 