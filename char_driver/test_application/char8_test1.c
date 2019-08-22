#include <stdio.h>
#include <signal.h>

//typedef void (*sighandler_t)(int);
//sighandler_t signal(int signum, sighandler_t handler);

void signal_handler(int signum)
{
	printf("catch %d signal\n", signum);
}

int main()
{
	signal(SIGINT, signal_handler);
	while (1);
	printf("end\n");
	return 0;
}
