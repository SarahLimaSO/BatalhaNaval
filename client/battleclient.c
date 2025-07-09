#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include "../common/protocol.h"
#include "../common/gameFeatures.h"

//$$$ ## ** barcos

#define PORT 8080

int main() {

    char tabuleiro1[L][C];
    char tabuleiro2[L][C];

    inicializa_tabuleiro(tabuleiro1);
    inicializa_tabuleiro(tabuleiro2);

    printf("[TABULEIRO]\n");
    print_tabuleiro(tabuleiro1);
    printf("[TABULEIRO 2]\n");
    print_tabuleiro(tabuleiro2);
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    char buffer[MAX_MSG] = {0};

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    read(sock, buffer, MAX_MSG);
    printf("Servidor: %s\n", buffer);

    // int total_barcos = MAX_DEST + MAX_FRAG + MAX_SUB;
    // int i = 0;

    //Deve ler as entradas do jogador e mandar ao servidor(onde processa essa entrada) e devolve resposta
    


    close(sock);
    return 0;
}
