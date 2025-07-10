
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

    // PREFERIDO
    
    int total_navios = MAX_DEST + MAX_FRAG + MAX_SUB;
    int navios_pos = 0; //Numero de navios ja posicionados
   
    while (navios_pos < total_navios) {
        memset(buffer, 0, sizeof(buffer));

        int n = recv(player->socket, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            perror("Erro ao receber dados do cliente");
            break;
        }

        buffer[n] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0;  // Remove \n ou \r\n (possiveis caracteres indesejados)

        // Esperado: "POS SUBMARINO 4 3 H"
        if (strncmp(buffer, "POS", 3) == 0) {
            char tipo[20];
            int x, y;
            char orientacao;

            if (sscanf(buffer, "POS %s %d %d %c", tipo, &x, &y, &orientacao) == 4) {

                x -= 1;
                y -= 1;
    
                //Recebe o retorno indicando se o posicionamento foi bem sucedido
                int sucesso = posiciona_navio(player, tipo, x, y, orientacao);

                if (sucesso == 1) {
                    tabuleiro_em_str(player, tab_str);
                    snprintf(msg, sizeof(msg), "**Navio posicionado**\n%s", tab_str);
                    send(player->socket, msg, strlen(msg), 0);
                    navios_pos++;
                } 
                else if (sucesso == -1) {
                    tabuleiro_em_str(player, tab_str);
                    snprintf(msg, sizeof(msg), "!!Limite máximo de %s atingido!!\n%s", tipo, tab_str);
                    send(player->socket, msg, strlen(msg), 0);
                } 
                else {
                    tabuleiro_em_str(player, tab_str);
                    snprintf(msg, sizeof(msg), "!!Erro ao posicionar navio!!\n%s", tab_str);
                    send(player->socket, msg, strlen(msg), 0);
                } 
            }
            else {
                send(player->socket, "!!Formato inválido!!\n", strlen("!!Formato inválido!!\n"), 0);
            }
        } 
        else {
            send(player->socket, "!!Comando desconhecido!!\n", strlen("!!Comando desconhecido!!\n"), 0);
        }
    }

    // Fim do posicionamento
    pthread_mutex_lock(&lock);
    jogador_navios_ok++;

    player->posicionamento_ok = 1; //Sinaliza q esse jogador terminou de posicionar os seu navios
    
    if (jogador_navios_ok == 1) {
        send(player->socket,
            "**Todos os navios posicionados. Aguardando oponente...**\n",
            strlen("**Todos os navios posicionados. Aguardando oponente...**\n"), 0);
    } else {
        send(player->socket,
            "**Todos os navios posicionados.**\n",
            strlen("**Todos os navios posicionados.**\n"), 0);
    }
    pthread_mutex_unlock(&lock);
    pthread_exit(NULL);
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

    // Depois que os dois jogadores deram JOIN

    // Inicializa o tabuleiro do jogador
    inicializa_tabuleiro(player->tab);
    tabuleiro_em_str(player, tab_str); //Transforma o tabuleiro em str
    
    char msgInicial[4096];

    sprintf(msgInicial, "JOGO INICIADO\n [Seu tabuleiro]%s", tab_str);
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

    // Cria uma thread para cada jogador
    pthread_t threads[2];
    pthread_create(&threads[0], NULL, recebe_jogador, &player1);
    pthread_create(&threads[1], NULL, recebe_jogador, &player2);

    // Aguarda ambas terminarem
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);


    // Decide qual jogador eh o jogador 1 e 2
    // atribui_jogadores(player1.socket, player2.socket); //Sorteia qual dos jogadores eh o jogador 1 e 2  

    // Fecha os sockets dos jogadores
    close(player1.socket);
    close(player2.socket);

    // Fecha o socket do servidor
    close(server_fd);
    return 0;
}
