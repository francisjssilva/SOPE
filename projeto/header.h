typedef struct
{
	int nr;
	int time;

} ArgsToSend;

typedef struct 
{
	int p; // nr serie do pedido
	char g; // genero M ou F
	int t; // duracao da utilizacao
	int reject; //nr vezes rejeitado

} Spedido;

typedef struct 
{
	char genero; // genero de pessoas actualmente na sauna
	int maxPedidos; // max pessoas na sauna
	int nrPessoasSauna; // nr pessoas actualmente na sauna

} InfoSauna;