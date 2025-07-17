#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "../common/protocol.h"

#define PORT 8080

#define CMD_WAIT "AGUARDE O TURNO DO OUTRO JOGADOR"

// Tabuleiro de ataque do jogador (tamanho definido em protocol.h)
char tabuleiro_ataque[L][C];
// Mutex para proteger acesso ao tabuleiro durante concorrência com threads
pthread_mutex_t a_lock = PTHREAD_MUTEX_INITIALIZER;
// Coordenadas do último tiro disparado
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

        pthread_mutex_lock(&a_lock); // Protege acesso ao tabuleiro

        // Mensagem indicando que é o turno do jogador
        if (strstr(buffer, "<<PLAY") != NULL && strstr(buffer, "É SEU TURNO!") != NULL) {
            printf("\nServidor: %s\n", buffer);
            imprime_tabuleiro_ataque();
            printf("\n> ");
            fflush(stdout);
        }
        // Mensagem de espera
        else if (strncmp(buffer, "PLAY ", 5) == 0 && strstr(buffer, "Aguarde o turno de") != NULL) {
            printf("\nServidor: %s\n", buffer);
            fflush(stdout);
        }
        // Mensagens de resultado do tiro
        else if (strncmp(buffer, "HIT", 3) == 0 || strncmp(buffer, "MISS", 4) == 0 || strncmp(buffer, "SUNK", 4) == 0) {
            if (ultimo_tiro_x >= 0 && ultimo_tiro_y >= 0) {
                if (strncmp(buffer, "MISS", 4) == 0)
                    tabuleiro_ataque[ultimo_tiro_x][ultimo_tiro_y] = 'o'; // tiro na água
                else if (strncmp(buffer, "HIT", 3) == 0)
                    tabuleiro_ataque[ultimo_tiro_x][ultimo_tiro_y] = 'X'; // tiro acertou navio
                else if (strncmp(buffer, "SUNK", 4) == 0)
                    tabuleiro_ataque[ultimo_tiro_x][ultimo_tiro_y] = 'S'; // navio afundado

                printf("\nServidor: %s\n", buffer);

                if (strncmp(buffer, "SUNK", 4) == 0) {
                    printf(">>> Navio AFUNDADO! <<<\n");
                }
            }
            fflush(stdout);
        }
        // Verifica se venceu
        else if (strncmp(buffer, CMD_WIN, strlen(CMD_WIN)) == 0) {
            printf("\n>>>>> WIN! VOCÊ VENCEU! <<<<<\n");
            close(sock);
            exit(0);
        }
        // Verifica se perdeu
        else if (strncmp(buffer, CMD_LOSE, strlen(CMD_LOSE)) == 0) {
            printf("\nXXXXX LOSE! VOCÊ PERDEU! XXXXX\n");
            close(sock);
            exit(0);
        }
        // Verifica fim de jogo
        else if (strncmp(buffer, CMD_END, strlen(CMD_END)) == 0) {
            printf("\nO jogo terminou. Conexão encerrada.\n");
            close(sock);
            exit(0);
        }
        // Mensagens gerais
        else {
            printf("\nServidor: %s\n ", buffer);
            fflush(stdout);
        }

        pthread_mutex_unlock(&a_lock); // Libera acesso
    }
    return NULL;
}

// Loop principal do jogo (envio de tiros)
void inicia_jogo(int sock) {
    char buffer[MAX_MSG];
    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, recebe_mensagens, &sock);

    while (1) {
        printf("> ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\r\n")] = '\0'; // Remove quebra de linha

        if (strncmp(buffer, "FIRE", 4) == 0) {
            int x, y;
            if (sscanf(buffer, "FIRE %d %d", &x, &y) == 2) {
                ultimo_tiro_x = x - 1;
                ultimo_tiro_y = y - 1;
            }
            // Envia comando FIRE
            send(sock, buffer, strlen(buffer), 0); 
        }
    }
     // Aguarda thread
    pthread_join(recv_thread, NULL);
}

// Posicionamento dos navios no tabuleiro inicial
void le_posicionamento_navios(int sock) {
    char buffer[MAX_MSG];
    int navios_posicionados = 0;
    printf("**Fase de Posicionamento de Navios**\n\n");
    printf("Use o formato: POS <TIPO> <L> <C> <H/V>\n");

    while (navios_posicionados < MAX_NAVIOS) {
        printf("> ");
        fflush(stdout);

        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\r\n")] = '\0'; // Limpa entrada

        if (strncmp(buffer, "POS ", 4) == 0) {
            send(sock, buffer, strlen(buffer), 0);

            memset(buffer, 0, sizeof(buffer)); // Zera buffer
            int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (n > 0) {
                buffer[n] = '\0';
                printf("%s", buffer);

                // Verifica se navio foi aceito (foi posicionado)
                if (strstr(buffer, "**Navio posicionado**")) {
                    navios_posicionados++;
                }
            }
        } else {
            printf("Comando inválido, use o formato: POS <TIPO> <L> <C> <H/V>\n");
        }
    }
}

// Envio do comando READY para iniciar o jogo
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
        printf("\nServidor: %s\n", buffer);
    }
}

int main() {
    // Cria o socket para comunicação TCP com o servidor
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    // Define a estrutura com os dados do servidor
    struct sockaddr_in serv_addr;

    // Buffer para receber mensagens do servidor
    char buffer[MAX_MSG] = {0};

    // Configura o protocolo de comunicação como IPv4
    serv_addr.sin_family = AF_INET;

    // Define a porta a ser usada (convertida para o formato de rede)
    serv_addr.sin_port = htons(PORT);

    // Converte o IP "127.0.0.1" (localhost) para o formato binário e armazena
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    // Estabelece a conexão com o servidor
    connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    // Lê a mensagem inicial enviada pelo servidor (boas-vindas ou instruções)
    read(sock, buffer, MAX_MSG);
    printf("Servidor: %s", buffer);

    // Solicita ao jogador que envie o comando JOIN com seu nome
    while (1) {
        printf("Para começar a jogar, digite JOIN <seu_nome>:\n> ");
        fgets(buffer, sizeof(buffer), stdin); // Lê a linha digitada
        buffer[strcspn(buffer, "\r\n")] = '\0'; // Remove caracteres de nova linha

        // Extrai o comando e o nome digitado
        char command[10], name_arg[50];
        if (sscanf(buffer, "%s %s", command, name_arg) == 2 && strcasecmp(command, CMD_JOIN) == 0) {
            // Se o comando estiver correto, envia ao servidor
            send(sock, buffer, strlen(buffer), 0);
            break;
        } else {
            // Mensagem de erro para comando incorreto
            printf("Formato inválido. Por favor, use: JOIN <nome>\n");
        }
    }

    // Aguarda mensagens do servidor até receber a confirmação "JOGO INICIADO"
    while (1) {
        int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) exit(1); // Encerra se a conexão falhar
        buffer[n] = '\0';
        printf("\nServidor: %s\n", buffer);
        if (strstr(buffer, "JOGO INICIADO")) break; // Sai do loop se o jogo começar
    }

    // Inicializa o tabuleiro do jogador com água
    inicializa_tabuleiro_ataque();

    // Inicia a fase de posicionamento dos navios
    le_posicionamento_navios(sock);

    // Espera o jogador digitar READY para iniciar o jogo
    prepara_inicio_jogo(sock);

    // Entra no loop principal do jogo (turnos, disparos, etc.)
    inicia_jogo(sock);

    // Encerra o socket e termina o programa
    close(sock);
    return 0;
}