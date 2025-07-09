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

int posiciona_navio(char tab[L][C], char tipo[20], int x, int y, char orientacao){
    
    //Falta tratar caso de sobreposicao !!!!!!
    
    if (strncmp(tipo, "SUBMARINO", len("SUBMARINO")) == 0) tab[x][y] = '*';
    
    if (strncmp(tipo, "FRAGATA", len("FRAGATA")) == 0){
        
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
    if (strncmp(tipo, "DESTROYER", len("DESTROYER")) == 0){
        
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
    Jogador* jogador = (struct Jogador*) arg;
    char buffer[1024];

    int max_barcos = MAX_DEST + MAX_FRAG + MAX_SUB;
    int i = 0; //Contador
    
    // Posicionando os barcos
    while (i <= max_barcos) {
        recv(jogador->socket, buffer, sizeof(buffer), 0);

        if (strncmp(buffer, "POS", 3) == 0) {
            char tipo[20];
            int x, y;
            char orientacao;
            sscanf(buffer, "POS %s %d %d %c", tipo, &x, &y, &orientacao);

            int resultado = posiciona_navio(jogador->tab, tipo, x, y, orientacao);

            if (resultado == 1) {
                send(jogador->socket, "**Navio posicionado**\n", strlen("**Navio posicionado**\n"), 0);
                i++;
            } else {
                send(jogador->socket, "!!Erro ao posicionar navio!!\n", strlen("!!Erro ao posicionar navio!!\n"), 0);
            }
        }
    }

    printf("Todos os navios foram posicionados!\n");
    print_tabuleiro(jogador->tab); // Exibe o tabuleiro final

    pthread_exit(NULL);

}