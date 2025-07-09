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
    // while (i <= total_barcos) {
    //     char tipo[20];
    //     int x, y;
    //     char orientacao;

    //     printf("\nDigite tipo, x, y e orientação (ex: DESTROYER 2 3 H): ");
    //     scanf("%s %d %d %c", tipo, &x, &y, &orientacao);

    //     sprintf(buffer, "POS %s %d %d %c", tipo, x, y, orientacao);
    //     send(sock, buffer, strlen(buffer), 0);

    //     memset(buffer, 0, sizeof(buffer));
    //     recv(sock, buffer, sizeof(buffer), 0);
    //     printf("Servidor respondeu: %s\n", buffer);

    //     if (strncmp(buffer, "**Navio posicionado**", 22) == 0) {
    //         i++;
    //     }
    // }


    close(sock);
    return 0;
}
