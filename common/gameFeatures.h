#include <stdio.h>
#include <stdlib.h>

#define L 8 //Numero de Linhas
#define C 8 //Numero de Colunas

// Inicializa o tabuleiro
void inicializa_tabuleiro(char tab[L][C]);

// Imprime tabuleiro
void print_tabuleiro(char tab[L][C]);

// Posiciona os barcos no tabuleiro
void* posiciona_barcos(void *arg);

// Processa as jogadas do cliente
void jogadas_cliente(char tab[L][C]);
