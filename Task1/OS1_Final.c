#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>                     
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#define BUFFER_SIZE 5000

#define RED "\033[31;1m"
#define GREEN "\033[32m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"

void Task(char* member, char* color, int repeats, int initiate,int file_des)
{

	if (member == "Parent")
	{
		//Create an array with message to be written in the file
            	char msgp[strlen("Message from xxxxx\n")];
             	snprintf(msgp, strlen("Message from xxxxx\n"), "Message from %d\n", getpid());

		for(int i = initiate; i <= repeats; i++)
		{
			printf("%s" "[%s] Heartbeat Pid = %d Time = %ld\n", color, member, getpid(), (long int) time(NULL));
			if(write(file_des, msgp, strlen(msgp)) == -1)
			{
				printf("%s""[%s] Failed to write to the file!\n", color, member);
				exit(0);
			}
			sleep(1);
		}
		printf("%s""[%s] Waiting for children...\n", color, member);
  		printf("%s""[%s] Child with PID = %d terminated\n", color, member, wait(NULL));
		printf("%s""[%s] Waiting for remaining child...\n", color, member);
    	printf("%s""[%s] Child with PID = %d terminated\n", color, member, wait(NULL));
    	printf("%s""[%s] PID = %d Reading file:\n", color, member, getpid());
	}
	else
	{
	  	//Create an array with message to be written in the file
               char msgc[strlen("Message from xxxxx\n")];
               snprintf(msgc, strlen("Message from xxxxx\n"), "Message from %d\n", getpid());

		printf("%s" "[%s] Started.PID = %d PPID = %d\n", color, member, getpid(), getppid());
		for(int i = initiate; i <= repeats; i = i + 2)
		{
			printf("%s" "[%s] Heartbeat PID = %d Time = %ld x = %d\n", color, member, getpid(), (long int) time(NULL), i);
			if(write(file_des, msgc, strlen(msgc)) == -1)
			{
				printf("%s""[%s] Failed to write to the file!\n", color, member);
				exit(0);
			}
			sleep(1);
		}
		printf("%s" "[%s] Terminating!\n", color, member);
		exit(0);
	}
	return;
}

void file_manipulation(int file_des, char* buf)
{
	int n_read, n_write;
	do
	{
		n_read = read(file_des, buf, sizeof(buf));
		if(n_read == -1)
		{
			printf("Error reading the file!\n");
		}

		n_write = write(1, buf, n_read);                            //1 refers to STD_OUT
		if(n_write == -1)
		{
			printf("Error writing to the file!\n");
		}
	} while(n_read > 0);								//If the file offset is at or past the end of file,  no bytes are read, and read() returns zero.
}

char buffer[BUFFER_SIZE];

int main(int argc,char* argv[])
{
    //Arguments given
    const char* fname = argv[1];            //get the filename given
    int N = atoi(argv[2]);                  //atoi() converts a string to an integer,thus giving us the argument we need

    //File Opening-Creation (if non-existent) and cleared
    int fid = open(fname, O_CREAT|O_TRUNC|O_RDWR, S_IRWXU);
    if(fid == -1)
    {
    	printf("Failed to create the file!\n");
    	exit(0);
    }

    //Processes C1 creation
    pid_t fpid1 = fork();                    //Parent's pid1 is a positive int, while C1's is 0

    if(fpid1 == -1)                          //error case
    {
        perror("Child1 failed to be created!");
    }

    //Parent's case (not C1's)
    if(fpid1 != 0)
    {
    	//Process C2 creation
    	pid_t fpid2 = fork();                  //Parent's pid2 is a positive int, while C2's is 0

        //C2 error case
        if(fpid2 == -1)
        {
  	      	perror("Child2 failed to be created!");
        }

        //C2 case
        if(fpid2 == 0)
 		{
			Task("Child2", CYAN, N, 1, fid);
		}
	//Exclusively Parent's Case
	else
	{
		Task("Parent", RED, N/2, 0, fid);

		//Resetting offset to the beggining of the file
		int offset = lseek(fid, 0, SEEK_SET);

		//Error case of resetting offset
		if(offset == -1)
		{
			printf("Offset set failed");
		}

		file_manipulation(fid,buffer);
		close(fid);
		printf(WHITE "End of program!\n");
		return 0;
	}
     }
     else
     {
     	Task("Child1", GREEN, N, 0, fid);
     }
}
