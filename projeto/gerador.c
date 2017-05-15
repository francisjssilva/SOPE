#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <unistd.h>
#include <sys/mman.h>
#include <time.h>
#include <errno.h> 
#include <string.h>
#include <signal.h> 
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "header.h"
#include <sys/time.h>

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0
#define MAX_SIZE 100

Spedido* pedido;
int fd1, fdrd;
Spedido thrd;
struct timespec time_init, time_end;
FILE* gerfile;
char pidchar[MAX_SIZE];

void *rejThr(void* args)
{
	while(read(fdrd, pedido, sizeof(Spedido)) > 0){



		pedido->reject++;

		thrd.p = pedido->p;
		thrd.g = pedido->g;
		thrd.t = pedido->t;
		thrd.reject = pedido->reject;

		float inst;

		if(pedido->reject < 3){
			printf("\nPedido nr %d rejeitado, vai voltar a ser analisado!\n", pedido->p);
			
			clock_gettime(CLOCK_MONOTONIC, &time_end);
			inst = (float)((float)time_end.tv_nsec-time_init.tv_nsec)/100000;
			fprintf(gerfile, "%f - %s - %d: %c - %d - REJEITADO\n", inst, pidchar, thrd.p, thrd.g, thrd.t);

			write(fd1, &thrd, sizeof(Spedido));
		}
		else{
			clock_gettime(CLOCK_MONOTONIC, &time_end);
			inst = (float)((float)time_end.tv_nsec-time_init.tv_nsec)/100000;
			fprintf(gerfile, "%f - %s - %d: %c - %d - DESCARTADO\n", inst, pidchar, thrd.p, thrd.g, thrd.t);

			printf("\nPedido nr %d excede tentativas, nao vai voltar a ser analisado!\n", pedido->p);
		}
	}

	return NULL;
}

void *thrRandom(void* args)
{
	mkfifo("/tmp/entrada", 0660); 

	fd1 = open("/tmp/entrada", O_WRONLY);
	fdrd = open("/tmp/rejeitados", O_RDONLY);

	char gername[MAX_SIZE];

	strcpy(gername, "/tmp/ger.");
	sprintf(pidchar, "%ld", (long)getpid());
	strcat(gername, pidchar);

	gerfile = fopen(gername, "w");

	if (fd1 < 0)
	{
		perror("FIFO open()");
		exit(EXIT_FAILURE);
	}

	float inst;


	for(int i = 0; i < ((ArgsToSend *)args)->nr; i++){
		thrd.p = i;
		thrd.g = (rand()%2)?'F':'M';
		thrd.t = rand() % ((ArgsToSend *)args)->time + 1;
		thrd.reject = 0;
		thrd.nrpessoas = ((ArgsToSend *)args)->nr;

		clock_gettime(CLOCK_MONOTONIC, &time_end);
		inst = (float)((float)time_end.tv_nsec-time_init.tv_nsec)/100000;

		fprintf(gerfile, "%f - %s - %d: %c - %d - PEDIDO\n", inst, pidchar, thrd.p, thrd.g, thrd.t);


		//write(gerfile, buffer, sizeof(buffer));

		//printf("sizeof(Spedido)%lu\n", sizeof(Spedido));
		write(fd1, &thrd, sizeof(Spedido));
	}

	//return NULL;

	pedido = malloc(sizeof(Spedido));

	pthread_t thread;
	pthread_create(&thread, NULL, rejThr, args); 
	pthread_join(thread, NULL);

	fclose(gerfile);
	close(fd1);
	close(fdrd);
	free(args);
	free(pedido);
	return NULL;
}


int main(int argc, char * argv[])
{
	srand(time(NULL)-getpid());
	clock_gettime(CLOCK_MONOTONIC, &time_init);

	if (argc != 3) 
	{
		printf("Wrong number of arguments\nUse like: gerador < nr. de pedidos > < Max time (milisegundos) >\n"); 
		exit(EXIT_FAILURE);
	}

	ArgsToSend * args =  malloc(sizeof(ArgsToSend));

	args->nr = atoi(argv[1]);
	args->time = atoi(argv[2]);

	pthread_t thread;
	pthread_create(&thread, NULL, thrRandom, (void *)args); 
	pthread_join(thread, NULL);

	exit(EXIT_SUCCESS);
}
