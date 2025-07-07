
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

char info[1024];

// void recebe_comando(const char* cmd){
//     if (strncmp(cmd, CMD_JOIN, 4) == 0) {
//         sscanf(cmd, "JOIN %s", info); //Guarda a informacao do jogador
        
//     }
//     else if(strncmp(cmd, CMD_READY, 5) == 0){

//     }
// }
void responde_comando(){

}
#endif
