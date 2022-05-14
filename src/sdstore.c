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
#define MAIN_FIFO "tmp/main_fifo"


int main(int argc, char const *argv[]){
    
    if (argc == 1) { // Caso para explicar o que fazer ao utilizador.

        write(1, "Executável precisa de argumentos!\nInformação sobre o servidor: (executavél) status\nEnvio de processamento de ficheiro: (executavél) proc-file input-filename output-filename transformation-1 transformation-2 ...\nEnvio de processamento de ficheiro com prioridade: (executavél) proc-file -p <priority> input-filename output-filename transformation-1 transformation-2 ...\n", 375);
        return 0;
    }
    // para obter um nome para criar um pipe em que retorne a informacao vinda do servidor.
    int pid = getpid();
    char *ret_fifo = malloc(MAX);
    snprintf(ret_fifo, MAX, "tmp/p%d", pid);

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
        snprintf(buffer, MAX, "s;%s\n", ret_fifo);
    }
    // Envia mensagem "TERMINATE" para fechar o servidor
    else if( strcmp(argv[1], "TERMINATE") == 0 ){
        snprintf(buffer, 3, "t\n");
    }
    // Caso com prioridade definida.
    else if(strcmp(argv[1], "proc-file") == 0 && strcmp(argv[2], "-p") == 0 && atoi(argv[3]) >= 0 && atoi(argv[3]) <= 5){
        //printf("PASSOU PELA PARTE DE PRIOS\n");
        snprintf(buffer, MAX, "%s;%s;%s;", argv[3], argv[4], argv[5]);
        i = 6;

        // Loop para escrever as transformations separadas por ";"
        for ( ; i < argc; i++) {
            snprintf(buffer + strlen(buffer), MAX, "%s;", argv[i]);
            //if (i != argc - 1) snprintf(buffer + strlen(buffer), MAX, ";");
        }

        // To write the return pipe of the client
        snprintf(buffer + strlen(buffer), MAX, "end;%s\n", ret_fifo);
    }
    // Caso com o valor da prioridade fora dos limites.
    else if( strcmp(argv[1], "proc-file") == 0 && strcmp(argv[2], "-p") == 0 && (atoi(argv[3]) < 0 || atoi(argv[3]) > 5) ){
        write(1, "Prioridade definida tem de estar entre 0 e 5.\n", 47);
        free(buffer);
        unlink(ret_fifo);
        free(ret_fifo);
        return 0;
    }
    // Caso com prioridade default = 0.
    else if( strcmp(argv[1], "proc-file") == 0){
        snprintf(buffer, MAX, "0;%s;%s;", argv[2], argv[3]);
        i = 4;

        // Loop para escrever as transformations separadas por ";"
        for ( ; i < argc; i++) {
            snprintf(buffer + strlen(buffer), MAX, "%s;", argv[i]);
            //if (i != argc - 1) snprintf(buffer + strlen(buffer), MAX, ";");
        }

        // To write the return pipe of the client
        snprintf(buffer + strlen(buffer), MAX, "end;%s\n", ret_fifo);
    }
    
    else{ // caso em que os argumentos estao mal definidos.
        write(1, "Informação sobre o servidor: (executavél) status\nEnvio de processamento de ficheiro: (executavél) proc-file input-filename output-filename transformation-1 transformation-2 ...\nEnvio de processamento de ficheiro com prioridade: (executavél) proc-file -p <priority> input-filename output-filename transformation-1 transformation-2 ...\n", 340);
        free(buffer);
        unlink(ret_fifo);
        free(ret_fifo);
        return 0;
    }

    fd = open(MAIN_FIFO, O_WRONLY);
    write(fd, buffer, strlen(buffer));
    close(fd);

    // Para impedir que o cliente tente ler algo que venha do servidor que nunca virá, uma vez que é um TERMINATE.
    if ( strcmp(buffer, "t\n") == 0 ){
        free(buffer);
        unlink(ret_fifo);
        free(ret_fifo);
        return 0;
    }
    
    // abrir o ret_fifo para leitura do que vem do server 
    //printf ("A ABRIR O FD_REPLY PRA LEITURA\n");
    while( (fd = open(ret_fifo, O_RDONLY)) == -1);
    //printf ("ABRIU PRA LEITURA\n");
    if (fd == -1){
        perror("Error opening return_fifo");
        exit(1);
    }
    memset(buffer, 0, MAX); // talvez em vez de MAX possa usar strlen(buffer);
    
    // Escreve o que vier do servidor.
    int bytes_read, fd_fake = 0;
    while( (bytes_read = read(fd, buffer, MAX)) > 0){
        if(strcmp(buffer, "pending\n") == 0){
            fd_fake = open(ret_fifo, O_WRONLY); // para impedir que seja terminada precocemente a leitura do ret_fifo.
        }
        if(fd_fake != 0 && strncmp(buffer, "concluded",9) == 0){
            close(fd_fake);
        }
        write(1, buffer, bytes_read);
    }

    close(fd);

    
    free(buffer);
    unlink(ret_fifo);
    free(ret_fifo);
    return 0;
}
