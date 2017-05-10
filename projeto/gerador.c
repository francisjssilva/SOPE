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

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0
#define MAX_SIZE 300

void *thrRandom(void* args)
{

	mkfifo("/tmp/entrada", 0660); 

	Spedido thrd;

	int fd1 = open("/tmp/entrada", O_WRONLY);
	int fdrd = open("/tmp/rejeitados", O_RDONLY);

	if (fd1 < 0)
	{
		perror("FIFO open()");
		exit(EXIT_FAILURE);
	}

	for(int i = 0; i < ((ArgsToSend *)args)->nr; i++){
		printf("entra\n");
		thrd.p = i;
		thrd.g = 'M';
		thrd.t = 1000;
		thrd.reject = 0;
		printf("sizeof(Spedido)%lu\n", sizeof(Spedido));
		printf("write:%zd\n", write(fd1, &thrd, sizeof(Spedido)));
	}

	Spedido* pedido = malloc(sizeof(Spedido));

	while(read(fdrd, pedido, sizeof(Spedido)) > 0){

		pedido->reject++;

		thrd.p = pedido->p;
		thrd.g = pedido->g;
		thrd.t = pedido->t;
		thrd.reject = pedido->reject;

		if(pedido->reject < 3){
			printf("\nPedido nr %d rejeitado, vai voltar a ser analisado!\n", pedido->p);
			write(fd1, &thrd, sizeof(Spedido));
		}
		else{
			printf("\nPedido nr %d excede tentativas, nao vai voltar a ser analisado!\n", pedido->p);
		}
	}

	close(fd1);
	close(fdrd);
	free(args);
	free(pedido);
	pthread_exit(NULL);
}


int main(int argc, char * argv[])
{
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
	//pthread_exit(0);

	exit(EXIT_SUCCESS);
}
