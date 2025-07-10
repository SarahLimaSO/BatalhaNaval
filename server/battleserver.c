
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include "../common/protocol.h"
#include <string.h>

#define PORT 8080

char info[1024];
int jogadores_prontos = 0; // Indica quantos jogadores deram JOIN
int jogador_navios_ok = 0; //Indica quantos jogadores terminaram o seu posicionamento

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_inicio = PTHREAD_COND_INITIALIZER;
int jogadores_ready = 0;
int result_sorteio = 0; //guarda o valor sorteado para a escolha do jogador 1 e 2
int fez_sorteio = 0; // registra se foi feito o sorteio ou nao

int processa_comando(char cmd[1024], Jogador* player){

    if (strncmp(cmd, CMD_JOIN, 4) == 0) {
        char nome[22];

         if (sscanf(cmd, "JOIN %63s", nome) == 1) {
            strncpy(player->nome, nome, sizeof(player->nome) - 1);
            player->nome[sizeof(player->nome) - 1] = '\0';

            send(player->socket, "JOGO INICIADO!\n", strlen("JOGO INICIADO!\n"), 0);
            
            return 1;  
        }
    }
    
    else if(strncmp(cmd, CMD_POS, 3) == 0){
        char tipo[20];
        int x, y;
        char orientacao;
        
        if (sscanf(cmd, "POS %19s %d %d %c", tipo, &x, &y, &orientacao) == 4) {
            return 2;
        }
    }
    else if(strncmp(cmd, CMD_READY, 5) == 0){
        printf("\n");
        return 3;
    }
    fprintf(stderr, "Erro: comando desconhecido.\n");
    return 0;
}

// Sorteia qual jogador sera o 1 e 2
int sorteia_player(){
    srand(time(NULL));
    return rand() % 2; 
}

// Inicializa o tabuleiro
void inicializa_tabuleiro(char tab[L][C]){
   
    for (int i = 0; i < L; i++) {
        for (int j = 0; j < C; j++) {
            tab[i][j] = '~';
        }
    }
}

//Transforma o tabuleiro em str para que possa ser passado ao cliente
void tabuleiro_em_str(Jogador* player, char* tab_str){

    char line[1024];
    strcpy(tab_str, "\n   ");
    for (int col = 0; col < C; col++) {
        char num[10];
        sprintf(num, " %2d ", col + 1);
        strcat(tab_str, num);
    }
    strcat(tab_str, "\n");

    for (int i = 0; i < L; i++) {

        // Se a fase de posicionamento foi concluida ele imprime o tabuleiro com letras
        if (player->posicionamento_ok){
            sprintf(line, "%c  ", 'A' + i);
        }//Se nao ele imrime com indices em numero
        else{
            sprintf(line, "%d  ", i + 1);
            strcat(tab_str, line);
        }
        for (int j = 0; j < C; j++) {
            sprintf(line, "| %c ", player->tab[i][j]);
            strcat(tab_str, line);
        }
        strcat(tab_str, "|\n");
    }
}

// Espera o jogador dar ready
void jogador_ready(Jogador* player) {
    char buffer[1024];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int n = recv(player->socket, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            printf("Cliente desconectado ao esperar READY.\n");
            return;
        }
        buffer[n] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0;

        if (strcasecmp(buffer, "READY") == 0) {
            pthread_mutex_lock(&lock);
            jogadores_ready++;

            if (jogadores_ready == 1) {
                send(player->socket, "**AGUARDANDO ADVERSÁRIO...**\n", 30, 0);
                while (jogadores_ready < 2) {
                    pthread_cond_wait(&cond_inicio, &lock);
                }
            } else if (jogadores_ready == 2) {
                pthread_cond_broadcast(&cond_inicio);
            }

            pthread_mutex_unlock(&lock);
            send(player->socket, "**INÍCIO DO JOGO!**\n", 21, 0);
            break;
        } else {
            send(player->socket, "!!Comando inválido! Use READY!!\n", 33, 0);
        }
    }
}

int posiciona_navio(Jogador *player, char tipo[20], int x, int y, char orientacao){
    int tamanho;
    char simb;

    // Verifica qual navio foi escolhido
    if (strcasecmp(tipo, "SUBMARINO") == 0) {
        if (player->total_sub >= MAX_SUB) return -1; //Sinaliza que numero maximo de barcos desse tipo foi atingido
        tamanho = 1;
        simb = '*';
    } else if (strcasecmp(tipo, "FRAGATA") == 0) {
        if (player->total_frag >= MAX_FRAG) return -1; //Sinaliza que numero maximo de barcos desse tipo foi atingido
        tamanho = 2;
        simb = '$';
    } else if (strcasecmp(tipo, "DESTROYER") == 0) {
        if (player->total_dest >= MAX_DEST) return -1; //Sinaliza que numero maximo de barcos desse tipo foi atingido
        tamanho = 3;
        simb = '#';
    }
    else return 0;
    
    if (x < 0 || x >= L || y < 0 || y >= C) {
        return 0; 
    }

    // Verifica limites e sobreposição
    if (orientacao == 'H' || orientacao == 'h') {
        if (y + tamanho > C) return 0;

        for (int i = 0; i < tamanho; i++) {
            if (player->tab[x][y + i] != '~') return 0; // Já tem navio
        }

        for (int i = 0; i < tamanho; i++) {
            player->tab[x][y + i] = simb;
        }

        // Incrementa o numero de barcos colocados de acordo com o seu tipo
        if (strcasecmp(tipo, "SUBMARINO") == 0) {
            player->total_sub++;
        } else if (strcasecmp(tipo, "FRAGATA") == 0) {
            player->total_frag++;
        } else if (strcasecmp(tipo, "DESTROYER") == 0) {
            player->total_dest++;
        }

    } 
    else if (orientacao == 'V' || orientacao == 'v') {
        if (x + tamanho > L) return 0;

        for (int i = 0; i < tamanho; i++) {

            // Caso a posicao ja estaja ocupada
            if (player->tab[x + i][y] != '~') return 0; 
        }

        for (int i = 0; i < tamanho; i++) {
            player->tab[x + i][y] = simb;
        }

        // Incrementa o numero de barcos colocados de acordo com o seu tipo
        if (strcasecmp(tipo, "SUBMARINO") == 0) {
            player->total_sub++;
        } else if (strcasecmp(tipo, "FRAGATA") == 0) {
            player->total_frag++;
        } else if (strcasecmp(tipo, "DESTROYER") == 0) {
            player->total_dest++;
        }

    } else return 0;

    return 1;
}

void posicionamento_player(Jogador* player ) {
    char buffer[1024];
    char tab_str[2048];

    int msg_enviada = 0; // Flag para enviar a mensagem de confirmação apenas uma vez

    while (1) {
        memset(buffer, 0, sizeof(buffer));

        int n = recv(player->socket, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            perror("Erro ao receber dados do cliente");
            break;
        }

        buffer[n] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0;  // Remove \r e \n

        // Comando READY - Verifica se todos os navios foram posicionados
        if (strcasecmp(buffer, CMD_READY) == 0) {
            if (player->total_sub == MAX_SUB && player->total_frag == MAX_FRAG && player->total_dest == MAX_DEST) {

                pthread_mutex_lock(&lock);
                player->posicionamento_ok = 1;
                jogador_navios_ok++;

                if (jogador_navios_ok < 2) {
                    // Primeiro jogador a digitar READY espera o segundo
                    snprintf(msg, sizeof(msg), "**AGUARDANDO ADVERSÁRIO...**\n");
                    send(player->socket, msg, strlen(msg), 0);

                    // Espera o outro jogador dar READY
                    while (jogador_navios_ok < 2) {
                        pthread_cond_wait(&cond_inicio, &lock);
                    }

                } else {
                    // Segundo jogador libera o outro
                    pthread_cond_broadcast(&cond_inicio);
                }

                pthread_mutex_unlock(&lock);

                // Ambos os jogadores recebem início do jogo
                snprintf(msg, sizeof(msg), "**INÍCIO DO JOGO!**\n");
                send(player->socket, msg, strlen(msg), 0);
                break;

            } else {
                snprintf(msg, sizeof(msg),
                    "!!Você precisa posicionar todos os navios antes de enviar READY!!\n"
                    "- Submarinos: %d/%d\n- Fragatas: %d/%d\n- Destroyers: %d/%d\n",
                    player->total_sub, MAX_SUB,
                    player->total_frag, MAX_FRAG,
                    player->total_dest, MAX_DEST
                );
                send(player->socket, msg, strlen(msg), 0);
                continue;
            }
        }

        // Comando POS
        if (strncmp(buffer, CMD_POS, 3) == 0) {
            char tipo[20];
            int x, y;
            char orientacao;

            if (sscanf(buffer, "POS %s %d %d %c", tipo, &x, &y, &orientacao) == 4) {
                x -= 1;
                y -= 1;

                int sucesso = posiciona_navio(player, tipo, x, y, orientacao);
                tabuleiro_em_str(player, tab_str);

                if (sucesso == 1) {
                    snprintf(msg, sizeof(msg), "**Navio posicionado**\n%s", tab_str);
                    send(player->socket, msg, strlen(msg), 0);

                    if ((player->total_sub == MAX_SUB && player->total_frag == MAX_FRAG && player->total_dest == MAX_DEST) && !msg_enviada) {
                        snprintf(msg, sizeof(msg), "**Todos os navios posicionados. Envie READY para confirmar.**\n");
                        send(player->socket, msg, strlen(msg), 0);
                        msg_enviada = 1;
                    }
                } 
                else if (sucesso == -1) {
                    snprintf(msg, sizeof(msg), "!!Limite máximo de %s atingido!!\n%s", tipo, tab_str);
                    send(player->socket, msg, strlen(msg), 0);
                } 
                else {
                    snprintf(msg, sizeof(msg), "!!Erro ao posicionar navio!!\n%s\nPosicionamento inválido. Tente novamente.\n", tab_str);
                    send(player->socket, msg, strlen(msg), 0);
                } 
            } else {
                send(player->socket, "!!Formato inválido!!\n", strlen("!!Formato inválido!!\n"), 0);
            }
        } 
        else if (strcasecmp(buffer, CMD_READY) != 0) {
            send(player->socket, "!!Comando desconhecido!!\n", strlen("!!Comando desconhecido!!\n"), 0);
        }
    }

    // Fim do posicionamento
    jogador_ready(player);
    pthread_exit(NULL);
}

void turnos_jogo(Jogador* player1, Jogador* player2, int jogador_inicial) {
    int jogador_atual = jogador_inicial;
    char buffer[1024];

    while (1) {
        Jogador *atual = (jogador_atual == 1) ? player1 : player2;
        Jogador *outro = (jogador_atual == 1) ? player2 : player1;

        // Informações de turno
        send(atual->socket, "\n>> Seu turno! Use: FIRE <x> <y>\n", 34, 0);
        send(outro->socket, "\n>> Aguarde o turno do outro jogador...\n", 40, 0);

        // Recebe jogada
        memset(buffer, 0, sizeof(buffer));
        int n = recv(atual->socket, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            printf("Jogador desconectado.\n");
            break;
        }

        buffer[n] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0;

        int x, y;
        if (sscanf(buffer, "FIRE %d %d", &x, &y) == 2) {
            printf("Jogador %d disparou em (%d, %d)\n", jogador_atual, x, y);

            // TODO: lógica de acerto/erro etc.
            send(atual->socket, ">> Jogada registrada!\n", 23, 0);
            send(outro->socket, ">> O adversário jogou. Agora é seu turno!\n", 43, 0);

            // Troca de jogador
            jogador_atual = (jogador_atual == 1) ? 2 : 1;
        } else {
            send(atual->socket, "!! Comando inválido. Use: FIRE <x> <y>\n", 39, 0);
        }
    }
}


//Recebe o comando JOIN dos jogadores e posiciona navios
void* recebe_jogador(void* arg) {
    Jogador* player = (Jogador*) arg;
    char buffer[1024];
    char tab_str[2048];

    player->posicionamento_ok = 0; //Inicializa a flag do jogador indicando que o posicionamento ainda nao foi concluido

    int n = recv(player->socket, buffer, sizeof(buffer), 0);
    if (n <= 0) {
        perror("Erro ao receber JOIN do cliente");
        close(player->socket);
        pthread_exit(NULL);
        exit(1);
    }
    sscanf(buffer, "JOIN %63s", player->nome);

    // Incrementa número de jogadores prontos com proteção
    pthread_mutex_lock(&lock);
    jogadores_prontos++;

    if (jogadores_prontos < 2) {
        send(player->socket, "AGUARDE JOGADOR\n", strlen("AGUARDE JOGADOR\n"), 0);

        // Espera até que o outro jogador esteja pronto
        pthread_cond_wait(&cond_inicio, &lock);
    } else {
        // Se o segundo jogador estiver pronto libera todas as threads
        pthread_cond_broadcast(&cond_inicio);
    }
    pthread_mutex_unlock(&lock);

    // Inicializa o tabuleiro do jogador
    inicializa_tabuleiro(player->tab);
    tabuleiro_em_str(player, tab_str); //Transforma o tabuleiro em str
    
    char msgInicial[4096];

    // Envia msg aos jogadores informando o inicio do jogo, o tabuleiro inicial, e se ele eh jogador 1 ou 2
    sprintf(msgInicial, "JOGO INICIADO\n [Seu tabuleiro]%s\nVocê é o jogador %d\n", tab_str, player->id);
    send(player->socket, msgInicial,strlen(msgInicial), 0); // Manda a msg JOGO INICIADO + tabuleiro inicial

    // Inicializa o total de navios posicionados pelo jogador
    player->total_sub = 0;
    player->total_frag = 0;
    player->total_dest = 0;
    
    // Posiciona os barcos
    posicionamento_player(player);

    return NULL;
}

int main() {

    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Deixa a port aser reutilizada (perguntar do prof como ele quer q resolvamos esse problema !!!!!!!!!!)
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind error\n");
        exit(1);
    }
    if (listen(server_fd, 2) < 0) {
        perror("listen error\n");
        exit(1);
    } // até dois jogadores

    printf("Servidor aguardando jogadores na porta %d...\n", PORT);

    Jogador player1, player2;

    player1.socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
    player2.socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
    
    if (player1.socket < 0) perror("Accept player 1 failed");
    if (player2.socket < 0) perror("Accept player 2 failed");

    send(player1.socket, "<<[Bem vind@ ao jogo Batalha Naval!]>>\n", strlen("<<[Bem vind@ ao jogo Batalha Naval!]>>\n"), 0);
    send(player2.socket, "<<[Bem vind@ ao jogo Batalha Naval!]>>\n", strlen("<<[Bem vind@ ao jogo Batalha Naval!]>>\n"), 0);

    // Faz o sorteio pra decidir qual jogador eh o 1 e 2
    int sort = sorteia_player();  // retorna 0 ou 1

    if (sort == 0) {
        player1.id = 1;  // jogador 1
        player2.id = 2;  // jogador 2
    } else {
        player1.id = 2;
        player2.id = 1;
    }

    // Cria uma thread para cada jogador
    pthread_t threads[2];
    pthread_create(&threads[0], NULL, recebe_jogador, &player1);
    pthread_create(&threads[1], NULL, recebe_jogador, &player2);

    // Aguarda ambas terminarem
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);

    int iniciante = (sort == 0) ? 1 : 2;
    turnos_jogo(&player1, &player2, iniciante);

    // Fecha os sockets dos jogadores
    close(player1.socket);
    close(player2.socket);

    // Fecha o socket do servidor
    close(server_fd);
    return 0;
}
