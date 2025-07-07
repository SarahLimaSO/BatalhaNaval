
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include "../common/protocol.h"
#include <pthread.h>

#define PORT 8080
#define L 8 //Numero de Linhas
#define C 8 //Numero de Colunas

// Inicializa o tabuleiro
void inicializa_tabuleiro(char tab[L][C]){
   
    for (int i = 0; i < L; i++) {
        for (int j = 0; j < C; j++) {
            tab[i][j] = '~';
        }
    }
}

// Imprime tabuleiro
void print_tabuleiro(char tab[L][C]){

    //Identificando a numeracao das colunas
    
    printf("  ");
    for(int i = 0; i < C; i++){
        printf("  %d ", i+1);
    }
    putchar('\n');

    //Identificando as linhas do tabuleiro
    for(int i = 0; i < L; i++){
        
        printf("%c ", i+65); //Usando a tabela ascii para imprimir as letras
       
        // Imprimindo o tabuleiro
        for (int j = 0; j < C; j++) {
            putchar('|');
            printf(" %c ", tab[i][j]);
        }
        putchar('|');
        putchar('\n');
    }
}

void jogadas_cliente(char tab[L][C]){

}

int main() {

    char tabuleiro1[L][C];
    char tabuleiro2[L][C];

    inicializa_tabuleiro(tabuleiro1);
    inicializa_tabuleiro(tabuleiro2);

    printf("[TABULEIRO]\n");
    print_tabuleiro(tabuleiro1);
    printf("[TABULEIRO 2]\n");
    print_tabuleiro(tabuleiro2);

    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind\n");
        exit(1);
    }
    if (listen(server_fd, 2) < 0) {
        perror("listen\n");
        exit(1);
    } // até dois jogadores

    printf("Servidor aguardando jogadores na porta %d...\n", PORT);

    int player1 = accept(server_fd, (struct sockaddr*)&address, &addrlen);
    int player2 = accept(server_fd, (struct sockaddr*)&address, &addrlen);
    
    if (player1 < 0) perror("Accept player 1 failed");
    if (player2 < 0) perror("Accept player 2 failed");
    

    send(player1, "<<[Bem vind@ ao jogo Batalha Naval!]>>\n", 33, 0);
    send(player1, "Emparelhado! Você é o Jogador 1\n", 33, 0);

    send(player2, "<<[Bem vind@ ao jogo Batalha Naval!]>>\n", 33, 0);
    send(player2, "Emparelhado! Você é o Jogador 2\n", 33, 0);

    // Criando as threads para parelizacao
    // pthread_t thread[2];
    // pthread_create(&thread[0], NULL, jogadas_cliente, server_fd);
    // pthread_create(&thread[1], NULL, jogadas_cliente, server_fd);

    // Lendo os comandos do jogador
    // char buffer[1024];
    // recv(server_fd, buffer, sizeof(buffer), 0);

    // Mensagem msg = recebe_comando(buffer);
    // const char* resposta = gerar_resposta(msg.tipo);

    // send(socket_fd, resposta, strlen(resposta), 0);

    // Libera recursos da thread ao terminar
    // pthread_detach(thread[0]); 
    // pthread_detach(thread[1]); 

    close(server_fd);
    return 0;
}
