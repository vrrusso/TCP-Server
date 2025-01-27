#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include "../config.h"

typedef struct sockaddr_in socket_address;

void send_file(FILE* fp, int client_socket, int last_packet_size, int n_packets){
	char buffer[PACKET_SIZE];
	memset(buffer, 0, PACKET_SIZE);

	if (last_packet_size != 0){
		n_packets--;
	}

	for(int i = 0 ; i < n_packets; i++){
    	fread(buffer, 1, PACKET_SIZE, fp);
    	write(client_socket , buffer , PACKET_SIZE);
  	}

  	fread(buffer, 1, last_packet_size, fp);
  	write(client_socket , buffer , last_packet_size);

  	fclose(fp);
}

void* communation_thread(void *client_sock){

	// Voltando o socker descriptor do cliente para inteiro
    int client_socket = *(int*)client_sock, file_size, n_packets, last_packet_size;
	char filename[FILENAME_SIZE];
	memset(filename, 0, FILENAME_SIZE);

	FILE* fp;

	// Le o input do cliente até serem enviados 0 bytes
	while(read(client_socket , filename, FILENAME_SIZE)){

		if(!strcmp(filename, "sair")){
			break;
		}

		fp = fopen(filename, "rb");
		if (fp == NULL){
			perror("Erro ao ler o arquivo");
			exit(1);
		}

		fseek(fp, 0, SEEK_END);
		file_size = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		last_packet_size = file_size%PACKET_SIZE;
		printf("Tamanho do Arquivo: %d\n", file_size);
		n_packets = last_packet_size ? file_size/PACKET_SIZE+1 : file_size/PACKET_SIZE;
		printf("Numero de pacotes a ser enviado: %d\n", n_packets);

		write(client_socket, &n_packets, sizeof(int));
		send_file(fp, client_socket, last_packet_size, n_packets);

		memset(filename, 0, FILENAME_SIZE);
	}
	printf("\n----- Conexão encerrada -----\n");

	close(client_socket);
	return NULL;
}

int main(){
	socket_address address;
	int server_fd, client_socket, addrlen = sizeof(address);

	// Socket descriptor, inteiro que a aplicação usa sempre que quer se referir a este socket
	server_fd = socket(AF_INET, SOCK_STREAM, 0);

	// AF_INET: IPv4
	// INADDR_ANY: Para todas as interfaces de rede disponíveis
	// PORT: 1337
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	address.sin_port = htons(PORT);

	// Vincula o socket ao localhost e a porta 1337
	bind(server_fd,(const struct sockaddr *)&address,sizeof(address));

	// Coloca o socket do servidor em modo passivo, ou seja, espera pelo cliente estabelecer uma conexão
	// Segundo parametro (backlog): Números máximo de conexões pendentes na fila
	listen(server_fd, 10);
	pthread_t thread_id;
	printf("\n----- Esperando Conexão -----\n");

	while(1){

		// Retira a primeira requisição da fila, cria um novo socket e retorna um novo socket descriptor (new_socket)
		client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
		printf("\n----- Conexão Estabelecida -----\n");

		pthread_create( &thread_id , NULL , communation_thread , (void*) &client_socket);
		//pthread_join(thread_id , NULL);
	}

	return 0;
}
