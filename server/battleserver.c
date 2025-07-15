#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "../common/protocol.h"

#define PORT 8080

// Variáveis globais para sincronização
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_inicio = PTHREAD_COND_INITIALIZER;
int jogadores_prontos = 0;
int jogador_navios_ok = 0;

// Declarações de Funções
void inicializa_tabuleiro(char tab[L][C]);
void tabuleiro_em_str(Jogador* player, char* tab_str);
int posiciona_navio(Jogador *player, char tipo[], int x, int y, char orientacao);
int verifica_afundou(Jogador* oponente, char simb);
int processa_tiro(Jogador *oponente, int x, int y, char *resposta);
void turnos_jogo(Jogador* player1, Jogador* player2, int jogador_inicial);
void* recebe_jogador(void* arg);

// Inicializa o tabuleiro com água
void inicializa_tabuleiro(char tab[L][C]) {
    for (int i = 0; i < L; i++) {
        for (int j = 0; j < C; j++) {
            tab[i][j] = '~'; // '~' representa água
        }
    }
}

// Converte o tabuleiro para string para envio ao cliente
void tabuleiro_em_str(Jogador* player, char* tab_str) {
    char line[1024];
    strcpy(tab_str, "\n   ");
    for (int col = 0; col < C; col++) {
        sprintf(line, " %2d ", col + 1);
        strcat(tab_str, line);
    }
    strcat(tab_str, "\n");

    for (int i = 0; i < L; i++) {
        sprintf(line, "%d  ", i + 1);
        strcat(tab_str, line);
        for (int j = 0; j < C; j++) {
            sprintf(line, "| %c ", player->tab[i][j]);
            strcat(tab_str, line);
        }
        strcat(tab_str, "|\n");
    }
}

// Posiciona um navio no tabuleiro do jogador
int posiciona_navio(Jogador *player, char tipo[], int x, int y, char orientacao) {
    int tamanho;
    char simb;

    if (strcasecmp(tipo, "SUBMARINO") == 0) {
        if (player->total_sub >= MAX_SUB) return -1;
        tamanho = 1;
        simb = '*';

    } else if (strcasecmp(tipo, "FRAGATA") == 0) {
        if (player->total_frag >= MAX_FRAG) return -1;
        tamanho = 2;

        // Diferencia as fragatas por símbolo
        if (player->total_frag == 0)
            simb = '$';  // Primeira fragata
        else
            simb = '&';  // Segunda fragata

    } else if (strcasecmp(tipo, "DESTROYER") == 0) {
        if (player->total_dest >= MAX_DEST) return -1;
        tamanho = 3;
        simb = '#';

    } else {
        return 0; // Tipo de navio inválido
    }

    // Validação de coordenadas
    if (x < 0 || x >= L || y < 0 || y >= C) return 0;

    // Verifica espaço disponível
    if (orientacao == 'H' || orientacao == 'h') {
        if (y + tamanho > C) return 0;
        for (int i = 0; i < tamanho; i++) {
            if (player->tab[x][y + i] != '~') return 0;
        }
        for (int i = 0; i < tamanho; i++) {
            player->tab[x][y + i] = simb;
        }

    } else if (orientacao == 'V' || orientacao == 'v') {
        if (x + tamanho > L) return 0;
        for (int i = 0; i < tamanho; i++) {
            if (player->tab[x + i][y] != '~') return 0;
        }
        for (int i = 0; i < tamanho; i++) {
            player->tab[x + i][y] = simb;
        }

    } else {
        return 0; // Orientação inválida
    }

    // Atualiza contador de navios posicionados
    if (strcasecmp(tipo, "SUBMARINO") == 0) player->total_sub++;
    else if (strcasecmp(tipo, "FRAGATA") == 0) player->total_frag++;
    else if (strcasecmp(tipo, "DESTROYER") == 0) player->total_dest++;

    return 1;
}

// Verifica se um navio foi completamente afundado
int verifica_afundou(Jogador* oponente, char simb) {
    for (int i = 0; i < L; i++) {
        for (int j = 0; j < C; j++) {
            if (oponente->tab[i][j] == simb) {
                return 0; // Ainda há partes não atingidas
            }
        }
    }
    return 1;
}

// Processa o tiro de um jogador
int processa_tiro(Jogador *oponente, int x, int y, char *resposta) {
    if (x < 0 || x >= L || y < 0 || y >= C) {
        strcpy(resposta, "Coordenadas inválidas. Tente novamente.\n");
        return -1;
    }

    char alvo = oponente->tab[x][y];

    if (alvo == '~') {
        oponente->tab[x][y] = 'o';
        strcpy(resposta, "MISS");
        return 0;
    } else if (alvo == '*' || alvo == '$' || alvo == '#' || alvo == '&') {
        char simbolo_original = alvo;
        printf("[DEBUG] Atingido (%d,%d) - Símbolo original: %c\n", x, y, simbolo_original);
        oponente->tab[x][y] = 'X';

        if (verifica_afundou(oponente, simbolo_original)) {
            // Marca todas as partes do navio como 'S'
            for (int i = 0; i < L; i++) {
                for (int j = 0; j < C; j++) {
                    if (oponente->tab[i][j] == 'X') {
                        oponente->tab[i][j] = 'S';
                    }
                }
            }
            strcpy(resposta, "SUNK");
            return 2;
        } else {
            strcpy(resposta, "HIT");
            return 1;
        }
    } else {
        strcpy(resposta, "Você já atirou aqui. Tente outra posição.\n");
        return -1;
    }
}

// Verifica se o jogo terminou
int game_over(Jogador *player) {
    for (int i = 0; i < L; i++) {
        for (int j = 0; j < C; j++) {
            if (player->tab[i][j] == '*' || player->tab[i][j] == '$' || player->tab[i][j] == '#' || player->tab[i][j] == '&') {
                return 0; // Ainda há navios
            }
        }
    }
    return 1; // Fim de jogo
}

void posicionamento_player(Jogador* player) {
    char buffer[MAX_MSG];
    char tab_str[MAX_MSG * 4];
    char msg[MAX_MSG * 5]; // Aumentado para evitar truncamento
    int msg_enviada = 0;

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int n = recv(player->socket, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            perror("Erro ao receber dados do cliente");
            break;
        }
        buffer[n] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0;

        if (strcasecmp(buffer, CMD_READY) == 0) {
            if (player->total_sub == MAX_SUB && player->total_frag == MAX_FRAG && player->total_dest == MAX_DEST) {
                pthread_mutex_lock(&lock);
                player->posicionamento_ok = 1;
                jogador_navios_ok++;
                if (jogador_navios_ok < 2) {
                    snprintf(msg, sizeof(msg), "**AGUARDANDO ADVERSÁRIO...**\n");
                    send(player->socket, msg, strlen(msg), 0);
                    while (jogador_navios_ok < 2) {
                        pthread_cond_wait(&cond_inicio, &lock);
                    }
                } else {
                    pthread_cond_broadcast(&cond_inicio);
                }
                pthread_mutex_unlock(&lock);
                snprintf(msg, sizeof(msg), "**INÍCIO DO JOGO!**\n");
                send(player->socket, msg, strlen(msg), 0);
                break;
            } else {
                snprintf(msg, sizeof(msg),
                    "!!Você precisa posicionar todos os navios antes de enviar READY!!\n"
                    "- Submarinos: %d/%d\n- Fragatas: %d/%d\n- Destroyers: %d/%d\n",
                    player->total_sub, MAX_SUB, player->total_frag, MAX_FRAG, player->total_dest, MAX_DEST);
                send(player->socket, msg, strlen(msg), 0);
                continue;
            }
        }

        if (strncmp(buffer, CMD_POS, 3) == 0) {
            char tipo[20];
            int x, y;
            char orientacao;
            if (sscanf(buffer, "POS %s %d %d %c", tipo, &x, &y, &orientacao) == 4) {
                x -= 1; y -= 1;
                int sucesso = posiciona_navio(player, tipo, x, y, orientacao);
                tabuleiro_em_str(player, tab_str);
                if (sucesso == 1) {
                    snprintf(msg, sizeof(msg), "**Navio posicionado**\n%s\n", tab_str);
                    if ((player->total_sub == MAX_SUB && player->total_frag == MAX_FRAG && player->total_dest == MAX_DEST) && !msg_enviada) {
                         strcat(msg, "\n**Todos os navios posicionados. Envie READY para confirmar.**\n> ");
                         msg_enviada = 1;
                    }
                } else if (sucesso == -1) {
                    snprintf(msg, sizeof(msg), "!!Limite máximo de %s atingido!!\n%s\n", tipo, tab_str);
                } else {
                    snprintf(msg, sizeof(msg), "!!Erro ao posicionar navio!!\n%s\nPosicionamento inválido. Tente novamente.\n", tab_str);
                }
                send(player->socket, msg, strlen(msg), 0);
            } else {
                send(player->socket, "!!Formato inválido!!\n", strlen("!!Formato inválido!!\n"), 0);
            }
        } else if (strcasecmp(buffer, CMD_READY) != 0) {
            send(player->socket, "!!Comando desconhecido!!\n", strlen("!!Comando desconhecido!!\n"), 0);
        }
    }
}

// Gerencia os turnos do jogo
void turnos_jogo(Jogador* player1, Jogador* player2, int jogador_inicial) {
    int jogador_atual = jogador_inicial;
    char buffer[MAX_MSG], resposta[MAX_MSG];
    int fim_jogo = 0;

    while (!fim_jogo) {
        Jogador* atual = (jogador_atual == player1->id) ? player1 : player2;
        Jogador* oponente = (jogador_atual == player1->id) ? player2 : player1;

        char msg[MAX_MSG * 4];

        // Mensagem para o jogador da vez
        snprintf(msg, sizeof(msg), "<<PLAY %d>>\nÉ SEU TURNO! Use FIRE <linha> <coluna> (ex: FIRE 1 1)\n", atual->id);
        send(atual->socket, msg, strlen(msg), 0);

        // Mensagem para o oponente
        snprintf(msg, sizeof(msg), "<<PLAY %d>>\nAguarde o turno de %s...\n", atual->id, atual->nome);
        send(oponente->socket, msg, strlen(msg), 0);

        // Recebe o comando do jogador
        memset(buffer, 0, sizeof(buffer));
        int n = recv(atual->socket, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) break;
        buffer[n] = '\0';

        int x, y;
        if (sscanf(buffer, "FIRE %d %d", &x, &y) == 2) {
            processa_tiro(oponente, x - 1, y - 1, resposta);
            
            char msg_atirador[MAX_MSG];
            char msg_oponente[MAX_MSG];

            if (strcmp(resposta, "MISS") == 0 || strcmp(resposta, "HIT") == 0 || strcmp(resposta, "SUNK") == 0) {
                // Atirador
                snprintf(msg_atirador, sizeof(msg_atirador), "%.1000s\n", resposta);
                send(atual->socket, msg_atirador, strlen(msg_atirador), 0);

                // Oponente
                snprintf(msg_oponente, sizeof(msg_oponente), "Você foi atingido em (%d, %d): %.980s\n", x, y, resposta);
                send(oponente->socket, msg_oponente, strlen(msg_oponente), 0);
            } else {
                snprintf(msg_atirador, sizeof(msg_atirador), "%.1000s\n", resposta);
                send(atual->socket, msg_atirador, strlen(msg_atirador), 0);
            }

            printf("[DEBUG] Resposta enviada: %s\n", resposta);

            if (game_over(oponente)) {
                // Envia WIN para jogador atual e LOSE para o oponente
                send(atual->socket, CMD_WIN, strlen(CMD_WIN), 0);
                send(oponente->socket, CMD_LOSE, strlen(CMD_LOSE), 0);

                // Envia mensagem organizada de navio afundado para o jogador atual
                send(atual->socket, ">>> NAVIO AFUNDADO! <<<\n", strlen(">>> NAVIO AFUNDADO! <<<\n"), 0);

                // Envia comando END para ambos para sinalizar fim do jogo
                send(atual->socket, CMD_END, strlen(CMD_END), 0);
                send(oponente->socket, CMD_END, strlen(CMD_END), 0);

                fim_jogo = 1;
            }
            else {
                jogador_atual = oponente->id;
            }
        } else {
            snprintf(msg, sizeof(msg), "Comando inválido. Use: FIRE <x> <y>\n");
            send(atual->socket, msg, strlen(msg), 0);
        }
    }

    send(player1->socket, CMD_END, strlen(CMD_END), 0);
    send(player2->socket, CMD_END, strlen(CMD_END), 0);
}

// Thread para cada jogador
void* recebe_jogador(void* arg) {
    Jogador* player = (Jogador*)arg;
    char buffer[MAX_MSG];
    char tab_str[MAX_MSG * 4];

    // Recebe o comando JOIN
    int n = recv(player->socket, buffer, sizeof(buffer) - 1, 0);
    if (n > 0) {
        buffer[n] = '\0';
        sscanf(buffer, "JOIN %63s", player->nome);
    }

    pthread_mutex_lock(&lock);
    jogadores_prontos++;
    if (jogadores_prontos < 2) {
        send(player->socket, "AGUARDE JOGADOR\n", strlen("AGUARDE JOGADOR\n"), 0);
        pthread_cond_wait(&cond_inicio, &lock);
    } else {
        pthread_cond_broadcast(&cond_inicio);
    }
    pthread_mutex_unlock(&lock);
    
    char msgInicial[MAX_MSG*5];
    inicializa_tabuleiro(player->tab);
    tabuleiro_em_str(player, tab_str);
    snprintf(msgInicial, sizeof(msgInicial), "JOGO INICIADO\n[Seu tabuleiro]%s\nVocê é o jogador %d\n", tab_str, player->id);
    send(player->socket, msgInicial, strlen(msgInicial), 0);

    player->total_sub = player->total_frag = player->total_dest = 0;
    posicionamento_player(player);

    return NULL;
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    // Criação do socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Erro ao criar o socket");
        exit(1);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Erro ao configurar o socket");
        exit(1);
    }

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Erro ao associar o socket");
        exit(1);
    }

    if (listen(server_fd, 2) < 0) {
        perror("Erro ao aguardar conexões");
        exit(1);
    }

    printf("Servidor aguardando jogadores na porta %d...\n", PORT);

    // Esperando por dois jogadores
    Jogador player1, player2;

    // Aceitando o primeiro jogador
    player1.socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
    if (player1.socket < 0) {
        perror("Erro ao aceitar conexão do jogador 1");
        exit(1);
    }
    printf("Jogador 1 conectado!\n"); 
    send(player1.socket, "<<[Bem vind@ ao jogo Batalha Naval!]>>\n", strlen("<<[Bem vind@ ao jogo Batalha Naval!]>>\n"), 0);

    // Aceitando o segundo jogador
    player2.socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
    if (player2.socket < 0) {
        perror("Erro ao aceitar conexão do jogador 2");
        exit(1);
    }
    printf("Jogador 2 conectado!\n");
    send(player2.socket, "<<[Bem vind@ ao jogo Batalha Naval!]>>\n", strlen("<<[Bem vind@ ao jogo Batalha Naval!]>>\n"), 0);

    srand(time(NULL));
    int sort = rand() % 2;
    if (sort == 0) {
        player1.id = 1;
        player2.id = 2;
    } else {
        player1.id = 2;
        player2.id = 1;
    }

    strcpy(player1.nome, "Player1");
    strcpy(player2.nome, "Player2");

    pthread_t threads[2];
    pthread_create(&threads[0], NULL, recebe_jogador, &player1);
    pthread_create(&threads[1], NULL, recebe_jogador, &player2);

    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);

    // Aguarda ambos os jogadores enviarem READY
    while (1) {
        pthread_mutex_lock(&lock);
        if (jogador_navios_ok == 2) {
            pthread_mutex_unlock(&lock);
            break;
        }
        pthread_mutex_unlock(&lock);
        sleep(1); 
    }

    // Ambos estão prontos — sorteio já ocorreu antes
    printf("Ambos os jogadores estão prontos. Iniciando o jogo...\n");

    // Inicia a função de turnos com base no sorteio
    turnos_jogo(&player1, &player2, (sort == 0) ? player1.id : player2.id);

    // Fechando os sockets dos jogadores e do servidor
    close(player1.socket);
    close(player2.socket);
    close(server_fd);

    return 0;
}

