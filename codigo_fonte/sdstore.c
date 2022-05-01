#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

// CLIENTE

#define MAX 1024
#define MAIN_FIFO "../tmp/main_fifo"

char* build_request(char* argv[], int argc, char* pid) {

    int i;
    char* buffer = malloc(MAX);
    // Type of instruction, input, output,
    snprintf(buffer, MAX, "1,%s,%s,", argv[0], argv[1]);

    // Loop to write the filters separated by ";"
    for (i = 2; i < argc; i++) {
        snprintf(buffer + strlen(buffer), MAX, "%s", argv[i]);
        if (i != argc - 1) snprintf(buffer + strlen(buffer), MAX, " ");
    }

    // Loop to write the pid of the client
    snprintf(buffer + strlen(buffer), MAX, ",%s", pid);

    return buffer;
}


int main(int argc, char const *argv[]){
    
    if (argc == 1) { // Caso para explicar o que fazer ao utilizador

        char * buffer = malloc(MAX);
        sprintf(buffer, "./sdstore status\n./sdstore option input-filename output-filename filter-id-1 filter-id-2 ...\n");
        write(1, buffer, strlen(buffer));
        free(buffer);
        exit(1);
    }
    // para obter um nome para criar um pipe em que retorne a informacao vinda do servidor.
    int pid = getpid();
    char *ret_fifo = malloc(MAX);
    snprintf(ret_fifo, MAX, "../tmp/fifo%d", pid);

    // Criar o Pipe de retorno de informaçao vindo do server.
    int mkr = mkfifo(ret_fifo, 0644);
    if (mkr == -1){
        perror("Error creating ret_fifo");
        exit(1);
    }

    int fd, i;
    
    char* buffer = malloc(MAX); memset(buffer, 0, MAX);
    // Envia mensagem "status" caso queira saber o status do servidor
    if ( strcmp(argv[1], "status") == 0 ){
        snprintf(buffer, MAX, "status,%s", ret_fifo);
    }
    // Envia mensagem "TERMINATE" para fechar o servidor
    else if( strcmp(argv[1], "TERMINATE") == 0 ){
        snprintf(buffer, 10, "TERMINATE");
    }
    else{
        // Type of instruction, input, output,
        if( strcmp(argv[1], "proc-file") != 0 ){
            snprintf(buffer, MAX, "0,%s,%s,", argv[1], argv[2]);
            i = 3;
        }
        else{
            snprintf(buffer, MAX, "1,%s,%s,", argv[2], argv[3]);
            i = 4;
        }    

        // Loop to write the filters separated by ";"
        for ( ; i < argc; i++) {
            snprintf(buffer + strlen(buffer), MAX, "%s", argv[i]);
            if (i != argc - 1) snprintf(buffer + strlen(buffer), MAX, ";");
        }

        // Loop to write the pid of the client
        snprintf(buffer + strlen(buffer), MAX, ",%s", ret_fifo);
    }
    

    fd = open(MAIN_FIFO, O_WRONLY);
    write(fd, buffer, strlen(buffer));
    close(fd);

/*
    // abrir o ret_fifo para leitura do que vem do server (TO COMPLETE)
    int bytes_read;
    fd = open(ret_fifo, O_RDONLY);
    if (fd == -1){
        perror("Error opening return_fifo");
        exit(1);
    }
    bytes_read = read(fd, buffer, MAX);
    // DECIDIR O QUE FAZER COM O QUE VEM DO SERVER, O QUE ESTA NO BUFFER.
*/
    
    //free(buffer);
    unlink(ret_fifo);
    free(ret_fifo);
    return 0;
}
