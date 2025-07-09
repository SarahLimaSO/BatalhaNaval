
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
int jogadores_prontos = 0;
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

int sorteia_player(int player1, int player2){
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

// Imprime tabuleiro
void print_tabuleiro(char tab[L][C]){

    //Identificando a numeracao das colunas
    
    printf("  ");
    for(int i = 0; i < C; i++){
        printf("  %d ", i+1);
    }
    putchar('\n');

    //Identificando as linhas do tabuleiro
    for(int i = 0; i < L; i++){
        
        printf("%c ", i+65); //Usando a tabela ascii para imprimir as letras
       
        // Imprimindo o tabuleiro
        for (int j = 0; j < C; j++) {
            putchar('|');
            printf(" %c ", tab[i][j]);
        }
        putchar('|');
        putchar('\n');
    }
}

int posiciona_navio(char tab[L][C], char tipo[20], int x, int y, char orientacao){
    
    //Falta tratar caso de sobreposicao !!!!!!
    if (x < 0 || x >= L || y < 0 || y >= C) {
        return 0; 
    }
    
    if (strncmp(tipo, "SUBMARINO", strlen("SUBMARINO")) == 0) tab[x][y] = '*';
    
    if (strncmp(tipo, "FRAGATA", strlen("FRAGATA")) == 0){
        
        if(orientacao == 'H'){
            if(y+1 < C){
                tab[x][y] = '$';
                tab[x][y+1] = '$';
                return 1;
            }
            else{
                tab[x][y-1] = '$';
                tab[x][y] = '$';
                return 1;
            }
        }
        else{
            if(x+1 < L){
                tab[x][y] = '$';
                tab[x+1][y] = '$';
                return 1;
            }
            else{
                tab[x-1][y] = '$';
                tab[x][y] = '$';
                return 1;
            }
        }
    }
    if (strncmp(tipo, "DESTROYER", strlen("DESTROYER")) == 0){
        
        if(orientacao == 'H'){
            if(y+2 < C){
                tab[x][y] = '#';
                tab[x][y+1] = '#';
                tab[x][y+2] = '#';
                return 1;
            }
            else if(y+1 < C){
                tab[x][y-1] = '#';
                tab[x][y] = '#';
                tab[x][y+1] = '#';
                return 1;
            }
            else{
                tab[x][y-2] = '#';
                tab[x][y-1] = '#';
                tab[x][y] = '#';
                return 1;
            }
        }
        else{
            if(x+2 < L){
                tab[x][y] = '#';
                tab[x+1][y] = '#';
                tab[x+2][y] = '#';
                return 1;
            }
            else if(x+1 < L){
                tab[x-1][y] = '#';
                tab[x][y] = '#';
                tab[x+1][y] = '#';
                return 1;
            }
            else{
                tab[x-2][y] = '#';
                tab[x-1][y] = '#';
                tab[x][y] = '#';
                return 1;
            }
        }
    }
    return 0;
}

void posicionamento_player(Jogador* player ) {
    char buffer[1024];

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

        // Esperado: "POS SUBMARINO 4 3 H"
        if (strncmp(buffer, "POS", 3) == 0) {
            char tipo[20];
            int x, y;
            char orientacao;

            if (sscanf(buffer, "POS %s %d %d %c", tipo, &x, &y, &orientacao) == 4) {

                //Recebe o retorno indicando se o posicionamento foi bem sucedido
                int sucesso = posiciona_navio(player->tab, tipo, x, y, orientacao);
                if (sucesso) {
                    send(player->socket, "**Navio posicionado**\n", strlen("**Navio posicionado**\n"), 0);
                    navios_pos++;
                } else {
                    send(player->socket, "!!Erro ao posicionar navio!!\n", strlen("!!Erro ao posicionar navio!!\n"), 0);
                }
            } else {
                send(player->socket, "!!Formato inválido!!\n", strlen("!!Formato inválido!!\n"), 0);
            }
        } else {
            send(player->socket, "!!Comando desconhecido!!\n", strlen("!!Comando desconhecido!!\n"), 0);
        }
    }

    // Fim do posicionamento
    send(player->socket, "**Todos os navios posicionados. Aguardando oponente...**\n",
    strlen("**Todos os navios posicionados. Aguardando oponente...**\n"), 0);

    pthread_exit(NULL);
}

//Recebe o comando JOIN dos jogadores e posiciona navios
void* recebe_jogador(void* arg) {
    Jogador* player = (Jogador*) arg;
    char buffer[1024];

    int n = recv(player->socket, buffer, sizeof(buffer), 0);
    if (n <= 0) {
        perror("Erro ao receber JOIN do cliente");
        close(player->socket);
        pthread_exit(NULL);
    }
    buffer[n] = '\0'; //
    if (sscanf(buffer, "JOIN %63s", player->nome) != 1) {
        send(player->socket, "!!Comando JOIN inválido!!\n", 27, 0);
        close(player->socket);
        pthread_exit(NULL);
    }

    // Incrementa número de jogadores prontos com proteção
    pthread_mutex_lock(&lock);
    jogadores_prontos++;

    if (jogadores_prontos < 2) {
        send(player->socket, "AGUARDE JOGADOR\n", 17, 0);
        pthread_cond_wait(&cond_inicio, &lock);
    } else {
        pthread_cond_broadcast(&cond_inicio);
    }

    pthread_mutex_unlock(&lock);

    // Depois que os dois jogadores deram JOIN imprime
    send(player->socket, "JOGO INICIADO\n", 15, 0);

    // Inicializa o tabuleiro do jogador
    inicializa_tabuleiro(player->tab);

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

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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
