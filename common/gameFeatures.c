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
    Jogador *player = arg;

    int submarinos = 1;
    int fragatas = 2;
    int destroyers = 1;
    int posX, posY;

    printf("**Posicione os seus navios**\n");

    // Posicionando os  Submarinos (tamanho 1)

    for (int i = submarinos; i > -1; i--) {
        printf("Número de submarinos restantes: %d\n", i);
        if (strncmp(buffer, "POS", 3) == 0) {
            char tipo[20];
            int x, y;
            char orientacao;

            sscanf(buffer, "POS %s %d %d %c", tipo, &x, &y, &orientacao);

        // Solicita entrada, valida e posiciona
    }

    // Posicionando as Fragatas (tamanho 2)
    for (int i = fragatas; i > -1; i--) {
        printf("Número de fragatas restantes: %d\n", i);
        // Solicita início, direção e valida posições
    }

    // Posicionando os Destroyers (tamanho 3)
    for (int i = destroyers; i > -1; i--){
        printf("Número de destroyers restantes: %d\n", i);
        // Idem acima, adaptado pro tamanho
    }

    printf("Todos os navios foram posicionados!\n");
    print_tabuleiro(tab); // Exibe o tabuleiro final
}

void jogadas_cliente(char tab[L][C]){

}