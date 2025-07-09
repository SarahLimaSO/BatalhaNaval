#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include "../common/protocol.h"

#define PORT 8080

//$$$ ## ** barcos

void le_posicionamento_navios(int sock){
    char buffer[1024];
    char tipo[20];
    int x, y;
    char orientacao;
    
    // Loop para enviar comandos de posicionamento
    for (int i = 0; i < MAX_NAVIOS; i++) {
        printf("Digite o navio e a posição desejada (tipo x y orientacao H/V): ");
        scanf("%s %d %d %c", tipo, &x, &y, &orientacao);

        // Monta a mensagem no formato esperado: "POS tipo x y Oriantacao"
        snprintf(buffer, sizeof(buffer), "POS %s %d %d %c", tipo, x, y, orientacao);

        // Envia para o servidor
        send(sock, buffer, strlen(buffer), 0);

        // Recebe resposta do servidor
        memset(buffer, 0, sizeof(buffer));
        int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            printf("Servidor desconectado.\n");
            break;
        }
        // buffer[n] = '\0';

        printf("Servidor: %s\n", buffer);

    }

    printf("Posicionamento finalizado. Aguarde o adversário...\n");
}

int main() {

    // inicializa_tabuleiro(tabuleiro1);
    // inicializa_tabuleiro(tabuleiro2);

    // printf("[TABULEIRO]\n");
    // print_tabuleiro(tabuleiro1);
    // printf("[TABULEIRO 2]\n");
    // print_tabuleiro(tabuleiro2);
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    char buffer[MAX_MSG] = {0};
    char nome[MAX_MSG];

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    read(sock, buffer, MAX_MSG);
    printf("Servidor: %s\n", buffer);

    //Le o comando JOIN <nome>
    printf("Digite um comando:\n");
    fgets(nome, sizeof(nome), stdin);

    // Envia mensagem JOIN nome
    snprintf(buffer, sizeof(buffer), "JOIN %.22s", nome);
    send(sock, buffer, strlen(buffer), 0);

    //Le a entrada(posicionamento) do cliente
    le_posicionamento_navios(sock);

    close(sock);
    return 0;
}
