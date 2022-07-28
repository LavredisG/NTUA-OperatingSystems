#define _XOPEN_SOURCE 700 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

#define CYAN "\033[36m"
#define WHITE "\033[37m"

int /*volatile*/ create = 0, Index = 0; //Index refers to the ID of the child process //question for class***(volatile)-> read directly from memory and not from register
bool /*volatile*/ end = 1, PrintChildrenState = 0;
pid_t pid, F, childPID[10000];

//Error for system calls
void syscallError(int ret, const char* msg) 
{
	if (ret < 0)
	{
		perror(msg);
		exit(EXIT_FAILURE);
	}
}

void describe_wait_status(pid_t pid, int status) 
{
  syscallError(pid, WHITE "waitpid() call failed!");

  if (WIFSTOPPED(status)) 
  {
    printf(CYAN "[PARENT/PID=%d] Child with PID=%d stopped\n", getpid(), pid);
  } 
  else if (WIFEXITED(status) && end) //when end == 1 parent hasn't received SIGTERM signal
  {
	int i = 0;
	while (childPID[i] != pid) i++;
	printf(CYAN "[PARENT/PID=%d] Child %d with PID=%d exited with status code %d!\n", getpid(), i, pid, WEXITSTATUS(status));
  }
  else if (WIFEXITED(status) && !end) 
  {
	printf(CYAN "[PARENT/PID=%d] Child with PID=%d terminated successfully with exit status code %d!\n", getpid(), pid, WEXITSTATUS(status));
  }
}

void CreateChild(char* state)
{
	F = fork();
	syscallError(F, WHITE "Parent failed to create child!");

	if (F == 0) 
	{
	  char indexstring[10]; //pass Index as a string
	  sprintf(indexstring, "%d", Index);
      char *const args[] = {"./child", state, indexstring, NULL};
      execv(args[0], args);
      perror(WHITE "execv() failed!");
	  exit(EXIT_FAILURE);
	}
	childPID[Index] = F; //store children PID in an array
}

void SIGUSR1_handler(int signum)
{
	PrintChildrenState = 1; 
}

//this handler is used only when the parent hasn't received SIGTERM signal
void SIGCHLD_handler(int signum)
{
	if (end)
	{
		int status;
		pid = waitpid(-1, &status, WUNTRACED); //WUNTRACED returns immediatly if a child has stopped
		if (WIFSTOPPED(status))
		{
			describe_wait_status(pid, status);
			kill(pid, SIGCONT);
		}
		else if (pid > 0)
		{
			describe_wait_status(pid, status);
			Index = 0;
			while(pid != childPID[Index]) Index++;
			create++;
		}
	}
}

void SIGTERM_handler(int signum)
{
	end = 0;
}

int main(int argc, char** argv)
{
  int N = strlen(argv[1]); //Number of chlidren
  if (argc < 2 | argc > 2)
  {
    printf("Invalid number of Arguments!\n");
    return EXIT_FAILURE;
  }
  for(int i = 0; i < N; i++)
  {
	if (argv[1][i] != 't' && argv[1][i] != 'f')
	{
		printf("Invalid sequence of characters! Please insert a string consisting of 't' or 'f'\n");
    	return EXIT_FAILURE;
	}
  }
  struct sigaction act1, act2, act3;
  
  act1.sa_handler = SIGUSR1_handler;
  act1.sa_flags = SA_RESTART; //This flag controls what happens when a signal is delivered during certain primitives (such as open, read or write), and the signal handler returns normally. Returning from a handler resumes the library function
  syscallError(sigaction(SIGUSR1, &act1, NULL), WHITE "sigaction () failed!");

  act2.sa_handler = SIGCHLD_handler;
  act2.sa_flags = SA_RESTART;
  syscallError(sigaction(SIGCHLD, &act2, NULL), WHITE "sigaction () failed!");
  
  act3.sa_handler = SIGTERM_handler;
  act3.sa_flags = SA_RESTART;
  syscallError(sigaction(SIGTERM, &act3, NULL), WHITE "sigaction () failed!");
  
  //Creation of children
  for(int i = 0; i < N; i++)
  {
	CreateChild(argv[1]);
	printf(CYAN "[PARENT/PID=%d] Created child %d (PID=%d) and initial state '%c'\n", getpid(), Index, childPID[i], argv[1][i]);
	Index++;
  }

  while(end)
  {
	if (create)
	{
	  CreateChild(argv[1]);
	  printf(CYAN "[PARENT/PID=%d] Created new child for gate %d (PID=%d) and initial state '%c'\n", getpid(), Index, childPID[Index], argv[1][Index]);
	  create--;
	}
	if (PrintChildrenState) 
	{
		for(int i = 0; i < N; i++)
		{
			syscallError(kill(childPID[i], SIGUSR1), WHITE "Parent failed to send SIGUSR1 signal to childern!\n");
		}
		PrintChildrenState = 0;
	}	
  }
  for(int i = 0; i < N; i++)
  {
	int status;
	printf(CYAN "[PARENT/PID=%d] Waiting for %d children to exit.\n", getpid(), N - i);
	syscallError(kill(childPID[i], SIGTERM), WHITE "Parent failed to send SIGTERM signal to children!\n");
	describe_wait_status(waitpid(-1, &status, 0), status);
  }

  printf(CYAN "[PARENT/PID=%d] All children exited, terminating as well.\n", getpid());

  return 0;
}