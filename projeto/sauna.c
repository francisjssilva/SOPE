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

InfoSauna* infosauna;
int fdwr;
pthread_mutex_t lock;

void *thrSauna(void* args)
{	

	if(((Spedido*)args)->p == 0){ //primeiro pedido que limita o tipo de sauna.
		infosauna->genero = 'F';
		infosauna->nrPessoasSauna = 1;
		printf("Primeira pessoa a delimitar a sauna: %d\n", infosauna->nrPessoasSauna);
	}
	else{
		if(infosauna->nrPessoasSauna < infosauna->maxPedidos){ //ha vagas na sauna?
			infosauna->nrPessoasSauna++;
			printf("Ainda ha vaga para o cliente do genero %c com o id %d!\n",((Spedido*)args)->g, ((Spedido*)args)->p);
		}
		else{ //sim, excedeu, nao pode entrar. vai para uma fila de espera.
			printf("Ainda nao ha vaga para o cliente do genero %c com o id %d!\n",((Spedido*)args)->g, ((Spedido*)args)->p);
			if(((Spedido*)args)->reject < 3){
				write(fdwr, ((Spedido*)args), sizeof(Spedido)); //devolve ao gerador
			}

		}
	}

//free(actPedido);
return NULL;
}

void *thrFunc(void* args)
{		
	if(((Spedido*)args)->p == 0){ //primeiro pedido que limita o tipo de sauna.
		infosauna->genero = ((Spedido*)args)->g;
		pthread_t thread;
		pthread_create(&thread, NULL, thrSauna, ((Spedido*)args)); 
		pthread_join(thread, NULL);
	}
	else{
		if(((Spedido*)args)->g == infosauna->genero){ //pode entrar na sauna
			printf("O pedido e do mesmo genero\n");
			pthread_t thread;
			pthread_create(&thread, NULL, thrSauna, ((Spedido*)args)); 
			pthread_join(thread, NULL);
		}
		else{ //não é do mesmo genero, nao pode entrar, vai para uma fila de espera
			printf("O pedido nao e do mesmo genero\n");
			printf("rejeitado=%d\n", ((Spedido*)args)->reject);
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

	while(read(fd, pedido, sizeof(Spedido)) > 0){
		printf("\n\nPedido nr %d esta a ser analisado com os dados:\n", pedido->p);
		printf(" - Genre: %c\n", pedido->g);
		printf(" - Time: %d\n", pedido->t);
		printf(" - Rej: %d\n", pedido->reject);
		pthread_create(&thread, NULL, thrFunc, pedido);
		pthread_join(thread, NULL);
	}

	free(pedido);
	free(infosauna);
	close(fdwr);
	close(fd);
	return 0;
}
