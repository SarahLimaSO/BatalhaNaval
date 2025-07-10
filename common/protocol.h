
#ifndef PROTOCOL_H
#define PROTOCOL_H

#define MAX_MSG 1024

// Comandos
#define CMD_JOIN "JOIN"
#define CMD_READY "READY"
#define CMD_POS "POS"
#define CMD_FIRE "FIRE"
#define CMD_HIT "HIT"
#define CMD_MISS "MISS"
#define CMD_SUNK "SUNK"
#define CMD_WIN "WIN"
#define CMD_LOSE "LOSE"

#endif

#define L 8 //Numero de Linhas do tabuleiro
#define C 8 //Numero de Colunas do tabuleiro
#define MAX_SUB 1 //Numero maximo de submarinos
#define MAX_FRAG 2 //Numero maximo de fragatas
#define MAX_DEST  1 //Numero maximo de destroyers

int MAX_NAVIOS = MAX_DEST + MAX_FRAG + MAX_SUB;
char msg[4096]; //Vetor auxiliar para envio de mensagens

typedef struct{
    int socket;
    char nome[22];
    int total_frag, total_dest, total_sub; // Total de navios posicionados de cada tipo
    int posicionamento_ok; //Sinaliza se o jogador ja terminou o seu posicionamento
    int id; //Indica se o jogador eh o 1 ou 2
    char tab[L][C]; //Tabuleiro do jogador
}Jogador;
