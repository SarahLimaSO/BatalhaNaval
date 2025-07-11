#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include "../common/protocol.h"

#define PORT 8080

// Variáveis globais para o tabuleiro de ataque
char tabuleiro_ataque[L][C];
int a_sock;

void* recebe_mensagens_auto(void* arg);

void posiciona_navios_auto(int sock) {
    char buffer[MAX_MSG];
    char* tipos[] = {"SUBMARINO", "FRAGATA", "FRAGATA", "DESTROYER"};
    int navios_posicionados = 0;

    while (navios_posicionados < MAX_NAVIOS) {
        int x = rand() % L;
        int y = rand() % C;
        char orientacao = (rand() % 2 == 0) ? 'H' : 'V';
        
        snprintf(buffer, sizeof(buffer), "POS %s %d %d %c", tipos[navios_posicionados], x + 1, y + 1, orientacao);
        send(sock, buffer, strlen(buffer), 0);
        
        memset(buffer, 0, sizeof(buffer));
        int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (n > 0) {
            buffer[n] = '\0';
            if (strstr(buffer, "**Navio posicionado**")) {
                navios_posicionados++;
            }
        }
    }
    printf("Auto-Client: Navios posicionados.\n");
    send(sock, CMD_READY, strlen(CMD_READY), 0);
}

// Thread para receber e processar mensagens do servidor
void* recebe_mensagens_auto(void* arg) {
    char buffer[MAX_MSG];
    int sock = *(int*)arg;

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            printf("Auto-Client: Servidor desconectado.\n");
            exit(0);
        }
        buffer[n] = '\0';
        printf("Auto-Client Received: %s\n", buffer);

        if (strncmp(buffer, CMD_PLAY, strlen(CMD_PLAY)) == 0) {
            // É a vez deste cliente, envia um tiro aleatório
            int x, y;
            do {
                x = rand() % L;
                y = rand() % C;
            } while (tabuleiro_ataque[x][y] != '~');
            
            char fire_cmd[50];
            snprintf(fire_cmd, sizeof(fire_cmd), "FIRE %d %d", x + 1, y + 1);
            send(sock, fire_cmd, strlen(fire_cmd), 0);
            printf("Auto-Client Sent: %s\n", fire_cmd);
             tabuleiro_ataque[x][y] = ' '; // Marca como tentado

        } else if (strcmp(buffer, CMD_END) == 0) {
            printf("Auto-Client: Jogo terminou.\n");
            close(sock);
            exit(0);
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <nome_do_jogador>\n", argv[0]);
        return 1;
    }

    a_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    char buffer[MAX_MSG] = {0};

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(a_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Erro de conexão");
        return 1;
    }

    read(a_sock, buffer, MAX_MSG); // Mensagem de boas-vindas
    printf("Auto-Client Received: %s\n", buffer);

    // Envia o comando JOIN
    snprintf(buffer, sizeof(buffer), "JOIN %s", argv[1]);
    send(a_sock, buffer, strlen(buffer), 0);
    printf("Auto-Client Sent: %s\n", buffer);

    // Aguarda o início do Jogo
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int n = recv(a_sock, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) exit(1);
        buffer[n] = '\0';
        printf("Auto-Client Received: %s\n", buffer);
        if (strstr(buffer, "JOGO INICIADO")) break;
    }

    srand(time(NULL) ^ getpid()); 
    for(int i=0; i<L; i++) for(int j=0; j<C; j++) tabuleiro_ataque[i][j] = '~';

    posiciona_navios_auto(a_sock);
    
    // Aguarda o início do jogo e processa os turnos
    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, recebe_mensagens_auto, &a_sock);
    pthread_join(recv_thread, NULL);

    close(a_sock);
    return 0;
}