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

        printf("**Posicione os navios**\n");
        printf("- Use o formato: POS <tipo> <x> <y> <H/V>\n\n");
      
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

        // Limpa o buffer e recebe resposta do servidor
        memset(buffer, 0, sizeof(buffer));
        int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            printf("Servidor desconectado.\n");
            break;
        }
        buffer[n] = '\0';

        printf("Servidor:\n%s\n", buffer);
        

        // Verifica se a resposta do servidor confirma o posicionamento
        if (strstr(buffer, "**Navio posicionado**") != NULL) {
            total_coord++; // só avança se o posicionamento for aceito
        } 
        else if (strstr(buffer, "!!Limite máximo") == NULL) {
            printf("Posicionamento inválido. Tente novamente.\n");
        }
    }
}


void prepara_inicio_jogo(int sock) {
    char buffer[1024];

    printf("\nDigite READY para começar o jogo:\n");
    while (1) {
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\r\n")] = 0;

        if (strcasecmp(buffer, "READY") == 0) {
            send(sock, buffer, strlen(buffer), 0);
            break;
        } else {
            printf("Digite READY para continuar.\n");
        }
    }

    // Aguarda mensagens do servidor após o READY
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) break;

        buffer[n] = '\0';
        printf("\nServidor: %s\n", buffer);

        if (strstr(buffer, "**INÍCIO DO JOGO!**") != NULL) {
            break;
        }
    }
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
    printf("\nServidor: %s\n", buffer);

    //Le o comando JOIN <nome>
    printf("Para começar a jogar digite JOIN <nome>:\n");
    fgets(nome, sizeof(nome), stdin);

    //Remove o \n do final 
    nome[strcspn(nome, "\n")] = '\0';

    // Envia mensagem JOIN nome
    snprintf(buffer, sizeof(buffer), "JOIN %.22s", nome);
    send(sock, buffer, strlen(buffer), 0);

    // Aguarda resposta do servidor
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            printf("Conexão fechada pelo servidor ou erro.\n");
            exit(1);
        }

        buffer[n] = '\0';
        printf("\nServidor: %s\n", buffer);

        // Só começa o posicionamento logo após o "JOGO INICIADO"
        if (strstr(buffer, "JOGO INICIADO") != NULL) {
            break;
        }
    }

    //Le a entrada(posicionamento) do cliente
    le_posicionamento_navios(sock);

    // Envia o ready e espera inicio do jogo
    prepara_inicio_jogo(sock);

    close(sock);
    return 0;
}


//PROBLEMAS A SEREM resolvidos:
// AJEITAR ESSA IMPRESSAO P FICAR MELHOR
// **Posicione os navios**
// - Use o formato: POS <tipo> <x> <y> <H/V>

// POS FRAGATA  2 2  
// Entrada inválida. Use o formato: POS <tipo> <x> <y> <H/V>
// **Posicione os navios**
// - Use o formato: POS <tipo> <x> <y> <H/V>