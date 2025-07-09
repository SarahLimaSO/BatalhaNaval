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

    int x, y, total_coord;
    char orientacao;

    total_coord = 0;
    
    // Loop para enviar comandos de posicionamento
    while(total_coord < MAX_NAVIOS) {
        printf("**Comece a posicionar os navios**\n");
        printf("- Use o formato: POS <tipo> <x> <y> <H/V>\n");

        fgets(buffer, sizeof(buffer), stdin);
            
        //Remove o \n do final do buffer
        buffer[strcspn(buffer, "\r\n")] = '\0';
        
        //Verifica de a entrada esta no formato esperado
        if (sscanf(buffer, "POS %s %d %d %c", tipo, &x, &y, &orientacao) != 4) {
            printf("Entrada inválida. Use o formato: POS <tipo> <x> <y> <H/V>\n");
            continue;
        }

        // Monta a mensagem no formato esperado: "POS tipo x y Oriantacao"
        snprintf(buffer, sizeof(buffer), "POS %s %d %d %c", tipo, x, y, orientacao);

        // Envia para o servidor
        send(sock, buffer, strlen(buffer), 0);

        // RLimpa o buffer e recebe resposta do servidor
        memset(buffer, 0, sizeof(buffer));
        int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            printf("Servidor desconectado.\n");
            break;
        }

        buffer[n] = '\0';
        printf("Servidor: %s\n", buffer);
        total_coord++;

    }

    printf("Posicionamento finalizado. Aguarde o adversário...\n");
}

int main() {

    
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

    //Remove o \n do final 
    nome[strcspn(nome, "\n")] = '\0';

    // Envia mensagem JOIN nome
    snprintf(buffer, sizeof(buffer), "JOIN %.22s", nome);
    send(sock, buffer, strlen(buffer), 0);

    // Aguarda resposta do servidor
    while(1){
        memset(buffer, 0, sizeof(buffer));
        recv(sock, buffer, sizeof(buffer) - 1, 0);
        printf("Servidor: %s\n", buffer);

        // Sai do loop somente se os dois jogadores deram JOIN
        if (strstr(buffer, "JOGO INICIADO") != NULL) {
            break;
        }
    }

    //Le a entrada(posicionamento) do cliente
    le_posicionamento_navios(sock);

    close(sock);
    return 0;
}
