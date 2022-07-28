#define _XOPEN_SOURCE 700 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <stdbool.h>
#include <signal.h>

#define WHITE "\033[37m"
#define MAGENTA "\033[35m"
#define GREEN "\033[32m"
#define RED "\033[31;1m"

bool /*volatile*/ state;
int ID;
pid_t time_0;
//Error for system calls
void syscallError(int ret, const char* msg) 
{
	if (ret < 0)
	{
		perror(msg);
		exit(EXIT_FAILURE);
	}
}

void ChildWork()
{
	if (state)
	{
	  printf(GREEN "[ID=%d/PID=%d/TIME=%lds] The gates are open!\n", ID, getpid(), time(NULL) - time_0);
	}
	else
	{
	  printf(RED "[ID=%d/PID=%d/TIME=%lds] The gates are closed!\n", ID, getpid(), time(NULL) - time_0);
	}
}

//Prints its' current state
void SIGUSR1_handler(int signum)
{
	ChildWork();
}

//Changes its' gate state and prints the new state
void SIGUSR2_handler(int signum)
{
	state = !state;	
	ChildWork();
}

void SIGTERM_handler(int signum)
{
	exit(0);
}

void SIGALRM_handler(int signum)
{
	ChildWork();
	alarm(15);
}

int main(int argc, char* argv[]) //argv[1] == "tftft", argv[2] == gate (ID of child)
{
	sleep(1);
	ID = atoi(argv[2]); 
	state = argv[1][ID] == 't';
	struct sigaction act1, act2, act3, act4;
	
	act1.sa_handler = SIGUSR1_handler;
	act1.sa_flags = SA_RESTART;
	syscallError(sigaction(SIGUSR1, &act1, NULL), WHITE "[SIGUSR1] sigaction () failed!");

	act2.sa_handler = SIGUSR2_handler;
	act2.sa_flags = SA_RESTART;
	syscallError(sigaction(SIGUSR2, &act2, NULL), WHITE "[SIGUSR2] sigaction () failed!");

	act3.sa_handler = SIGALRM_handler;
	act3.sa_flags = SA_RESTART;
	syscallError(sigaction(SIGALRM, &act3, NULL), WHITE "[SIGALRM] sigaction () failed!");

	act4.sa_handler = SIGTERM_handler;
	act4.sa_flags = SA_RESTART;
	syscallError(sigaction(SIGTERM, &act4, NULL), WHITE "[SIGTERM] sigaction () failed!");

	time_0 = time(NULL);
	ChildWork();
	alarm(15);

	while(1)
	{
		sleep(15); //sleep makes process sleep until a signal is received (and isn't ignored)
	}
}