#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#define NUM_LOOPS 25

void ParentProcess(int *sharedMem);
void ChildProcess(int *sharedMem);

int main() {
    int ShmID;
    int *ShmPTR;
    pid_t pid;
    int status;

    // Create shared memory segment of two integers: BankAccount and Turn
    ShmID = shmget(IPC_PRIVATE, 2 * sizeof(int), IPC_CREAT | 0666);
    if (ShmID < 0) {
        printf("*** shmget error ***\n");
        exit(1);
    }
    printf("Server has received a shared memory segment for two integers...\n");

    // Attach shared memory to server's address space
    ShmPTR = (int *) shmat(ShmID, NULL, 0);
    if (*ShmPTR == -1) {
        printf("*** shmat error ***\n");
        exit(1);
    }
    printf("Server has attached the shared memory...\n");

    // Initialize BankAccount and Turn to 0
    ShmPTR[0] = 0;  // BankAccount
    ShmPTR[1] = 0;  // Turn (0 for Parent, 1 for Child)

    printf("Server has initialized BankAccount to %d and Turn to %d\n", ShmPTR[0], ShmPTR[1]);

    // Fork a child process
    pid = fork();
    if (pid < 0) {
        printf("*** fork error ***\n");
        exit(1);
    } else if (pid == 0) {
        // Child process runs ChildProcess()
        ChildProcess(ShmPTR);
        exit(0);
    } else {
        // Parent process runs ParentProcess()
        ParentProcess(ShmPTR);
    }

    // Wait for child process to complete
    wait(&status);
    printf("Server has detected the completion of its child...\n");

    // Detach and remove shared memory
    shmdt((void *) ShmPTR);
    shmctl(ShmID, IPC_RMID, NULL);
    printf("Server has removed its shared memory...\n");
    printf("Server exits...\n");

    return 0;
}

void ParentProcess(int *sharedMem) {
    int account;
    srand(time(NULL));  // Seed random number generator

    for (int i = 0; i < NUM_LOOPS; i++) {
        sleep(rand() % 6);  // Sleep between 0-5 seconds
        account = sharedMem[0];  // Read the BankAccount

        // Busy wait while Turn != 0
        while (sharedMem[1] != 0);

        if (account <= 100) {
            int balance = rand() % 101;  // Random deposit between 0 and 100
            if (balance % 2 == 0) {
                account += balance;
                printf("Dear old Dad: Deposits $%d / Balance = $%d\n", balance, account);
            } else {
                printf("Dear old Dad: Doesn't have any money to give\n");
            }
            sharedMem[0] = account;  // Update BankAccount
        } else {
            printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n", account);
        }

        sharedMem[1] = 1;  // Set Turn to 1 (for Child)
    }
}

void ChildProcess(int *sharedMem) {
    int account;
    srand(time(NULL) + getpid());  // Seed random number generator differently for child

    for (int i = 0; i < NUM_LOOPS; i++) {
        sleep(rand() % 6);  // Sleep between 0-5 seconds
        account = sharedMem[0];  // Read the BankAccount

        // Busy wait while Turn != 1
        while (sharedMem[1] != 1);

        int balance = rand() % 51;  // Random withdrawal between 0 and 50
        printf("Poor Student needs $%d\n", balance);

        if (balance <= account) {
            account -= balance;
            printf("Poor Student: Withdraws $%d / Balance = $%d\n", balance, account);
        } else {
            printf("Poor Student: Not Enough Cash ($%d)\n", account);
        }

        sharedMem[0] = account;  // Update BankAccount
        sharedMem[1] = 0;        // Set Turn to 0 (for Parent)
    }
}
