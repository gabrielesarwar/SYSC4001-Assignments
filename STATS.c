#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include "com.h"
#include <stdlib.h>
#include <sys/shm.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>
#include <sys/sem.h>

#define MICRO_SEC_IN_SEC 1000000
#define MATRIX_SIZE 5
#define NUM_SEMAPHORES 3

static int bin_sem[NUM_SEMAPHORES];

// initializes the semaphore
static int set_semvalue(int i)
{
    union semun sem_union;
    sem_union.val = 1;
    if (semctl(bin_sem[i], 0, SETVAL, sem_union) == -1) return(0);
    return(1);
}

// remove semaphore value
static void del_semvalue(int i)
{
    union semun sem_union;
    if (semctl(bin_sem[i-1], 0, IPC_RMID, sem_union) == -1)
    fprintf(stderr, "Failed to delete semaphore\n");
}

// changes the semaphore by -1 (wait operation)
static int semaphore_p(int i)
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = -1; /* P() */
    sem_b.sem_flg = SEM_UNDO;
    if (semop(bin_sem[i-1], &sem_b, 1) == -1) {
        fprintf(stderr, "semaphore_p failed\n");
        return(0);
    }
    return(1);
}

// changes the semaphore by 1 (release operation)
static int semaphore_v(int i)
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = 1; /* V() */
    sem_b.sem_flg = SEM_UNDO;
    if (semop(bin_sem[i-1], &sem_b, 1) == -1) {
        fprintf(stderr, "semaphore_v failed\n");
        return(0);
    }
    return(1);
}


int main(){
   
   int pid = getpid(); 	// pid for identifying parent or child process
   int i=0;
   int exit_code;
   void *shared_memory = (void *)0;
   struct shared_matrices *shared_stuff;
   int shmid;
   int debug = 0;
    
   for(int k =0; k<NUM_SEMAPHORES; k++){
      bin_sem[k] = semget((key_t)1234+k, 1, 0666 | IPC_CREAT);
      if (!set_semvalue(k)) {
         fprintf(stderr, "Failed to initialize semaphore\n");
         exit(EXIT_FAILURE);
      }
    }

   /* create the shared memory segment */
   shmid = shmget((key_t)1234, sizeof(struct shared_matrices), 0666 | IPC_CREAT);
   if (shmid == -1) {
      fprintf(stderr, "shmget failed\n");
      exit(EXIT_FAILURE);
   }

   /* make the shared memory accessible to the program */
   shared_memory = shmat(shmid, (void *)0, 0);
   if (shared_memory == (void *)-1) {
      fprintf(stderr, "shmat failed\n");
      exit(EXIT_FAILURE);
   }

   /* assigns the shared_memory segment to shared_stuff */
   shared_stuff = (struct shared_matrices *)shared_memory;

    /* user to specify if debug mode or not */
   char input[20];
   do{
      printf("Debug mode? (y/n): ");
      scanf("%s", input);
      if(input[0] == 'y'){	// check if input is y (debug mode)
          debug = 1;
      }
   }while(!(input[0] == 'y' || input[0] =='n'));
    
    /* user to specify if debug mode or not */
    
    printf("Enter 5 distinct integers...\n");
    
   
   for(int k=0; k<MATRIX_SIZE; k++){
     int done = 0;
      do{
         char intInput[20];
         printf("Enter integer : ");
         scanf("%s", intInput);
         if(isdigit(intInput[0])){	// check if input an integer
            shared_stuff->B[k] = intInput[0] - '0';
            done = 1;
         }else{
            printf("Invalid! try again\n");
         }
      }while(done == 0);
   }
    
   printf("fork program starting\n");

   /* parent process forks 3 child processes */
   for(int index = 0; index < 3; index ++){
      if(pid > 0){
         pid = fork();
         i++;	// process num
      }	
   }
  
   /* different tasks depending on if child or parent process */
   switch(pid)
   {
      case -1:
         perror("fork failed");
         exit(1);
      case 0:	// child process
         if(i == 1){        // process 1
            if (!semaphore_p(1))exit(EXIT_FAILURE);
            if(shared_stuff->B[0] < shared_stuff->B[1]){
               int temp = shared_stuff->B[0];
                shared_stuff->B[0] = shared_stuff->B[1];
                shared_stuff->B[1] = temp;
                if(debug)printf("Process P1: performed swapping\n");
            }else{
                if(debug)printf("Process P1: No swapping\n");
            }
             if(!semaphore_v(1)) exit(EXIT_FAILURE);

           
         }else if(i == 2){  // process 2
             if(!semaphore_p(1)) exit(EXIT_FAILURE);
             if(!semaphore_p(2)) exit(EXIT_FAILURE);
             if(shared_stuff->B[1] < shared_stuff->B[2]){
                 int temp = shared_stuff->B[1];
                 shared_stuff->B[1] = shared_stuff->B[2];
                 shared_stuff->B[2] = temp;
                 if(debug)printf("Process P2: performed swapping\n");
             }else{
                 if(debug)printf("Process P2: No swapping\n");
             }
             if(!semaphore_v(1)) exit(EXIT_FAILURE);
             if(!semaphore_v(2)) exit(EXIT_FAILURE);
         }else if(i == 3){  // process 3
             if(!semaphore_p(2)) exit(EXIT_FAILURE);
             if(!semaphore_p(3)) exit(EXIT_FAILURE);
             if(shared_stuff->B[2] < shared_stuff->B[3]){
                 int temp = shared_stuff->B[2];
                 shared_stuff->B[2] = shared_stuff->B[3];
                 shared_stuff->B[3] = temp;
                 if(debug)printf("Process P3: performed swapping\n");
             }else{
                 if(debug)printf("Process P3: No swapping\n");
             }
             if(!semaphore_v(2)) exit(EXIT_FAILURE);
             if(!semaphore_v(3)) exit(EXIT_FAILURE);
         }
         exit_code = 37;
         break;
      default:	// parent process (process 4)
       if(!semaphore_p(3)) exit(EXIT_FAILURE);
        if(shared_stuff->B[3] < shared_stuff->B[4]){
           int temp = shared_stuff->B[3];
           shared_stuff->B[3] = shared_stuff->B[4];
            shared_stuff->B[4] = temp;
            if(debug) printf("Process P4: performed swapping\n");
        }else{
            if(debug) printf("Process P4: No swapping\n");
        }
       if(!semaphore_v(3)) exit(EXIT_FAILURE);
         exit_code = 0;
         break;
      }


   /* parent process waits for computation to finish */
   if(pid != 0) {
      int stat_val;
      pid_t child_pid;
	
       for(int k= 0; k<3; k++){
           child_pid = wait(&stat_val);
       }
       
       printf("Sorted list of integers: ");
       for(int k = 0; k<MATRIX_SIZE; k++){
           printf(" %d", shared_stuff->B[k]);
       }
       printf("\n");
       
       del_semvalue(1);
       del_semvalue(2);
       del_semvalue(3);
            /* shared memory is detached and then deleted */
      if (shmdt(shared_memory) == -1) {
         fprintf(stderr, "shmdt failed\n");
         exit(EXIT_FAILURE);
      }
      if (shmctl(shmid, IPC_RMID, 0) == -1) {
         fprintf(stderr, "shmctl(IPC_RMID) failed\n");
         exit(EXIT_FAILURE);
      }
   }
   
   exit(exit_code);
	
}
