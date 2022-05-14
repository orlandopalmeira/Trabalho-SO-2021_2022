#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#ifndef SDTORED_H
#define SDTORED_H

// Structs com informaçao sobre as transformaçoes. 
typedef struct transf{
    char* name;
    char* path;
    int running;
    int max;
} *TRANSF; 

typedef struct transfs{
    TRANSF* transformations;
    int atual;
    int max;
} *TRANSFS;

// Estrutura de dados que serve como Queue de pedidos enviados por clientes.
typedef struct requests {
    
    int task;           // identificador do número da task.
    int prio;           // prioridade do pedido.      
    char * source_path;
    char * output_path;
    char ** transformations;    // array com as strings relativas à transformação
    int n_transformations;       // numero de elementos do array 'transformations'
    int mem;            // indica a memoria alocada no array 'transformations'
    char * ret_fifo;    // string que indica o pipe que devem ser enviadas mensagens de reply ao cliente
    pid_t pid;          // 0 (default value) se ainda nem foi verificado para processar, -1 se já foi verificado para processar, >0 se está em processamento com o numero do pid que o está a processar.
    struct requests * next;

} *REQUEST;

#endif