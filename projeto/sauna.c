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
#define SNAME "/mysem"


InfoSauna* infosauna;
int fdwr;
sem_t *sem;
pthread_mutex_t mutex;
struct timespec time_init, time_end;
FILE* balfile;
char pidchar[MAX_SIZE];
char tidchar[MAX_SIZE];
int fd;

void *thrMLock(void* args)
{
	char g = ((Spedido*)args)->g;
	int p = ((Spedido*)args)->p;
	int t = ((Spedido*)args)->t;

	clock_gettime(CLOCK_MONOTONIC, &time_end);
	float inst = (float)((float)time_end.tv_nsec-time_init.tv_nsec)/100000;
	sprintf(tidchar, "%ld", (long)pthread_self());

	printf("\n\nPedido nr %d esta a ser analisado com os dados:\n", ((Spedido*)args)->p);
	printf(" - Genre: %c\n", ((Spedido*)args)->g);
	printf(" - Time: %d\n", ((Spedido*)args)->t);
	printf(" - Rej: %d\n", ((Spedido*)args)->reject);

pthread_mutex_lock(&mutex);
	fprintf(balfile, "%f - %s - %s - %d: %c - %d - RECEBIDO\n", inst, pidchar, tidchar, p, g, t);
	printf("Ainda ha vaga para o cliente do genero %c com o id %d!\n",g, p);
pthread_mutex_unlock(&mutex);

	sem_wait(sem);
		infosauna->nrPessoasSauna++;
		//while(((Spedido*)args)->t > 0){ //decrementa o tempo
			usleep(((Spedido*)args)->t*1000);
			//((Spedido*)args)->t=0;
			printf("Disfrutar da sauna :D\n");
	//	}
	sem_post(sem);

		clock_gettime(CLOCK_MONOTONIC, &time_end);
		inst = (float)((float)time_end.tv_nsec-time_init.tv_nsec)/100000;
		sprintf(tidchar, "%ld", (long)pthread_self());

pthread_mutex_lock(&mutex);
		fprintf(balfile, "%f - %s - %s - %d: %c - %d - SERVIDO\n", inst, pidchar, tidchar, p, g, t);
		infosauna->analperson--;
		infosauna->nrPessoasSauna--;
pthread_mutex_unlock(&mutex);

		printf("analperson: %d\n", infosauna->analperson);

		if(infosauna->analperson == 0){
			close(fd);
			close(fdwr);
		}

pthread_exit(0);
	return NULL;
}

/*void *thrSauna(void* args)
{
	clock_gettime(CLOCK_MONOTONIC, &time_end);
	float inst = (float)((float)time_end.tv_nsec-time_init.tv_nsec)/100000;
	sprintf(tidchar, "%ld", (long)pthread_self());

	printf("\n\nPedido nr %d esta a ser analisado com os dados:\n", ((Spedido*)args)->p);
	printf(" - Genre: %c\n", ((Spedido*)args)->g);
	printf(" - Time: %d\n", ((Spedido*)args)->t);
	printf(" - Rej: %d\n", ((Spedido*)args)->reject);


	fprintf(balfile, "%f - %s - %s - %d: %c - %d - RECEBIDO\n", inst, pidchar, tidchar, ((Spedido*)args)->p, ((Spedido*)args)->g, ((Spedido*)args)->t);
	printf("Ainda ha vaga para o cliente do genero %c com o id %d!\n",((Spedido*)args)->g, ((Spedido*)args)->p);
	pthread_t thread;
	pthread_create(&thread, NULL, thrMLock, (void*)((Spedido*)args));
	pthread_join(thread, NULL);

pthread_exit(0);
	return NULL;
}*/

void *thrFunc(void* args)
{
//	pthread_mutex_lock(&mutex);
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
			pthread_create(&thread, NULL, thrMLock, (void*)((Spedido*)args));
		}
		else{ //não é do mesmo genero, nao pode entrar, vai para uma fila de espera e o reject é decrementado
			printf("O pedido nao e do mesmo genero\n");

			clock_gettime(CLOCK_MONOTONIC, &time_end);
			float inst = (float)((float)time_end.tv_nsec-time_init.tv_nsec)/100000;
			sprintf(tidchar, "%ld", (long)pthread_self());

			pthread_mutex_lock(&mutex);
				fprintf(balfile, "%f - %s - %s - %d: %c - %d - REJEITADO\n", inst, pidchar, tidchar, ((Spedido*)args)->p, ((Spedido*)args)->g, ((Spedido*)args)->t);
			pthread_mutex_unlock(&mutex);

			if(((Spedido*)args)->reject == 2){
				infosauna->analperson--;
			}

			if(((Spedido*)args)->reject < 3){
				write(fdwr, ((Spedido*)args), sizeof(Spedido)); //devolve ao gerador
			}
		}
	}
//	pthread_mutex_unlock(&mutex);

pthread_exit(0);
return NULL;
}

int main(int argc, char * argv[])
{
   /*pthread_mutexattr_t shared;
    pthread_mutexattr_init(&shared);
    pthread_mutexattr_setpshared(&shared, PTHREAD_PROCESS_SHARED);*/

    pthread_mutex_init(&mutex, NULL);

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

	sem = sem_open(SNAME, O_CREAT, 0644, infosauna->maxPedidos);
	//sem_init(&sem, 0, infosauna->maxPedidos);
	//int index=0;

	while(read(fd, pedido, sizeof(Spedido)) > 0){
		if(pedido->p == 0){
			infosauna->analperson = pedido->nrpessoas;
		}

		pthread_create(&thread, NULL, thrFunc, (void*)pedido);
		pthread_join(thread, NULL);
		//index++;
	}

	/*int i;
	for(i = 0; i < index; index++){
		pthread_join(thread[i], NULL);
	}*/

	sem_destroy(sem);
	free(pedido);
	free(infosauna);
	fclose(balfile);
	return 0;
}
