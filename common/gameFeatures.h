#include <stdio.h>
#include <stdlib.h>

#define L 8 //Numero de Linhas do tabuleiro
#define C 8 //Numero de Colunas do tabuleiro
#define MAX_SUB 1 //Numero maximo de submarinos
#define MAX_FRAG 2 //Numero maximo de fragatas
#define MAX_DEST  1 //Numero maximo de destroyers

typedef struct{
    int socket;
    char tab[L][C]; //Tabuleiro do jogador
}Jogador;

// Inicializa o tabuleiro
void inicializa_tabuleiro(char tab[L][C]);

// Imprime tabuleiro
void print_tabuleiro(char tab[L][C]);

// Posiciona os barcos no tabuleiro
void* posiciona_barcos(void *arg);

int posiciona_navio(char tab[L][C], char tipo[20], int x, int y, char orientacao);

void* posicionamento_thread(void* arg);

