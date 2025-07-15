#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "../common/protocol.h"

#define PORT 8080

#define CMD_WAIT "AGUARDE O TURNO DO OUTRO JOGADOR"

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

        // Detecta mensagem de turno do jogador
        if (strstr(buffer, "<<PLAY") != NULL && strstr(buffer, "É SEU TURNO!") != NULL) {
            printf("\nServidor: %s\n", buffer);
            imprime_tabuleiro_ataque();
            printf("\n> ");
            fflush(stdout);
        }
        else if (strncmp(buffer, "PLAY ", 5) == 0 && strstr(buffer, "Aguarde o turno de") != NULL) {
            printf("\nServidor: %s\n", buffer);
            fflush(stdout);
        }
        else if (strncmp(buffer, "HIT", 3) == 0 || strncmp(buffer, "MISS", 4) == 0 || strncmp(buffer, "SUNK", 4) == 0) {
            if (ultimo_tiro_x >= 0 && ultimo_tiro_y >= 0) {
                if (strncmp(buffer, "MISS", 4) == 0)
                    tabuleiro_ataque[ultimo_tiro_x][ultimo_tiro_y] = 'o';
                else if (strncmp(buffer, "HIT", 3) == 0)
                    tabuleiro_ataque[ultimo_tiro_x][ultimo_tiro_y] = 'X';
                else if (strncmp(buffer, "SUNK", 4) == 0)
                    tabuleiro_ataque[ultimo_tiro_x][ultimo_tiro_y] = 'S';

                printf("\nServidor: %s\n", buffer);

                if (strncmp(buffer, "SUNK", 4) == 0) {
                    printf(">>> SUNK! Navio AFUNDADO! <<<\n");
                }
            }
            fflush(stdout);
        }
        else if (strncmp(buffer, CMD_WIN, strlen(CMD_WIN)) == 0) {
            printf("\n>>>>> WIN! VOCÊ VENCEU! <<<<<\n");
        }
        else if (strncmp(buffer, CMD_LOSE, strlen(CMD_LOSE)) == 0) {
            printf("\nXXXXX LOSE! VOCÊ PERDEU! XXXXX\n");
        }
        // Detecta fim do jogo pelo comando END
        else if (strncmp(buffer, CMD_END, strlen(CMD_END)) == 0) {
            printf("\nO jogo terminou. Conexão encerrada.\n");
            close(sock);
            exit(0);
        }
        else {
            printf("\nServidor: %s\n> ", buffer);
            fflush(stdout);
        }

        pthread_mutex_unlock(&a_lock);
    }
    return NULL;
}

void inicia_jogo(int sock) {
    char buffer[MAX_MSG];
    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, recebe_mensagens, &sock);

    while (1) {
        printf("> ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\r\n")] = '\0';

        if (strncmp(buffer, "FIRE", 4) == 0) {
            int x, y;
            if (sscanf(buffer, "FIRE %d %d", &x, &y) == 2) {
                ultimo_tiro_x = x - 1;
                ultimo_tiro_y = y - 1;
            }
            send(sock, buffer, strlen(buffer), 0);
        }
    }
    pthread_join(recv_thread, NULL);
}

void le_posicionamento_navios(int sock) {
    char buffer[MAX_MSG];
    int navios_posicionados = 0;
    printf("**Fase de Posicionamento de Navios**\n");
    printf("Use o formato: POS <TIPO> <L> <C> <H/V>\n");

    while (navios_posicionados < MAX_NAVIOS) {
        printf("> "); 
        fflush(stdout);

        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\r\n")] = '\0';

        if (strncmp(buffer, "POS ", 4) == 0) {
            send(sock, buffer, strlen(buffer), 0);

            memset(buffer, 0, sizeof(buffer));
            int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (n > 0) {
                buffer[n] = '\0';
                // Imprime exatamente o que recebeu, que já tem linhas e formatação do tabuleiro
                printf("%s", buffer);

                // Só incrementa se recebeu confirmação
                if (strstr(buffer, "**Navio posicionado**")) {
                    navios_posicionados++;
                }
            }
        } else {
            printf("Comando inválido, use o formato: POS <TIPO> <L> <C> <H/V>\n");
        }
    }
}

void prepara_inicio_jogo(int sock) {
    char buffer[MAX_MSG];

    while (1) {
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\r\n")] = 0;
        if (strcasecmp(buffer, CMD_READY) == 0) {
            send(sock, CMD_READY, strlen(CMD_READY), 0);
            break;
        } else {
            printf("> "); 
        }
    }

    int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (n > 0) {
        buffer[n] = '\0';
        printf("Servidor: %s\n", buffer);
    }
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