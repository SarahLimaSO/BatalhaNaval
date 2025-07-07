
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include "../common/protocol.h"

#define PORT 8080

int main() {
    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 2); // até dois jogadores

    printf("Servidor aguardando jogadores na porta %d...\n", PORT);

    printf("hey num 0\n");
    int player1 = accept(server_fd, (struct sockaddr*)&address, &addrlen);
    printf("hey num 1\n");
    int player2 = accept(server_fd, (struct sockaddr*)&address, &addrlen);
    printf("hey num 2\n");
    
    // if (player1 < 0) perror("Accept player 1 failed");
    // if (player2 < 0) perror("Accept player 2 failed");

    printf("Hello");
    send(player1, "Emparelhado! Você é o Jogador 1\n", 33, 0);
    send(player2, "Emparelhado! Você é o Jogador 2\n", 33, 0);

    close(server_fd);
    return 0;
}
