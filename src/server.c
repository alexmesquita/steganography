#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>

#define PORT 3001
#define IP "192.168.133.26"
#define MAX 1024

void down_server();
int do_connect(int port);
void receive_img();
void save_file(char* img, char* file, int size);
void logger(char * message);

int descriptor_server = 0;
int descriptor_client = 0;

main(int argc, char *argv[])
{
	// Trata o sinal de interrupcao
	signal(SIGINT, down_server);

	// Faz a conexao na porta definida
	descriptor_server = do_connect(PORT);

	receive_img();

	// Desassocia o socket ao ip e fecha as conexoes
	unlink(IP);	
	close(descriptor_server);
}

/*
*	Ao receber o sinal de interrupcao todas as conexoes sao encerradas e fecha o programa
*/
void down_server()
{
	logger("Derrubando o servidor\n");

	if(descriptor_server)
	{
		close(descriptor_server);
	}
	if(descriptor_client)
	{
		close(descriptor_client);
	}
	unlink(IP);

	exit(0);
}

/*
*	Habilida o socket a aceitar uma nova comunicacao
*	Return: Descritor do socket
*/
int do_connect(int port)
{
	char up[30];

	sprintf(up, "Subindo o Servidor na porta: %d\n", port);
	logger(up);
	struct sockaddr_in addr_server;
	int descriptor;

	// Cria o socket
	if ((descriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
	{
		errx(1, "Erro ao criar o socket");
	}

	// Define as propriedades da conexao
	addr_server.sin_family = AF_INET;
	addr_server.sin_port = htons(PORT);
	addr_server.sin_addr.s_addr = inet_addr(IP);
	bzero(&(addr_server.sin_zero), 8);

	// Associa uma porta ao socket
	if (bind(descriptor, (struct sockaddr *)&addr_server, sizeof(struct sockaddr))== -1) 
	{
		close(descriptor);
		errx(1, "Erro ao executar o bind");
	}
	// Habilita a porta para escuta
	if (listen(descriptor, 20) < 0)
	{
		close(descriptor);
		errx(1, "Erro ao executar o linten");
	}

	return descriptor;
}

void save_file(char* img, char* file, int size)
{
    FILE *img_file;
    img_file = fopen(file, "a+");
    if (img_file != NULL)
    {
        fwrite(img, 1, size, img_file);
    }
    else
    {
        errx(1, "It was not possible to open the image file");
    }
    fclose(img_file);
}

/*
*	Recebe uma requisicao de temperatura, chama a uart para conseguir a temperatura
*	atual e responde esta temperatura
*/
void receive_img()
{
	struct sockaddr_in addr_client;

	while(1)
	{
		int length = sizeof(struct sockaddr_in);
		descriptor_client = accept(descriptor_server, (struct sockaddr *)&addr_client, &length);
		
		if (descriptor_client < 0)
		{
			perror("Erro ao executar o accept");
			continue;
		}

		if(recv(descriptor_client, &length, sizeof(length), 0) == 0)
		{
			perror("Erro na leitura do tamanho");
			continue;
		}

		char* text;
		text = (char*) malloc(length);
		char confirm = 0x00;
		if(recv(descriptor_client, text, length, 0) < length)
		{
			confirm = 0x01;
			perror("Erro na leitura da mensagem");
		}
		else
		{
			save_file(text, "res/send.y", length);
		}
		if (send(descriptor_client, &confirm, sizeof(char), 0) == -1)
		{
			perror("Erro ao enviar resposta");
		}

		free(text);
		close(descriptor_client);
	}
}

/*
* Funcao responsavel por salvar o log no arquivo "server.log"
*/
void logger(char * message)
{
	long hour;

	time(&hour);

	FILE* f_log = fopen("./server.log", "a+");

	fprintf(f_log, "%s", ctime(&hour));
	fprintf(f_log, "\t %s\n", message);

	fclose(f_log);
}
