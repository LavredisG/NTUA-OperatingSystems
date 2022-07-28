#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

void check(int input, const char* msg)
{
	if(input < 0)
	{
	//	fprintf(stderr, msg);
        perror(msg);
        exit(EXIT_FAILURE);
	}
}		

int main(int argc, char* argv[])
{
    if(argc != 3)
    {
        fprintf(stderr, "Invalid number of arguments! (3 required) \n");
        exit(EXIT_FAILURE);
    }
    
    int children = atoi(argv[1]),  num = atoi(argv[2]);       //arguments 
    pid_t pid[children];
    int pd[children][2];             						  //Create a (children)x(2) pipe descriptor array
   
    if((children <= 1) || (num < 0))
    {
        fprintf(stderr, "Invalid arguments!\n");
        exit(EXIT_FAILURE);
    }

	//Pipes creation
    for(int i = 0; i < children; i++)
    {
        check(pipe(pd[i]), "Pipe creation failed! Exiting...\n");
    }


    //Children creation 
    for(int i = 0; i < children; i++)
    {
        check(pid[i] = fork(), "Failed to fork!\n");

        //Child case
        if(pid[i] == 0)
        {   
            if(i == 0)
            {   
                //Close unused ends
                for(int j = 1; j < (children - 1); j++)
                {    
                    check(close(pd[j][0]), "Failed to close a read pipe gate!\n");
                    check(close(pd[j][1]), "Failed to close a write pipe gate!\n");
                }
                check(close(pd[children - 1][1]), "Failed to close a write pipe gate\n");
                check(close(pd[i][0]), "Failed to close a read pipe gate!\n");
            
            	unsigned long long int counter = i + 1, receive, send;
                if(num != 0)
                while(1)
                {
                    check(read(pd[children - 1][0], &receive, sizeof(receive)), "Failed to read from a pipe!\n");
                    if(receive == -1)                       //result has been already found
                    {
                        send = -1;
                        check(write(pd[i][1], &send, sizeof(send)), "Failed to write to a pipe!\n");
                        check(close(pd[i][1]), "Failed to close a pipe gate!\n");
                        check(close(pd[children - 1][0]), "Failed to close a pipe gate!\n");
                        exit(0);
                    }
                    if(counter == num)                      //last child to compute
                    {
                        printf("%d! = %lld\n", num, counter * receive);
                        send = -1;
                        check(write(pd[i][1], &send, sizeof(send)), "Failed to write to a pipe!\n");
                        check(close(pd[i][1]), "Failed to close a pipe gate!\n");
                        check(close(pd[children - 1][0]), "Failed to close a pipe gate!n");
                        exit(0);
                    }
                    send = receive * counter;
                    check(write(pd[i][1], &send, sizeof(send)), "Failed to write to a pipe!\n");
                   	counter = counter + children;
                }
                //Case of 0!
            	check(read(pd[children - 1][0], &receive, sizeof(receive)), "Failed to read from a pipe!\n");
            	send = -1;
                check(write(pd[i][1], &send, sizeof(send)), "Failed to write to a pipe!\n");
                printf("%d! = %lld\n", num, receive);
                check(close(pd[i][1]), "Failed to close a pipe gate!\n");
                check(close(pd[children - 1][0]), "Failed to close a pipe gate!\n");
                exit(0);
            }
            else 
            { 
                if(i == (children - 1))
                {
                for(int j = 0; j < (i -1); j++)
                    {
                    check(close(pd[j][0]), "Failed to close a pipe gate!\n");
                    check(close(pd[j][1]), "Failed to close a pipe gate!\n");
                    }
                    check(close(pd[i][0]), "Failed to close a pipe gate!\n");
                    check(close(pd[i-1][1]), "Failed to close a pipe gate!\n");
                }
                else
                {   
                    for(int j = i; j < (children - 1); j++)          //close right pipes
                    {    
                        check(close(pd[j + 1][0]), "Failed to close a pipe gate!\n");
                        check(close(pd[j + 1][1]), "Failed to close a pipe gate!\n");
                    }
                    for(int j = 0; j < (i - 1); j++)                //close left pipes
                    {
                        check(close(pd[j][0]), "Failed to close a pipe gate!\n");
                        check(close(pd[j][1]), "Failed to close a pipe gate!\n");
                    }
                    check(close(pd[i][0]), "Failed to close a pipe gate!\n");
                    check(close(pd[i - 1][1]), "Failed to close a pipe gate!\n");
                }
                unsigned long long int counter = i + 1, receive, send;
                while(1)			//counter <= num
                {   
                    check(read(pd[i - 1][0], &receive, sizeof(receive)), "Failed to read from a pipe!\n");
                    if(receive == -1)
                    {
                        send = -1;
                        check(write(pd[i][1], &send, sizeof(send)), "Failed to write to a pipe!\n");
                        check(close(pd[i - 1][0]), "Failed to close a pipe gate!\n");
                        check(close(pd[i][1]), "Failed to close a pipe gate!\n");
                        exit(0);
                    }
                    if(counter == num)
                    {
                        printf("%d! = %lld\n", num, counter * receive);
                        send = -1;
                        check(write(pd[i][1], &send, sizeof(send)), "Failed to write to a gate\n");
                        check(close(pd[i - 1][0]), "Failed to close a pipe gate!\n");
                        check(close(pd[i][1]), "Failed to close a pipe gate!\n");
                        exit(0);
                    }
                    send = receive * counter;
                    check(write(pd[i][1], &send, sizeof(send)), "Failed to write to a pipe gate!\n");
                    counter = counter + children;
                }
                exit(0);
            }
            
        }
    } 

    for(int j = 0; j < (children - 1); j++)
    {  
        check(close(pd[j][0]), "Failed to close a pipe gate!\n");
        check(close(pd[j][1]), "Failed to close a pipe gate!\n");
    }
    check(close(pd[children - 1][0]), "Failed to close a pipe gate!\n");
    int start = 1;
    check(write(pd[children - 1][1], &start, sizeof(int)), "Failed to write to a pipe!\n");
    check(close(pd[children - 1][1]), "Failed to close a pipe gate!\n");
    for(int i = 0; i < children; i++)
        wait(NULL);

    return 0; 
}

