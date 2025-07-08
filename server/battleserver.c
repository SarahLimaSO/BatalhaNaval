
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include "../common/protocol.h"
#include "../common/gameFeatures.h"

#define PORT 8080

void atribui_jogadores(int player1, int player2){
    srand((unsigned)time(NULL));

    int sorteio = rand() % 2;

    if (sorteio == 0) {
        send(player1, "Você é o Jogador 1\n", 21, 0);
        send(player2, "Você é o Jogador 2\n", 21, 0);
    } else {
        send(player2, "Você é o Jogador 1\n", 21, 0);
        send(player1, "Você é o Jogador 2\n", 21, 0);
    }
}
void processa_tipoCmd(int tipoCmd, int player1, int player2){
    switch(tipoCmd){
        case 1:
            send(player1, "JOGO INICIADO!\n", strlen("JOGO INICIADO!\n"), 0);
            send(player2, "JOGO INICIADO!\n", strlen("JOGO INICIADO!\n"), 0);

            atribui_jogadores(player1, player2); //Sorteia qual dos jogadores eh o jogador 1 e 2  
            // send(player1, "-Posicione os seu barcos\n", strlen("-Posicione os seu barcos\n"), 0);
            // send(player2, "JOGO INICIADO!\n", strlen("JOGO INICIADO!\n"), 0);
            break;
    }
    return;
}

int main() {

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

    send(player1, "<<[Bem vind@ ao jogo Batalha Naval!]>>\n", strlen("<<[Bem vind@ ao jogo Batalha Naval!]>>\n"), 0);
    send(player2, "<<[Bem vind@ ao jogo Batalha Naval!]>>\n", strlen("<<[Bem vind@ ao jogo Batalha Naval!]>>\n"), 0);


    // Lendo os comandos do jogador
    char buffer[1024];
    recv(server_fd, buffer, sizeof(buffer), 0);

    int tipoCmd = recebe_comando(buffer);

    processa_tipoCmd(tipoCmd, player1, player2); //Processa e reaje conforme o cmd recebido

    // Fecha os sockets dos jogadores
    close(player1);
    close(player2);

    // Fecha o socket do servidor
    close(server_fd);
    return 0;
}
