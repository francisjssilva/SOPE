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

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

InfoSauna* infosauna;
int fdwr;
sem_t sem;

void *thrMLock(void* args)
{	
	sem_wait(&sem);
		infosauna->nrPessoasSauna++;
		while(((Spedido*)args)->t > 0){ //decrementa o tempo
			usleep(((Spedido*)args)->t*1000);
			((Spedido*)args)->t=0; 
			printf("Disfrutar da sauna :D\n");
		}
		infosauna->nrPessoasSauna--;
	sem_post(&sem);

	return NULL;
}

void *thrSauna(void* args)
{	
		//if(infosauna->nrPessoasSauna < infosauna->maxPedidos){ //ha vagas na sauna?
			printf("Ainda ha vaga para o cliente do genero %c com o id %d!\n",((Spedido*)args)->g, ((Spedido*)args)->p);
			pthread_t thread;
			pthread_create(&thread, NULL, thrMLock, ((Spedido*)args)); 
			//pthread_join(thread, NULL);
		/*}
		else{ //sim, excedeu, nao pode entrar. vai para uma fila de espera.
			printf("Ainda nao ha vaga para o cliente do genero %c com o id %d!\n",((Spedido*)args)->g, ((Spedido*)args)->p);
			pthread_t thread;
			pthread_create(&thread, NULL, thrMLock, ((Spedido*)args)); 
			//pthread_join(thread, NULL);
		}*/

	return NULL;
}

void *thrFunc(void* args)
{		
	if(infosauna->nrPessoasSauna == 0){ //primeiro pedido que limita o tipo de sauna.
		printf("Nr da Pessoa a limitar o genero da sauna: %d\n", ((Spedido*)args)->p);
		infosauna->genero = ((Spedido*)args)->g;
		pthread_t thread;
		pthread_create(&thread, NULL, thrMLock, ((Spedido*)args)); 
		//pthread_join(thread, NULL);
	}
	else{
		if(((Spedido*)args)->g == infosauna->genero){ //pode entrar na sauna
			printf("O pedido e do mesmo genero\n");
			pthread_t thread;
			pthread_create(&thread, NULL, thrSauna, ((Spedido*)args)); 
			//pthread_join(thread, NULL);
		}
		else{ //não é do mesmo genero, nao pode entrar, vai para uma fila de espera e o reject é decrementado
			printf("O pedido nao e do mesmo genero\n");
			//printf("rejeitado=%d\n", ((Spedido*)args)->reject);
			if(((Spedido*)args)->reject < 3){	
				write(fdwr, ((Spedido*)args), sizeof(Spedido)); //devolve ao gerador
			}
		}
	}

return NULL;
}

int main(int argc, char * argv[])
{
	infosauna = malloc(sizeof(InfoSauna*));
	if (argc != 2) 
	{
		printf("Wrong number of arguments!\nUse like: sauna < nr. max de pessoas na sauna>\n"); 
		exit(EXIT_FAILURE);
	}

	infosauna->maxPedidos = atoi(argv[1]);

	mkfifo("/tmp/rejeitados", 0660); 
	int fd=open("/tmp/entrada", O_RDONLY);
	fdwr = open("/tmp/rejeitados", O_WRONLY);

	Spedido *pedido = malloc(sizeof(Spedido));
	pthread_t thread;

	sem_init(&sem, 0, infosauna->maxPedidos);

	while(read(fd, pedido, sizeof(Spedido)) > 0){
		printf("\n\nPedido nr %d esta a ser analisado com os dados:\n", pedido->p);
		printf(" - Genre: %c\n", pedido->g);
		printf(" - Time: %d\n", pedido->t);
		printf(" - Rej: %d\n", pedido->reject);
		pthread_create(&thread, NULL, thrFunc, pedido);
		pthread_join(thread, NULL);
	}


	sem_destroy(&sem);
	free(pedido);
	free(infosauna);
	close(fdwr);
	close(fd);
	return 0;
}
