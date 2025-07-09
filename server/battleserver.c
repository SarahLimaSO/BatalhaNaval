
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
#define L 8 //Numero de Linhas do tabuleiro
#define C 8 //Numero de Colunas do tabuleiro
#define MAX_SUB 1 //Numero maximo de submarinos
#define MAX_FRAG 2 //Numero maximo de fragatas
#define MAX_DEST  1 //Numero maximo de destroyers

typedef struct{
    int socket;
    char tab[L][C]; //Tabuleiro do jogador
}Jogador;

char info[1024];

int recebe_comando(const char* cmd){
    if (strncmp(cmd, CMD_JOIN, 4) == 0) {
        sscanf(cmd, "JOIN %s", info); //Guarda a informacao do jogador
        return 1;
    }
    else if(strncmp(cmd, CMD_POS, 3) == 0){
        return 2;
    }
    else if(strncmp(cmd, CMD_READY, 5) == 0){
        printf("\n");
    }
    return -1;
}

void atribui_jogadores(int player1, int player2){
    srand((unsigned)time(NULL));

    int sorteio = rand() % 2;

    if (sorteio == 0) {
        send(player1, "Você é o Jogador 1\n", 21, 0);
        send(player2, "Você é o Jogador 2\n", 21, 0);
    } else {
        send(player2, "Você é o Jogador 1\n", 21, 0);
        send(player1, "Você é o Jogador 2\n", 21, 0);
    }
}

// Processa o comando enviado pelo cliente
void processa_tipoCmd(int tipoCmd, Jogador player1, Jogador player2){
    switch(tipoCmd){
        case 1:
            send(player1.socket, "JOGO INICIADO!\n", strlen("JOGO INICIADO!\n"), 0);
            send(player2.socket, "JOGO INICIADO!\n", strlen("JOGO INICIADO!\n"), 0);

            atribui_jogadores(player1.socket, player2.socket); //Sorteia qual dos jogadores eh o jogador 1 e 2  
            break;
        case 2:
            send(player1.socket, "**Posicione os seus navios**\n", strlen("**Posicione os seus navios**\n"), 0);
            send(player2.socket, "**Posicione os seus navios**\n", strlen("**Posicione os seus navios**\n"), 0);
            break;
    }
    return;
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

void* posicionamento_thread(void* arg) {
    Jogador* jogador = (Jogador*) arg;
    char buffer[1024];

    int total_navios = MAX_DEST + MAX_FRAG + MAX_SUB;
    int navios_pos = 0; //Numero de navios ja posicionados

    while (navios_pos < total_navios) {
        memset(buffer, 0, sizeof(buffer));

        int n = recv(jogador->socket, buffer, sizeof(buffer) - 1, 0);
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
                int sucesso = posiciona_navio(jogador->tab, tipo, x, y, orientacao);
                if (sucesso) {
                    send(jogador->socket, "**Navio posicionado**\n", strlen("**Navio posicionado**\n"), 0);
                    navios_pos++;
                } else {
                    send(jogador->socket, "!!Erro ao posicionar navio!!\n", strlen("!!Erro ao posicionar navio!!\n"), 0);
                }
            } else {
                send(jogador->socket, "!!Formato inválido!!\n", strlen("!!Formato inválido!!\n"), 0);
            }
        } else {
            send(jogador->socket, "!!Comando desconhecido!!\n", strlen("!!Comando desconhecido!!\n"), 0);
        }
    }

    // Fim do posicionamento
    send(jogador->socket, "**Todos os navios posicionados. Aguardando oponente...**\n",
    strlen("**Todos os navios posicionados. Aguardando oponente...**\n"), 0);

    pthread_exit(NULL);
}

// void* posicionamento_thread(void* arg) {
//     Jogador* jogador = (Jogador*) arg;
//     char buffer[1024];

//     int max_barcos = MAX_DEST + MAX_FRAG + MAX_SUB;
//     int i = 0;

//     //Deve receber a informacao do servidor acerca do posicionamento e processa-la
//     while (i <= max_barcos) {
//         memset(buffer, 0, sizeof(buffer));
//         recv(jogador->socket, buffer, sizeof(buffer), 0);

//         if (strncmp(buffer, "POS", 3) == 0) {
//             char tipo[20];
//             int x, y;
//             char orientacao;
//             sscanf(buffer, "POS %s %d %d %c", tipo, &x, &y, &orientacao);

//             //Recebe se o posicionamento foi bem sucedido ou n
//             int resultado = posiciona_navio(jogador->tab, tipo, x, y, orientacao);

//             if (resultado == 1) {
//                 send(jogador->socket, "**Navio posicionado**\n", strlen("**Navio posicionado**\n"), 0);
//                 i++;
//             } else {
//                 send(jogador->socket, "!!Erro ao posicionar navio!!\n", strlen("!!Erro ao posicionar navio!!\n"), 0);
//             }
//         }
//     }

//     printf("Todos os navios foram posicionados!\n");
//     print_tabuleiro(jogador->tab); // Exibe o tabuleiro final

//     pthread_exit(NULL);
// }

int main() {

    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind\n");
        exit(1);
    }
    if (listen(server_fd, 2) < 0) {
        perror("listen\n");
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


    // Lendo os comandos do jogador
    char buffer[1024];
    recv(server_fd, buffer, sizeof(buffer), 0);

    int tipoCmd = recebe_comando(buffer);

    processa_tipoCmd(tipoCmd, player1, player2); //Processa e reaje conforme o cmd recebido

    // Inicializa tabuleiros
    inicializa_tabuleiro(player1.tab);
    inicializa_tabuleiro(player2.tab);

    // Cria uma thread para cada jogador
    pthread_t threads[2];
    pthread_create(&threads[0], NULL, posicionamento_thread, &player1);
    pthread_create(&threads[1], NULL, posicionamento_thread, &player2);

    // Aguarda ambas terminarem
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);

    // Fecha os sockets dos jogadores
    close(player1.socket);
    close(player2.socket);

    // Fecha o socket do servidor
    close(server_fd);
    return 0;
}
