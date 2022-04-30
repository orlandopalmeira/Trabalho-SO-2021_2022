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

int main(int argc, char const *argv[]){
    
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

    int fd;
    char* buffer = malloc(MAX);
    // DECIDIR FORMATO DE STRING A MANDAR PARA O SERVIDOR, PARA SER PROCESSADA NO SERVIDOR.
    sprintf(buffer, "testing!\n");

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
    
    free(buffer);
    unlink(ret_fifo);
    return 0;
}
