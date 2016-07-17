#include <errno.h> 
#include <wait.h> 
#include <stdlib.h> 
#include <stdio.h>
#include <unistd.h>
//#include <curses.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/resource.h> 


/* Define semaphores to be placed in a single semaphore set */
/* Numbers indicate index in semaphore set for named semaphore */
#define SEM_COWSINGROUP 0
#define SEM_PCOWSINGROUP 1
#define SEM_COWSWAITING 2
#define SEM_PCOWSEATEN 3
#define SEM_COWSEATEN 4
#define SEM_COWSDEAD 5

#define SEM_SHEEPINGROUP 6
#define SEM_PSHEEPINGROUP 7
#define SEM_SHEEPWAITING 8
#define SEM_PSHEEPEATEN 9
#define SEM_SHEEPEATEN 10
#define SEM_SHEEPDEAD 11

#define SEM_PHUNTERCOUNTER 12
#define SEM_HUNTERWAITING 13
#define SEM_HUNTERFINISHED 14

#define SEM_PTHIEFCOUNTER 15
#define SEM_THIEVESWAITING 16
#define SEM_THIEFFINISHED 17

#define SEM_PTERMINATE 18
#define SEM_DRAGONEATING 19
#define SEM_DRAGONFIGHTING 20
#define SEM_DRAGONSLEEPING 21

#define SEM_PMEALWAITINGFLAG 22
#define SEM_PSHEEPMEALWAITINGFLAG 23

/* System constants used to control simulation termination */
#define MAX_COWS_EATEN 14
#define MAX_COWS_CREATED 80

#define MAX_SHEEP_EATEN 14
#define MAX_SHEEP_CREATED 80

#define MAX_THIEVES_WON 5
#define MAX_HUNTERS_WON 5

#define MIN_TREASURE_IN_HOARD 0
#define MAX_TREASURE_IN_HOARD 800
#define INITIAL_TREASURE_IN_HOARD 400

/* System constants to specify size of groups of cows*/
#define COWS_IN_GROUP 2
#define SHEEP_IN_GROUP 2

/* CREATING YOUR SEMAPHORES */
int semID; 

union semun
{
	int val;
	struct semid_ds *buf;
	ushort *array;
} seminfo;

struct timeval startTime;


/*  Pointers and ids for shared memory segments */

int *cowCounterp = NULL;
int *cowsEatenCounterp = NULL;
int *mealWaitingFlagp = NULL;
int cowCounter = 0;
int cowsEatenCounter = 0;
int mealWaitingFlag = 0;

int *sheepCounterp = NULL;
int *sheepEatenCounterp = NULL;
int *sheepMealWaitingFlagp = NULL;
int sheepCounter = 0;
int sheepEatenCounter = 0;
int sheepMealWaitingFlag = 0;

int *thiefCounterp = NULL;
int thiefCounter = 0;

int *hunterCounterp = NULL;
int hunterCounter = 0;

int *terminateFlagp = NULL;
int terminateFlag = 0;

/* Group IDs for managing/removing processes */
int smaugProcessID = -1;
int cowProcessGID = -1;
int sheepProcessGID = -1;
int parentProcessGID = -1;
int thiefProcessGID = -1;
int hunterProcessGID = -1;

/* Define the semaphore operations for each semaphore */
/* Arguments of each definition are: */
/* Name of semaphore on which the operation is done */
/* Increment (amount added to the semaphore when operation executes*/
/* Flag values (block when semaphore <0, enable undo ...)*/

/*Number in group semaphores*/
struct sembuf WaitCowsInGroup={SEM_COWSINGROUP, -1, 0};
struct sembuf SignalCowsInGroup={SEM_COWSINGROUP, 1, 0};

struct sembuf WaitSheepInGroup={SEM_SHEEPINGROUP, -1, 0};
struct sembuf SignalSheepInGroup={SEM_SHEEPINGROUP, 1, 0};

/*Number in group mutexes*/
struct sembuf WaitProtectCowsInGroup={SEM_PCOWSINGROUP, -1, 0};
struct sembuf SignalProtectCowsInGroup={SEM_PCOWSINGROUP, 1, 0};

struct sembuf WaitProtectMealWaitingFlag={SEM_PMEALWAITINGFLAG, -1, 0};
struct sembuf SignalProtectMealWaitingFlag={SEM_PMEALWAITINGFLAG, 1, 0};

struct sembuf WaitProtectSheepInGroup={SEM_PSHEEPINGROUP, -1, 0};
struct sembuf SignalProtectSheepInGroup={SEM_PSHEEPINGROUP, 1, 0};

struct sembuf WaitProtectSheepMealWaitingFlag={SEM_PSHEEPMEALWAITINGFLAG, -1, 0};
struct sembuf SignalProtectSheepMealWaitingFlag={SEM_PSHEEPMEALWAITINGFLAG, 1, 0};

struct sembuf WaitProtectThiefCounter={SEM_PTHIEFCOUNTER, -1, 0};
struct sembuf SignalProtectThiefCounter={SEM_PTHIEFCOUNTER, 1, 0};

struct sembuf WaitProtectHunterCounter={SEM_PHUNTERCOUNTER, -1, 0};
struct sembuf SignalProtectHunterCounter={SEM_PHUNTERCOUNTER, 1, 0};

/*Number waiting sempahores*/
struct sembuf WaitCowsWaiting={SEM_COWSWAITING, -1, 0};
struct sembuf SignalCowsWaiting={SEM_COWSWAITING, 1, 0};

struct sembuf WaitSheepWaiting={SEM_SHEEPWAITING, -1, 0};
struct sembuf SignalSheepWaiting={SEM_SHEEPWAITING, 1, 0};

struct sembuf WaitThiefWaiting={SEM_THIEVESWAITING, -1, 0};
struct sembuf SignalThiefWaiting={SEM_THIEVESWAITING, 1, 0};

struct sembuf WaitHunterWaiting={SEM_HUNTERWAITING, -1, 0};
struct sembuf SignalHunterWaiting={SEM_HUNTERWAITING, 1, 0};

/*Number eaten or fought semaphores*/
struct sembuf WaitCowsEaten={SEM_COWSEATEN, -1, 0};
struct sembuf SignalCowsEaten={SEM_COWSEATEN, 1, 0};

struct sembuf WaitSheepEaten={SEM_SHEEPEATEN, -1, 0};
struct sembuf SignalSheepEaten={SEM_SHEEPEATEN, 1, 0};

/*Number eaten or fought mutexes*/
struct sembuf WaitProtectCowsEaten={SEM_PCOWSEATEN, -1, 0};
struct sembuf SignalProtectCowsEaten={SEM_PCOWSEATEN, 1, 0};

struct sembuf WaitProtectSheepEaten={SEM_PSHEEPEATEN, -1, 0};
struct sembuf SignalProtectSheepEaten={SEM_PSHEEPEATEN, 1, 0};

/*Number Dead semaphores*/
struct sembuf WaitCowsDead={SEM_COWSDEAD, -1, 0};
struct sembuf SignalCowsDead={SEM_COWSDEAD, 1, 0};

struct sembuf WaitSheepDead={SEM_SHEEPDEAD, -1, 0};
struct sembuf SignalSheepDead={SEM_SHEEPDEAD, 1, 0};

struct sembuf waitThiefFinished={SEM_THIEFFINISHED, -1, 0};
struct sembuf signalThiefFinished={SEM_THIEFFINISHED, 1, 0};

struct sembuf waitHunterFinished={SEM_HUNTERFINISHED, -1, 0};
struct sembuf signalHunterFinished={SEM_HUNTERFINISHED, 1, 0};

/*Dragon Semaphores*/
struct sembuf WaitDragonEating={SEM_DRAGONEATING, -1, 0};
struct sembuf SignalDragonEating={SEM_DRAGONEATING, 1, 0};

struct sembuf WaitSheepDragonEating={SEM_DRAGONEATING, -1, 0};
struct sembuf SignalSheepDragonEating={SEM_DRAGONEATING, 1, 0};

struct sembuf WaitDragonFighting={SEM_DRAGONFIGHTING, -1, 0};
struct sembuf SignalDragonFighting={SEM_DRAGONFIGHTING, 1, 0};
struct sembuf WaitDragonSleeping={SEM_DRAGONSLEEPING, -1, 0};
struct sembuf SignalDragonSleeping={SEM_DRAGONSLEEPING, 1, 0};

/*Termination Mutex*/
struct sembuf WaitProtectTerminate={SEM_PTERMINATE, -1, 0};
struct sembuf SignalProtectTerminate={SEM_PTERMINATE, 1, 0};


double timeChange( struct timeval starttime );
void initialize();
void smaug(int winProbability);
void cow(int startTimeN);
void sheep(int startTimeN);
void terminateSimulation();
void releaseSemandMem();
void semopChecked(int semaphoreID, struct sembuf *operation, unsigned something); 
void semctlChecked(int semaphoreID, int semNum, int flag, union semun seminfo); 
	


void smaug(int winProbability)
{
	int k;
	int temp;
	int localpid;
	double elapsedTime;

	int numberOfJewels = INITIAL_TREASURE_IN_HOARD;

	/* local counters used only for smaug routine */
	int cowsEatenTotal = 0;
	int sheepEatenTotal = 0;
	int thievesWonTotal = 0;
	int huntersWonTotal = 0;

	int smaugSleepFlag = 0;
	int currentThiefPlayCount = 0;


	/* Initialize random number generator*/
	/* Random numbers are used to determine the time between successive beasts */
	smaugProcessID = getpid();
	printf("SMAUGSMAUGSMAUGSMAUGSMAU   PID is %d \n", smaugProcessID );
	localpid = smaugProcessID;
	printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has gone to sleep\n" );
	semopChecked(semID, &WaitDragonSleeping, 1);
	printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has woken up \n" );
	while (*terminateFlagp==0) {
		if (smaugSleepFlag == 1) {
			printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has gone to sleep\n" );
			semopChecked(semID, &WaitDragonSleeping, 1);
			printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has woken up \n" );
		}
		semopChecked(semID, &WaitProtectMealWaitingFlag, 1);
		semopChecked(semID, &WaitProtectSheepMealWaitingFlag, 1);
		if( *mealWaitingFlagp >= 1  && *sheepMealWaitingFlagp >=1 && smaugSleepFlag != 1) {
			while (*mealWaitingFlagp >= 1  && *sheepMealWaitingFlagp >=1) {
				*mealWaitingFlagp = *mealWaitingFlagp - 1;
				semopChecked(semID, &SignalProtectMealWaitingFlag, 1);

				*sheepMealWaitingFlagp = *sheepMealWaitingFlagp - 1;
				semopChecked(semID, &SignalProtectSheepMealWaitingFlag, 1);

				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug is eating a meal\n");
				for( k = 0; k < COWS_IN_GROUP; k++ ) {
					semopChecked(semID, &SignalCowsWaiting, 1);
					//printf("SMAUGSMAUGSMAUGSMAUGSMAU   A cow is ready to be eaten\n");
				}
				for( k = 0; k < SHEEP_IN_GROUP; k++ ) {
					semopChecked(semID, &SignalSheepWaiting, 1);
					//printf("SMAUGSMAUGSMAUGSMAUGSMAU   A sheep is ready to be eaten\n");
				}
				/*Smaug waits to eat*/
				semopChecked(semID, &WaitDragonEating, 1);
				semopChecked(semID, &WaitSheepDragonEating, 1);
				for( k = 0; k < COWS_IN_GROUP; k++ ) {
					semopChecked(semID, &SignalCowsDead, 1);
					cowsEatenTotal++;
					printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug finished eating %d cow\n", k+1);
				}
				for( k = 0; k < SHEEP_IN_GROUP; k++ ) {
					semopChecked(semID, &SignalSheepDead, 1);
					sheepEatenTotal++;
					printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug finished eating %d sheep\n", k+1);
				}
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has finished a meal\n");
				if(cowsEatenTotal >= MAX_COWS_EATEN && sheepEatenTotal >= MAX_SHEEP_EATEN ) {
					printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has eaten the allowed number of cows and sheep\n");
					*terminateFlagp= 1;
					break; 
				}

				/* Smaug checke to see if another snack is waiting */
				semopChecked(semID, &WaitProtectMealWaitingFlag, 1);
				semopChecked(semID, &WaitProtectSheepMealWaitingFlag, 1);
				if( *mealWaitingFlagp > 0 && *sheepMealWaitingFlagp > 0 ) {
					printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug eats again\n");
					continue;
				}
			}
		}
		semopChecked(semID, &SignalProtectSheepMealWaitingFlag, 1);
		semopChecked(semID, &SignalProtectMealWaitingFlag, 1);

		semopChecked(semID, &WaitProtectThiefCounter, 1);

		if (*thiefCounterp > 0 && smaugSleepFlag != 1) {
			while (*thiefCounterp > 0 && currentThiefPlayCount < 2) {
				currentThiefPlayCount++;
				*thiefCounterp = *thiefCounterp - 1;
				semopChecked(semID, &SignalProtectThiefCounter, 1);

				semopChecked(semID, &SignalThiefWaiting, 1);
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug is playing with a thief\n");
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug's treasure includes %d jewels\n", numberOfJewels);

				if (rand() %100 <= winProbability) {
					numberOfJewels += 20;
					thievesWonTotal += 1;

					printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug defeats a thief\n");
					printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has added to his treasure. His treasure now includes %d jewels\n", numberOfJewels);
				} 
				else {
					numberOfJewels -= 8;
					printf("SMAUGSMAUGSMAUGSMAUGSMAU   Thief defeats smaug\n");
					printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has lost some treasure. His treasure now includes %d jewels\n", numberOfJewels);
				}
				semopChecked(semID, &signalThiefFinished, 1);
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has finished a game\n");

				if (thievesWonTotal >= MAX_THIEVES_WON) {
					*terminateFlagp = 1;
					printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has won more than %d thieves. Simulation Ends.\n", MAX_THIEVES_WON);
				}
				else if (numberOfJewels >= MAX_TREASURE_IN_HOARD){
					*terminateFlagp = 1;
					printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has more than %d jewels in his treasure. Simulation Ends.\n", MAX_TREASURE_IN_HOARD);
				}
				else if (numberOfJewels <= MIN_TREASURE_IN_HOARD){
					*terminateFlagp = 1;
					printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has less than %d jewels in his treasure. Simulation Ends.\n", MIN_TREASURE_IN_HOARD);
				}
				semopChecked(semID, &WaitProtectThiefCounter, 1);
			}
			currentThiefPlayCount = 0;
			smaugSleepFlag = 1;
		}
		semopChecked(semID, &SignalProtectThiefCounter, 1);

		semopChecked(semID, &WaitProtectHunterCounter, 1);

		if (*hunterCounterp > 0 && smaugSleepFlag != 1) {
			*hunterCounterp = *hunterCounterp - 1;
			semopChecked(semID, &SignalProtectHunterCounter, 1);

			semopChecked(semID, &SignalHunterWaiting, 1);
			printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug is fighting a treasure hunter\n");
			printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug's treasure includes %d jewels\n", numberOfJewels);

			if (rand() %100 <= winProbability) {
				numberOfJewels += 5;
				huntersWonTotal += 1;

				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug defeats a treasure hunter\n");
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has added to his treasure. His treasure now includes %d jewels\n", numberOfJewels);
			} 
			else {
				numberOfJewels -= 10;
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Treasure hunter defeats smaug\n");
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has lost some treasure. His treasure now includes %d jewels\n", numberOfJewels);
			}
			semopChecked(semID, &signalHunterFinished, 1);
			printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has finished a battle\n");

			if (thievesWonTotal >= MAX_HUNTERS_WON) {
				*terminateFlagp = 1;
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has won more than %d thieves. Simulation Ends.\n", MAX_HUNTERS_WON);
			}
			else if (numberOfJewels >= MAX_TREASURE_IN_HOARD){
				*terminateFlagp = 1;
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has more than %d jewels in his treasure. Simulation Ends.\n", MAX_TREASURE_IN_HOARD);
			}
			else if (numberOfJewels <= MIN_TREASURE_IN_HOARD){
				*terminateFlagp = 1;
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has less than %d jewels in his treasure. Simulation Ends.\n", MIN_TREASURE_IN_HOARD);
			}
		}
		semopChecked(semID, &SignalProtectHunterCounter, 1);
	}
}


void initialize()
{
	/* Init semaphores */
	semID=semget(IPC_PRIVATE, 25, 0666 | IPC_CREAT);


	/* Init to zero, no elements are produced yet */
	seminfo.val=0;
	semctlChecked(semID, SEM_COWSINGROUP, SETVAL, seminfo);
	semctlChecked(semID, SEM_COWSWAITING, SETVAL, seminfo);
	semctlChecked(semID, SEM_COWSEATEN, SETVAL, seminfo);
	semctlChecked(semID, SEM_COWSDEAD, SETVAL, seminfo);

	semctlChecked(semID, SEM_SHEEPINGROUP, SETVAL, seminfo);
	semctlChecked(semID, SEM_SHEEPWAITING, SETVAL, seminfo);
	semctlChecked(semID, SEM_SHEEPEATEN, SETVAL, seminfo);
	semctlChecked(semID, SEM_SHEEPDEAD, SETVAL, seminfo);

	semctlChecked(semID, SEM_THIEFFINISHED, SETVAL, seminfo);
	semctlChecked(semID, SEM_THIEVESWAITING, SETVAL, seminfo);

	semctlChecked(semID, SEM_HUNTERFINISHED, SETVAL, seminfo);
	semctlChecked(semID, SEM_HUNTERWAITING, SETVAL, seminfo);

	semctlChecked(semID, SEM_DRAGONFIGHTING, SETVAL, seminfo);
	semctlChecked(semID, SEM_DRAGONSLEEPING, SETVAL, seminfo);
	semctlChecked(semID, SEM_DRAGONEATING, SETVAL, seminfo);
	printf("!!INIT!!INIT!!INIT!!  semaphores initiialized\n");
	
	/* Init Mutex to one */
	seminfo.val=1;
	semctlChecked(semID, SEM_PCOWSINGROUP, SETVAL, seminfo);
	semctlChecked(semID, SEM_PMEALWAITINGFLAG, SETVAL, seminfo);
	semctlChecked(semID, SEM_PCOWSEATEN, SETVAL, seminfo);

	semctlChecked(semID, SEM_PSHEEPINGROUP, SETVAL, seminfo);
	semctlChecked(semID, SEM_PSHEEPMEALWAITINGFLAG, SETVAL, seminfo);
	semctlChecked(semID, SEM_PSHEEPEATEN, SETVAL, seminfo);

	semctlChecked(semID, SEM_PTHIEFCOUNTER, SETVAL, seminfo);
	semctlChecked(semID, SEM_PHUNTERCOUNTER, SETVAL, seminfo);

	semctlChecked(semID, SEM_PTERMINATE, SETVAL, seminfo);
	printf("!!INIT!!INIT!!INIT!!  mutexes initiialized\n");


	/* Now we create and attach  the segments of shared memory*/
        if ((terminateFlag = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
                printf("!!INIT!!INIT!!INIT!!  shm not created for terminateFlag\n");
                 exit(1);
        }
        else {
                printf("!!INIT!!INIT!!INIT!!  shm created for terminateFlag\n");
        }
	if ((cowCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for cowCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm created for cowCounter\n");
	}
	if ((mealWaitingFlag = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for mealWaitingFlag\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm created for mealWaitingFlag\n");
	}
	if ((cowsEatenCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for cowsEatenCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm created for cowsEatenCounter\n");
	}

	//Sheep 

	if ((sheepCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for sheepCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm created for sheepCounter\n");
	}
	if ((sheepMealWaitingFlag = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for sheepMealWaitingFlag\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm created for sheepMealWaitingFlag\n");
	}
	if ((sheepEatenCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for sheepEatenCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm created for sheepEatenCounter\n");
	}

	//Thief Hunter

	if ((thiefCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for thiefCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm created for thiefCounter\n");
	}
	if ((hunterCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for hunterCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm created for hunterCounter\n");
	}


	/* Now we attach the segment to our data space.  */
        if ((terminateFlagp = shmat(terminateFlag, NULL, 0)) == (int *) -1) {
                printf("!!INIT!!INIT!!INIT!!  shm not attached for terminateFlag\n");
                exit(1);
        }
        else {
                 printf("!!INIT!!INIT!!INIT!!  shm attached for terminateFlag\n");
        }

	if ((cowCounterp = shmat(cowCounter, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for cowCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for cowCounter\n");
	}
	if ((mealWaitingFlagp = shmat(mealWaitingFlag, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for mealWaitingFlag\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for mealWaitingFlag\n");
	}
	if ((cowsEatenCounterp = shmat(cowsEatenCounter, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for cowsEatenCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for cowsEatenCounter\n");
	}
	//Sheep

	if ((sheepCounterp = shmat(sheepCounter, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for sheepCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for sheepCounter\n");
	}
	if ((sheepMealWaitingFlagp = shmat(sheepMealWaitingFlag, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for mealWaitingFlag\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for sheepMealWaitingFlag\n");
	}
	if ((sheepEatenCounterp = shmat(sheepEatenCounter, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for sheepEatenCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for sheepEatenCounter\n");
	}
	if ((thiefCounterp = shmat(thiefCounter, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for thiefCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for thiefCounter\n");
	}
	if ((hunterCounterp = shmat(hunterCounter, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for hunterCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for hunterCounter\n");
	}
	printf("!!INIT!!INIT!!INIT!!   initialize end\n");
}



void cow(int startTimeN)
{
	int localpid;
	int retval;
	int k;
	localpid = getpid();

	/* graze */
	printf("CCCCCCC %8d CCCCCCC   Cow with PID %d is born\n", localpid, localpid);
	if( startTimeN > 0) {
		if( usleep( startTimeN) == -1){
			/* exit when usleep interrupted by kill signal */
			if(errno==EINTR)exit(4);
		}	
	}
	printf("CCCCCCC %8d CCCCCCC   Cow with PID %d grazes for %f ms\n", localpid, localpid, startTimeN/1000.0);


	/* does this beast complete a group of BEASTS_IN_GROUP ? */
	/* if so wake up the dragon */
	semopChecked(semID, &WaitProtectCowsInGroup, 1);
	semopChecked(semID, &SignalCowsInGroup, 1);
	*cowCounterp = *cowCounterp + 1;
	printf("CCCCCCC %8d CCCCCCC   Cow with PID %d has been enchanted\n", localpid, localpid);
	printf("CCCCCCC %8d CCCCCCC   %d  cows have been enchanted \n", localpid, *cowCounterp );
	if( ( *cowCounterp  >= COWS_IN_GROUP )) {
		*cowCounterp = *cowCounterp - COWS_IN_GROUP;
		for (k=0; k<COWS_IN_GROUP; k++){
			semopChecked(semID, &WaitCowsInGroup, 1);
		}
		printf("CCCCCCC %8d CCCCCCC   The last cow (PID %d) in a snack is waiting\n", localpid, localpid);
		semopChecked(semID, &SignalProtectCowsInGroup, 1);

		semopChecked(semID, &WaitProtectMealWaitingFlag, 1);
		*mealWaitingFlagp = *mealWaitingFlagp + 1;
		semopChecked(semID, &SignalProtectMealWaitingFlag, 1);

		semopChecked(semID, &WaitProtectSheepMealWaitingFlag, 1);
		if (*sheepMealWaitingFlagp >= 1){
			semopChecked(semID, &SignalDragonSleeping, 1);
			printf("CCCCCCC %8d CCCCCCC   The last cow (PID %d) in a snack wakes the dragon \n", localpid, localpid);

		}
		semopChecked(semID, &SignalProtectSheepMealWaitingFlag, 1);

	}
	else
	{
		semopChecked(semID, &SignalProtectCowsInGroup, 1);
	}

	semopChecked(semID, &WaitCowsWaiting, 1);

	/* have all the beasts in group been eaten? */
	/* if so wake up the dragon */
	semopChecked(semID, &WaitProtectCowsEaten, 1);
	semopChecked(semID, &SignalCowsEaten, 1);
	*cowsEatenCounterp = *cowsEatenCounterp + 1;
	if( ( *cowsEatenCounterp >= COWS_IN_GROUP )) {
		*cowsEatenCounterp = *cowsEatenCounterp - COWS_IN_GROUP;
		for (k=0; k<COWS_IN_GROUP; k++){
       		        semopChecked(semID, &WaitCowsEaten, 1);
		}
		printf("CCCCCCC %8d CCCCCCC   Cow with PID %d is waiting to be eaten\n", localpid, localpid);
		semopChecked(semID, &SignalProtectCowsEaten, 1);
		semopChecked(semID, &SignalDragonEating, 1);
	}
	else
	{
		semopChecked(semID, &SignalProtectCowsEaten, 1);
		printf("CCCCCCC %8d CCCCCCC   Cow with PID %d is waiting to be eaten\n", localpid, localpid);
	}
	semopChecked(semID, &WaitCowsDead, 1);

	printf("CCCCCCC %8d CCCCCCC   Cow with PID %d dies\n", localpid, localpid);
}

void sheep(int startTimeN)
{
	int localpid;
	int retval;
	int k;
	localpid = getpid();

	/* graze */
	printf("SSSSSSS %8d SSSSSSS   Sheep with PID %d is born\n", localpid, localpid);
	if( startTimeN > 0) {
		if( usleep( startTimeN) == -1){
			/* exit when usleep interrupted by kill signal */
			if(errno==EINTR)exit(4);
		}	
	}
	printf("SSSSSSS %8d SSSSSSS   Sheep with PID %d grazes for %f ms\n", localpid, localpid, startTimeN/1000.0);


	/* does this beast complete a group of BEASTS_IN_GROUP ? */
	/* if so wake up the dragon */
	semopChecked(semID, &WaitProtectSheepInGroup, 1);
	semopChecked(semID, &SignalSheepInGroup, 1);
	*sheepCounterp = *sheepCounterp + 1;
	printf("SSSSSSS %8d SSSSSSS   Sheep with PID %d has been enchanted\n", localpid, localpid);
	printf("SSSSSSS %8d SSSSSSS   %d  sheep have been enchanted \n", localpid, *sheepCounterp );
	if( ( *sheepCounterp  >= SHEEP_IN_GROUP )) {
		*sheepCounterp = *sheepCounterp - SHEEP_IN_GROUP;
		for (k=0; k<SHEEP_IN_GROUP; k++){
			semopChecked(semID, &WaitSheepInGroup, 1);
		}
		printf("SSSSSSS %8d SSSSSSS   The last sheep (PID %d) in a snack is waiting\n", localpid, localpid);
		semopChecked(semID, &SignalProtectSheepInGroup, 1);

		semopChecked(semID, &WaitProtectSheepMealWaitingFlag, 1);
		*sheepMealWaitingFlagp = *sheepMealWaitingFlagp + 1;
		// printf("CCCCCCC %8d CCCCCCC   signal meal flag %d\n", localpid, *mealWaitingFlagp);
		semopChecked(semID, &SignalProtectSheepMealWaitingFlag, 1);

		semopChecked(semID, &WaitProtectMealWaitingFlag, 1);

		if (*mealWaitingFlagp >= 1){
			semopChecked(semID, &SignalDragonSleeping, 1);
			printf("SSSSSSS %8d SSSSSSS   The last sheep (PID %d) in a snack wakes the dragon \n", localpid, localpid);
		}
		semopChecked(semID, &SignalProtectMealWaitingFlag, 1);
		
	}
	else
	{
		semopChecked(semID, &SignalProtectSheepInGroup, 1);
	}

	semopChecked(semID, &WaitSheepWaiting, 1);

	/* have all the beasts in group been eaten? */
	/* if so wake up the dragon */
	semopChecked(semID, &WaitProtectSheepEaten, 1);
	semopChecked(semID, &SignalSheepEaten, 1);
	*sheepEatenCounterp = *sheepEatenCounterp + 1;
	if( ( *sheepEatenCounterp >= SHEEP_IN_GROUP )) {
		*sheepEatenCounterp = *sheepEatenCounterp - SHEEP_IN_GROUP;
		for (k=0; k<SHEEP_IN_GROUP; k++){
       		        semopChecked(semID, &WaitSheepEaten, 1);
		}
		printf("SSSSSSS %8d SSSSSSS   Sheep with PID %d is waiting to be eaten\n", localpid, localpid);
		semopChecked(semID, &SignalProtectSheepEaten, 1);
		semopChecked(semID, &SignalSheepDragonEating, 1);
	}
	else
	{
		semopChecked(semID, &SignalProtectSheepEaten, 1);
		printf("SSSSSSS %8d SSSSSSS   Sheep with PID %d is waiting to be eaten\n", localpid, localpid);
	}
	semopChecked(semID, &WaitSheepDead, 1);

	printf("SSSSSSS %8d SSSSSSS   Sheep with PID %d dies\n", localpid, localpid);
}


void thief(int startTimeN)
{
	int localpid;
	int retval;
	int k;
	localpid = getpid();

	printf("THIEFTH %8d THIEFTH   Thief with PID %d has found the path to the valley\n", localpid, localpid);
	if( startTimeN > 0) {
		if( usleep( startTimeN) == -1){
			/* exit when usleep interrupted by kill signal */
			if(errno==EINTR)exit(4);
		}	
	}

	semopChecked(semID, &WaitProtectThiefCounter, 1);
	*thiefCounterp = *thiefCounterp + 1;
	semopChecked(semID, &SignalProtectThiefCounter, 1);

	semopChecked(semID, &SignalDragonSleeping, 1);
	printf("THIEFTH %8d THIEFTH   Thief with PID %d has entered the valley and is waiting for smaug\n", localpid, localpid);

	semopChecked(semID, &WaitThiefWaiting, 1);
	printf("THIEFTH %8d THIEFTH   Thief with PID %d has entered smaug's cave\n", localpid, localpid);

	semopChecked(semID, &waitThiefFinished, 1);
	printf("THIEFTH %8d THIEFTH   Thief with PID %d leaves\n", localpid, localpid);	
}

void hunter(int startTimeN)
{
	int localpid;
	int retval;
	int k;
	localpid = getpid();

	printf("HUNTERH %8d HUNTERH   Treasure hunter with PID %d has found the path to the valley\n", localpid, localpid);
	if( startTimeN > 0) {
		if( usleep( startTimeN) == -1){
			/* exit when usleep interrupted by kill signal */
			if(errno==EINTR)exit(4);
		}	
	}

	semopChecked(semID, &WaitProtectHunterCounter, 1);
	*hunterCounterp = *hunterCounterp + 1;
	semopChecked(semID, &SignalProtectHunterCounter, 1);
	printf("HUNTERH %8d HUNTERH   Treasure hunter with PID %d has entered the valley\n", localpid, localpid);

	semopChecked(semID, &SignalDragonSleeping, 1);
	semopChecked(semID, &WaitHunterWaiting, 1);

	printf("HUNTERH %8d HUNTERH   Treasure hunter with PID %d has entered smaug's cave\n", localpid, localpid);

	semopChecked(semID, &waitHunterFinished, 1);
	printf("HUNTERH %8d HUNTERH   Treasure hunter with PID %d leaves\n", localpid, localpid);	
}

void terminateSimulation() {
	pid_t localpgid;
	pid_t localpid;
	int w = 0;
	int status;

	localpid = getpid();
	printf("RELEASESEMAPHORES   Terminating Simulation from process %8d\n", localpgid);
	if(cowProcessGID != (int)localpgid ){
		if(killpg(cowProcessGID, SIGKILL) == -1 && errno == EPERM) {
			printf("XXTERMINATETERMINATE   COWS NOT KILLED\n");
		}
		printf("XXTERMINATETERMINATE   killed cows \n");
	}

	//Sheep

	if(sheepProcessGID != (int)localpgid ){
		if(killpg(sheepProcessGID, SIGKILL) == -1 && errno == EPERM) {
			printf("XXTERMINATETERMINATE   SHEEP NOT KILLED\n");
		}
		printf("XXTERMINATETERMINATE   killed sheep \n");
	}
	if(smaugProcessID != (int)localpgid ) {
		kill(smaugProcessID, SIGKILL);
		printf("XXTERMINATETERMINATE   killed smaug\n");
	}
	while( (w = waitpid( -1, &status, WNOHANG)) > 1){
			printf("                           REAPED process in terminate %d\n", w);
	}
	releaseSemandMem();
	printf("GOODBYE from terminate\n");
}

void releaseSemandMem() 
{
	pid_t localpid;
	int w = 0;
	int status;

	localpid = getpid();

	//should check return values for clean termination
	semctl(semID, 0, IPC_RMID, seminfo);


	// wait for the semaphores 
	usleep(2000);
	while( (w = waitpid( -1, &status, WNOHANG)) > 1){
			printf("                           REAPED process in terminate %d\n", w);
	}
	printf("\n");
        if(shmdt(terminateFlagp)==-1) {
                printf("RELEASERELEASERELEAS   terminateFlag share memory detach failed\n");
        }
        else{
                printf("RELEASERELEASERELEAS   terminateFlag share memory detached\n");
        }
        if( shmctl(terminateFlag, IPC_RMID, NULL ))
        {
                printf("RELEASERELEASERELEAS   share memory delete failed %d\n",*terminateFlagp );
        }
        else{
                printf("RELEASERELEASERELEAS   share memory deleted\n");
        }
	if( shmdt(cowCounterp)==-1)
	{
		printf("RELEASERELEASERELEAS   cowCounterp memory detach failed\n");
	}
	else{
		printf("RELEASERELEASERELEAS   cowCounterp memory detached\n");
	}
	if( shmctl(cowCounter, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   cowCounter memory delete failed \n");
	}
	else{
		printf("RELEASERELEASERELEAS   cowCounter memory deleted\n");
	}
	if( shmdt(mealWaitingFlagp)==-1)
	{
		printf("RELEASERELEASERELEAS   mealWaitingFlagp memory detach failed\n");
	}
	else{
		printf("RELEASERELEASERELEAS   mealWaitingFlagp memory detached\n");
	}
	if( shmctl(mealWaitingFlag, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   mealWaitingFlag share memory delete failed \n");
	}
	else{
		printf("RELEASERELEASERELEAS   mealWaitingFlag share memory deleted\n");
	}
	if( shmdt(cowsEatenCounterp)==-1)
	{
		printf("RELEASERELEASERELEAS   cowsEatenCounterp memory detach failed\n");
	}
	else{
		printf("RELEASERELEASERELEAS   cowsEatenCounterp memory detached\n");
	}
	if( shmctl(cowsEatenCounter, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   cowsEatenCounter memory delete failed \n");
	}
	else{
		printf("RELEASERELEASERELEAS   cowsEatenCounter memory deleted\n");
	}

	//Sheep

	if( shmdt(sheepCounterp)==-1)
	{
		printf("RELEASERELEASERELEAS   sheepCounterp memory detach failed\n");
	}
	else{
		printf("RELEASERELEASERELEAS   sheepCounterp memory detached\n");
	}
	if( shmctl(sheepCounter, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   sheepCounter memory delete failed \n");
	}
	else{
		printf("RELEASERELEASERELEAS   sheepCounter memory deleted\n");
	}
	if( shmdt(sheepMealWaitingFlagp)==-1)
	{
		printf("RELEASERELEASERELEAS   sheepMealWaitingFlagp memory detach failed\n");
	}
	else{
		printf("RELEASERELEASERELEAS   sheepMealWaitingFlagp memory detached\n");
	}
	if( shmctl(sheepMealWaitingFlag, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   sheepMealWaitingFlag share memory delete failed \n");
	}
	else{
		printf("RELEASERELEASERELEAS   sheepMealWaitingFlag share memory deleted\n");
	}
	if( shmdt(sheepEatenCounterp)==-1)
	{
		printf("RELEASERELEASERELEAS   sheepEatenCounterp memory detach failed\n");
	}
	else{
		printf("RELEASERELEASERELEAS   sheepEatenCounterp memory detached\n");
	}
	if( shmctl(sheepEatenCounter, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   sheepEatenCounter memory delete failed \n");
	}
	else{
		printf("RELEASERELEASERELEAS   sheepEatenCounter memory deleted\n");
	}
	if( shmdt(thiefCounterp)==-1)
	{
		printf("RELEASERELEASERELEAS   thiefCounterp memory detach failed\n");
	}
	else{
		printf("RELEASERELEASERELEAS   thiefCounterp memory detached\n");
	}
	if( shmctl(thiefCounter, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   thiefCounter memory delete failed \n");
	}
	else{
		printf("RELEASERELEASERELEAS   thiefCounter memory deleted\n");
	}
	if( shmdt(hunterCounterp)==-1)
	{
		printf("RELEASERELEASERELEAS   hunterCounterp memory detach failed\n");
	}
	else{
		printf("RELEASERELEASERELEAS   hunterCounterp memory detached\n");
	}
	if( shmctl(hunterCounter, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   hunterCounter memory delete failed \n");
	}
	else{
		printf("RELEASERELEASERELEAS   hunterCounter memory deleted\n");
	}

}

void semctlChecked(int semaphoreID, int semNum, int flag, union semun seminfo) { 
	/* wrapper that checks if the semaphore control request has terminated */
	/* successfully. If it has not the entire simulation is terminated */

	if (semctl(semaphoreID, semNum, flag,  seminfo) == -1 ) {
		if(errno != EIDRM) {
			printf("semaphore control failed: simulation terminating\n");
			printf("errno %8d \n",errno );
			*terminateFlagp = 1;
			releaseSemandMem();
			exit(2);
		}
		else {
			exit(3);
		}
	}
}

void semopChecked(int semaphoreID, struct sembuf *operation, unsigned something) 
{

	/* wrapper that checks if the semaphore operation request has terminated */
	/* successfully. If it has not the entire simulation is terminated */
	if (semop(semaphoreID, operation, something) == -1 ) {
		if(errno != EIDRM) {
			printf("semaphore operation failed: simulation terminating\n");
			*terminateFlagp = 1;
			releaseSemandMem();
			exit(2);
		}
		else {
			exit(3);
		}
	}
}


double timeChange( const struct timeval startTime )
{
	struct timeval nowTime;
	double elapsedTime;

	gettimeofday(&nowTime,NULL);
	elapsedTime = (nowTime.tv_sec - startTime.tv_sec)*1000.0;
	elapsedTime +=  (nowTime.tv_usec - startTime.tv_usec)/1000.0;
	return elapsedTime;

}

int userInput(char *text){
	printf("Enter the value for %s: ", text);
	int input=0;
	scanf("%d", &input);
	return input;
}

int main(){
	initialize();

	const int seed = 2;
	const long int maximumCowInterval = userInput("maximumCowInterval");
	const long int maximumSheepInterval = userInput("maximumSheepInterval");
	const long int maximumHunterInterval = userInput("maximumHunterInterval");
	const long int maximumThiefInterval = userInput("maximumThiefInterval");
	const int winProb = userInput("winProb");

	srand(seed);

	double cowTimer = 0;
	double sheepTimer = 0;
	double thiefTimer = 0;
	double hunterTimer = 0;

	int parentProcessID = getpid();

	int childPID = fork();

	if (childPID < 0){
		printf("Error occured while creating a child process");
		return 1;

	}
	else if (childPID ==0){
		smaug(winProb);
		return 0;

	}
	smaugProcessID = childPID;
	gettimeofday(&startTime, NULL);

	int zombieProcessCounter = 0;
	while(*terminateFlagp == 0){
		zombieProcessCounter++;
		double simDuration = timeChange(startTime);


		/*if (cowTimer <= simDuration){
			double cowStartTime = (rand() %maximumCowInterval) / 1000.0;
			cowTimer = simDuration + cowStartTime;
			int childPID = fork();
			if (childPID == 0){
				cow(cowStartTime);
				return 0;
			}

		}

		if (sheepTimer <= simDuration){
			double sheepStartTime = (rand() %maximumSheepInterval) / 1000.0;
			sheepTimer = simDuration + sheepStartTime;
			int childPID = fork();
			if (childPID == 0){
				sheep(sheepStartTime);
				return 0;
			}

		}*/

		if (thiefTimer <= simDuration){
			double thiefStartTime = (rand() %maximumThiefInterval) / 1000.0;
			thiefTimer = simDuration + thiefStartTime;
			int childPID = fork();
			if (childPID == 0){
				thief(thiefStartTime);
				return 0;
			}

		}

		if (hunterTimer <= simDuration){
			double hunterStartTime = (rand() %maximumHunterInterval) / 1000.0;
			hunterTimer = simDuration + hunterStartTime;
			int childPID = fork();
			if (childPID == 0){
				hunter(hunterStartTime);
				return 0;
			}

		}

		if (zombieProcessCounter%5 == 0){
			zombieProcessCounter -= 5;
			int wPID = 0;
			int waitStatus = 0;
			while ((wPID = waitpid(-1, &waitStatus, WNOHANG))>1){
				printf("ZOMBIEZOMBIEZOMBIEZOMBIE   Reaped zombie process %d\n", wPID);
			}

		}
	}
	
	terminateSimulation();
	return 0;

}