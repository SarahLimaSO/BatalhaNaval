#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "../common/protocol.h"

#define PORT 8080

// Variáveis globais para o tabuleiro de ataque
char tabuleiro_ataque[L][C];
pthread_mutex_t a_lock = PTHREAD_MUTEX_INITIALIZER;
int ultimo_tiro_x = -1, ultimo_tiro_y = -1;

void inicializa_tabuleiro_ataque();
void imprime_tabuleiro_ataque();
void le_posicionamento_navios(int sock);
void prepara_inicio_jogo(int sock);
void inicia_jogo(int sock);
void* recebe_mensagens(void* arg);

// Inicializa o tabuleiro de ataque com água
void inicializa_tabuleiro_ataque() {
    for (int i = 0; i < L; i++) {
        for (int j = 0; j < C; j++) {
            tabuleiro_ataque[i][j] = '~';
        }
    }
}

// Imprime o tabuleiro de ataque
void imprime_tabuleiro_ataque() {
    printf("\n** SEU TABULEIRO DE ATAQUE **\n");
    char line[1024];
    printf("   ");
    for (int col = 0; col < C; col++) {
        sprintf(line, " %2d ", col + 1);
        printf("%s", line);
    }
    printf("\n");

    for (int i = 0; i < L; i++) {
        sprintf(line, "%d  ", i + 1);
        printf("%s", line);
        for (int j = 0; j < C; j++) {
            sprintf(line, "| %c ", tabuleiro_ataque[i][j]);
            printf("%s", line);
        }
        printf("|\n");
    }
}

// Thread para receber mensagens do servidor
void* recebe_mensagens(void* arg) {
    int sock = *(int*)arg;
    char buffer[MAX_MSG];

    while (1) {
        int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) break;
        buffer[n] = '\0';

        pthread_mutex_lock(&a_lock);
        if (strncmp(buffer, CMD_HIT, strlen(CMD_HIT)) == 0) {
            if (ultimo_tiro_x != -1) tabuleiro_ataque[ultimo_tiro_x][ultimo_tiro_y] = '!';
            imprime_tabuleiro_ataque();
            printf("\nServidor: HIT!\n");
        } else if (strncmp(buffer, CMD_MISS, strlen(CMD_MISS)) == 0) {
            if (ultimo_tiro_x != -1) tabuleiro_ataque[ultimo_tiro_x][ultimo_tiro_y] = 'X';
            imprime_tabuleiro_ataque();
            printf("\nServidor: MISS!\n");
        } else if (strncmp(buffer, CMD_SUNK, strlen(CMD_SUNK)) == 0) {
            if (ultimo_tiro_x != -1) tabuleiro_ataque[ultimo_tiro_x][ultimo_tiro_y] = '!';
            imprime_tabuleiro_ataque();
            printf("\nServidor: SUNK!\n");
        } else if (strncmp(buffer, CMD_PLAY, strlen(CMD_PLAY)) == 0) {
             printf("\nÉ seu turno! Use FIRE <linha> <coluna> (ex: FIRE 1 1)\n> ");
        } else if (strcmp(buffer, CMD_END) == 0) {
            printf("O jogo terminou. Conexão encerrada.\n");
            close(sock);
            exit(0);
        } else {
             printf("\nServidor: %s\n", buffer);
        }
        pthread_mutex_unlock(&a_lock);
        fflush(stdout);
    }
    return NULL;
}


void le_posicionamento_navios(int sock) {
    char buffer[MAX_MSG];
    int navios_posicionados = 0;
    printf("**Fase de Posicionamento de Navios**\n");
    printf("Use o formato: POS <TIPO> <L> <C> <H/V>\n");

    while (navios_posicionados < MAX_NAVIOS) {
        printf("> ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\r\n")] = '\0';
        if (strncmp(buffer, "POS ", 4) == 0) {
            send(sock, buffer, strlen(buffer), 0);
            memset(buffer, 0, sizeof(buffer));
            int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (n > 0) {
                buffer[n] = '\0';
                printf("%s\n", buffer);
                if (strstr(buffer, "**Navio posicionado**")) navios_posicionados++;
            }
        }
    }
}

void prepara_inicio_jogo(int sock) {
    char buffer[MAX_MSG];
    printf("\nDigite READY para sinalizar que você está pronto:\n> ");
    while (1) {
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\r\n")] = 0;
        if (strcasecmp(buffer, CMD_READY) == 0) {
            send(sock, CMD_READY, strlen(CMD_READY), 0);
            break;
        }
    }
    int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if(n > 0) {
        buffer[n] = '\0';
        printf("Servidor: %s\n", buffer);
        if (strstr(buffer, "**INÍCIO DO JOGO!**") == NULL) {
             n = recv(sock, buffer, sizeof(buffer)-1, 0);
             if(n > 0) buffer[n] = '\0';
             printf("Servidor: %s\n", buffer);
        }
    }
}

void inicia_jogo(int sock) {
    char buffer[MAX_MSG];
    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, recebe_mensagens, &sock);
    printf("\nO jogo começou! Aguarde seu turno para atacar.\n");

    while (1) {
        printf("> ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\r\n")] = '\0';
        if (strncmp(buffer, CMD_FIRE, strlen(CMD_FIRE)) == 0) {
            pthread_mutex_lock(&a_lock);
            sscanf(buffer, "FIRE %d %d", &ultimo_tiro_x, &ultimo_tiro_y);
            ultimo_tiro_x--; 
            ultimo_tiro_y--;
            pthread_mutex_unlock(&a_lock);
            send(sock, buffer, strlen(buffer), 0);
        }
    }
    pthread_join(recv_thread, NULL);
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    char buffer[MAX_MSG] = {0};

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    read(sock, buffer, MAX_MSG);
    printf("Servidor: %s", buffer);

    while (1) {
        printf("Para começar a jogar, digite JOIN <seu_nome>:\n> ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\r\n")] = '\0';
        
        char command[10], name_arg[50];
        if (sscanf(buffer, "%s %s", command, name_arg) == 2 && strcasecmp(command, CMD_JOIN) == 0) {
            send(sock, buffer, strlen(buffer), 0);
            break;
        } else {
            printf("Formato inválido. Por favor, use: JOIN <nome>\n");
        }
    }

    while (1) {
         int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
         if (n <= 0) exit(1);
         buffer[n] = '\0';
         printf("\nServidor: %s\n", buffer);
         if (strstr(buffer, "JOGO INICIADO")) break;
    }
    
    inicializa_tabuleiro_ataque();
    le_posicionamento_navios(sock);
    prepara_inicio_jogo(sock);
    inicia_jogo(sock);

    close(sock);
    return 0;
}