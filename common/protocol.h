#ifndef PROTOCOL_H
#define PROTOCOL_H

#define MAX_MSG 1024

// Comandos do Protocolo
#define CMD_JOIN "JOIN"
#define CMD_READY "READY"
#define CMD_POS "POS"
#define CMD_FIRE "FIRE"
#define CMD_PLAY "PLAY"
#define CMD_HIT "HIT"
#define CMD_MISS "MISS"
#define CMD_SUNK "SUNK"
#define CMD_WIN "WIN"
#define CMD_LOSE "LOSE"
#define CMD_END "END"

// Configurações do Tabuleiro e Navios
#define L 8  // Número de Linhas do tabuleiro
#define C 8  // Número de Colunas do tabuleiro

#define MAX_SUB 1    // Número máximo de submarinos
#define MAX_FRAG 2   // Número máximo de fragatas
#define MAX_DEST 1   // Número máximo de destroyers

#define MAX_NAVIOS (MAX_SUB + MAX_FRAG + MAX_DEST)

// Estrutura para representar um jogador
typedef struct {
    int socket;
    char nome[22];
    int total_frag, total_dest, total_sub; // Total de navios posicionados de cada tipo
    int posicionamento_ok; // Sinaliza se o jogador já terminou o seu posicionamento
    int id;                // Indica se o jogador é o 1 ou 2
    char tab[L][C];        // Tabuleiro do jogador
} Jogador;

#endif // PROTOCOL_H
