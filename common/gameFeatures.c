#include <gameFeatures.h>

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

void* posiciona_barcos(void *arg){
    char tab[L][C] = arg;
    
}

void jogadas_cliente(char tab[L][C]){

}