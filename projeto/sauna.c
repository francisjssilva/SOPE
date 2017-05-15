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
#include <semaphore.h>
#include <sys/time.h>

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0
#define MAX_SIZE 300


InfoSauna* infosauna;
int fdwr;
sem_t sem;
pthread_mutex_t mutex;
struct timespec time_init, time_end;
FILE* balfile;
char pidchar[MAX_SIZE];
char tidchar[MAX_SIZE];
int fd;

void *thrMLock(void* args)
{	
	sem_wait(&sem);
		infosauna->nrPessoasSauna++;
		while(((Spedido*)args)->t > 0){ //decrementa o tempo
			usleep(((Spedido*)args)->t*1000);
			((Spedido*)args)->t=0; 
			printf("Disfrutar da sauna :D\n");
		}
		clock_gettime(CLOCK_MONOTONIC, &time_end);
		float inst = (float)((float)time_end.tv_nsec-time_init.tv_nsec)/100000;
		sprintf(tidchar, "%ld", (long)pthread_self());


		fprintf(balfile, "%f - %s - %s - %d: %c - %d - SERVIDO\n", inst, pidchar, tidchar, ((Spedido*)args)->p, ((Spedido*)args)->g, ((Spedido*)args)->t);
		infosauna->analperson--;
		infosauna->nrPessoasSauna--;

		printf("analperson: %d\n", infosauna->analperson);	

		if(infosauna->analperson == 0){
			close(fd);
			close(fdwr);
		}

	sem_post(&sem);
pthread_exit(0);
	return NULL;
}

void *thrSauna(void* args)
{	
	clock_gettime(CLOCK_MONOTONIC, &time_end);
	float inst = (float)((float)time_end.tv_nsec-time_init.tv_nsec)/100000;
	sprintf(tidchar, "%ld", (long)pthread_self());


	fprintf(balfile, "%f - %s - %s - %d: %c - %d - RECEBIDO\n", inst, pidchar, tidchar, ((Spedido*)args)->p, ((Spedido*)args)->g, ((Spedido*)args)->t);
	printf("Ainda ha vaga para o cliente do genero %c com o id %d!\n",((Spedido*)args)->g, ((Spedido*)args)->p);
	pthread_t thread;
	pthread_create(&thread, NULL, thrMLock, (void*)((Spedido*)args)); 

pthread_exit(0);
	return NULL;
}

void *thrFunc(void* args)
{		
	pthread_mutex_lock(&mutex);
	if(infosauna->nrPessoasSauna == 0){ //primeiro pedido que limita o tipo de sauna.
		printf("Nr da Pessoa a limitar o genero da sauna: %d\n", ((Spedido*)args)->p);
		infosauna->genero = ((Spedido*)args)->g;
		pthread_t thread;
		pthread_create(&thread, NULL, thrMLock, (void*)((Spedido*)args)); 
	}
	else{
		if(((Spedido*)args)->g == infosauna->genero){ //pode entrar na sauna
			printf("O pedido e do mesmo genero\n");
			pthread_t thread;
			pthread_create(&thread, NULL, thrSauna, (void*)((Spedido*)args)); 
		}
		else{ //não é do mesmo genero, nao pode entrar, vai para uma fila de espera e o reject é decrementado
			printf("O pedido nao e do mesmo genero\n");

			clock_gettime(CLOCK_MONOTONIC, &time_end);
			float inst = (float)((float)time_end.tv_nsec-time_init.tv_nsec)/100000;
			sprintf(tidchar, "%ld", (long)pthread_self());


			fprintf(balfile, "%f - %s - %s - %d: %c - %d - REJEITADO\n", inst, pidchar, tidchar, ((Spedido*)args)->p, ((Spedido*)args)->g, ((Spedido*)args)->t);

			if(((Spedido*)args)->reject == 2){
				infosauna->analperson--;
			}

			if(((Spedido*)args)->reject < 3){	
				write(fdwr, ((Spedido*)args), sizeof(Spedido)); //devolve ao gerador
			}
		}
	}
	pthread_mutex_unlock(&mutex);

pthread_exit(0);
return NULL;
}

int main(int argc, char * argv[])
{
   pthread_mutexattr_t shared;
    pthread_mutexattr_init(&shared);
    pthread_mutexattr_setpshared(&shared, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&mutex, &shared);

    clock_gettime(CLOCK_MONOTONIC, &time_init);

    char balname[MAX_SIZE];


    strcpy(balname, "/tmp/bal.");
	sprintf(pidchar, "%ld", (long)getpid());
	strcat(balname, pidchar);

	balfile = fopen(balname, "w");

	infosauna = malloc(sizeof(InfoSauna*));
	if (argc != 2) 
	{
		printf("Wrong number of arguments!\nUse like: sauna < nr. max de pessoas na sauna>\n"); 
		exit(EXIT_FAILURE);
	}

	infosauna->maxPedidos = atoi(argv[1]);
	

	mkfifo("/tmp/rejeitados", 0660); 
	fd=open("/tmp/entrada", O_RDONLY);
	fdwr = open("/tmp/rejeitados", O_WRONLY);

	Spedido *pedido = malloc(sizeof(Spedido));
	pthread_t thread;


	sem_init(&sem, 0, infosauna->maxPedidos);

	while(read(fd, pedido, sizeof(Spedido)) > 0){
		if(pedido->p == 0){
			infosauna->analperson = pedido->nrpessoas;
		}
		printf("\n\nPedido nr %d esta a ser analisado com os dados:\n", pedido->p);
		printf(" - Genre: %c\n", pedido->g);
		printf(" - Time: %d\n", pedido->t);
		printf(" - Rej: %d\n", pedido->reject);
		pthread_create(&thread, NULL, thrFunc, (void*)pedido);
		pthread_join(thread, NULL);
	}

	sem_destroy(&sem);
	free(pedido);
	free(infosauna);
	fclose(balfile);
	return 0;
}
