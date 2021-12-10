#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

int out_char = 0, counter = 128;
int key = 1;
pid_t pid;

// SIGCHLD
void childexit(int signo) 
{
	key = 0;
}

// SIGALRM
void parentexit(int signo) 
{ 
	exit(EXIT_SUCCESS);
}

// Nothing
void empty(int signo) 
{
}

// SIGUSR1
void one(int signo) 
{
	out_char += counter;
	counter /= 2;	
	kill(pid, SIGUSR1);
}

// SIGUSR2
void zero(int signo) 
{ 
	counter/=2;	
	kill(pid, SIGUSR1);
}


int main(int argc, char ** argv)
{
  	if (argc != 2) 
  	{
    		printf("Use: %s [source]\n", argv[0]);
    		exit(EXIT_FAILURE);
  	}
  	pid_t ppid = getpid();

 	sigset_t set;

  	// SIGCHLD - exit
  	struct sigaction act_exit;
  	memset(&act_exit, 0, sizeof(act_exit));
  	act_exit.sa_handler = childexit; 
  	if(sigfillset(&act_exit.sa_mask) == -1)
    		exit(EXIT_FAILURE); 
  	if(sigaction(SIGCHLD, &act_exit, NULL)== -1)
    		exit(EXIT_FAILURE); 

	// SIGUSR1 - one()
 	struct sigaction act_one;
 	memset(&act_one, 0, sizeof(act_one));
  	act_one.sa_handler = one;
  	if(sigfillset(&act_one.sa_mask) == -1)
    		exit(EXIT_FAILURE);
  	if (sigaction(SIGUSR1, &act_one, NULL)== -1)
    		exit(EXIT_FAILURE);

  	// SIGUSR2 - zero()
  	struct sigaction act_zero;
  	memset(&act_zero, 0, sizeof(act_zero));
  	act_zero.sa_handler = zero;
  	if(sigfillset(&act_zero.sa_mask) == -1)
    		exit(EXIT_FAILURE);    
  	if(sigaction(SIGUSR2, &act_zero, NULL) == -1)
    		exit(EXIT_FAILURE);
    	//1) также идет race condition между обработчиком и родителем за counter

  	sigaddset(&set, SIGUSR1);
  	sigaddset(&set, SIGUSR2);
  	sigaddset(&set, SIGCHLD);
  	if(sigprocmask(SIG_BLOCK, &set, NULL ) == -1)
    		exit(EXIT_FAILURE);
  	sigemptyset(&set);

  	pid = fork();

  	if (pid == -1)
  	{
  		printf("Error of fork()\n");
  		exit(EXIT_SUCCESS); 
  	}
  	if (pid == 0) 
  	{
    		unsigned int fd = 0;
    		char c = 0;
    		sigemptyset(&set);

    		// SIGUSR1 - empty()
    		struct sigaction act_empty;                    
    		memset(&act_empty, 0, sizeof(act_empty));
    		act_empty.sa_handler = empty;
    		if(sigfillset(&act_empty.sa_mask) == -1)
    			exit(EXIT_FAILURE);    
    		if(sigaction(SIGUSR1, &act_empty, NULL) == -1)
    			exit(EXIT_FAILURE);
    		// SIGALRM - parentexit()
    		struct sigaction act_alarm;
    		memset(&act_alarm, 0, sizeof(act_alarm));
    		act_alarm.sa_handler = parentexit;
    		if(sigfillset(&act_alarm.sa_mask) == -1)
    			exit(EXIT_FAILURE);  
    		if (sigaction(SIGALRM, &act_alarm, NULL) == -1)
    			exit(EXIT_FAILURE);

    		if ((fd = open(argv[1], O_RDONLY)) < 0 )
    		{
      			perror("Can't open file");
      			exit(EXIT_FAILURE);
    		}

    		int i;
    		while (read(fd, &c, 1) > 0)
    		{	
      			alarm(3);
      			for ( i = 128; i >= 1; i /= 2)
      			{
        			if ( i & c )            
          				kill(ppid, SIGUSR1);
        			else                  
          				kill(ppid, SIGUSR2);
        			sigsuspend(&set); 
      			}
    		}
    		exit(EXIT_SUCCESS);
  	}

  	errno = 0;
 	do {	
    		if(counter == 0)
    		{     
			write(STDOUT_FILENO, &out_char, 1);       
      			fflush(stdout);
      			counter=128;
      			out_char = 0;
    		}
    		sigsuspend(&set);
  	} while (key);

  	return (0);
}

  	errno = 0;
 	do {	
    		if(counter == 0)
    		{     
			write(STDOUT_FILENO, &out_char, 1);       
      			fflush(stdout);
      			counter=128;
      			out_char = 0;
    		}
    		sigsuspend(&set);
  	} while (1);

  	return (0);
}
