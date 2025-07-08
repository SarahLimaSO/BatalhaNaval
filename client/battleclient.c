#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
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

    // Tomando precaucoes para que o sockets nao sejam sobrescritos nas threads
    int* socket_p1 = malloc(sizeof(int));
    int* socket_p2 = malloc(sizeof(int));

    socket_p1 = player1;
    socket_p2 = player2;
 
    // Criando as threads para parelizacao
    pthread_t thread[2];
    pthread_create(&thread[0], NULL, posiciona_barcos, socket_p1);
    pthread_create(&thread[1], NULL, posiciona_barcos, socket_p2);

    close(sock);
    return 0;
}
