#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

// SERVIDOR

#define MAX 2048
#define MAIN_FIFO "../tmp/main_fifo"

int task_n = 1;

// Funcao auxiliar que lê uma linha de um ficheiro (le ate ao carater '\n').
ssize_t readln(int fd, char *line, size_t size){

	char c = ' ';
	int i = 0;
	int n = 1;
	while (i<size && n > 0 && c != '\n'){
		n = read (fd, &c, 1);
		if (n > 0 && c != '\n')
			line[i++] = c;
	}
	// acrescentar o carater final de fim de string
	if (i < size){
		line[i] = 0; // 0 = EOF
	}
	else{
		line[--i] = 0; // 0 = EOF
	}

	// se deu erro na leitura retorna esse mesmo erro
	if (n < 0) {
		return n;
    }
	// no caso de apanhar a linha só com o '\n'
	if (i == 0 && n == 1) {
		return (-1);
    }
	return i;
}

// STRUCTS 
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
    //int type;           // 1 se for do tipo status, 0 caso contrario        
    char * source_path;
    char * output_path;
    char ** transformations;    // array com as strings relativas à transformação
    int ntransformations;       // numero de elementos do array 'transformations'
    int mem;            // indica a memoria alocada no array 'transformations'
    char * ret_fifo;    // string que indica o pipe que devem ser enviadas mensagens de reply ao cliente
    pid_t pid;          // campo para auxiliar o libertamento do request.
    struct requests * next;

} *REQUEST;


void add_request(REQUEST * r, char * request) {

    int i;
    // Appends the request
    while (*r) {
        r = &(*r)->next;
    }
    // Cria um novo request para adicionar à fila.
    *r = malloc(sizeof(struct requests));
    (*r)->mem = 10;

    // Parses the request
    char *string, *found;
    string = strdup(request);
    strsep(&string, ";"); // descartar número inicial que indica o tipo do request.
    (*r)->source_path = strdup(strsep(&string, ";"));
    //printf("entry_path:%s\n", (*r)->source_path); // para verificar o entry path.
    (*r)->output_path = strdup(strsep(&string, ";"));
    (*r)->transformations = malloc((*r)->mem * sizeof(char*));
     
    for (i = 0; strcmp( (found = strsep(&string, ";")), "end") ; i++){
        if (i == (*r)->mem){
            (*r)->mem += 10;
            (*r)->transformations = realloc((*r)->transformations, (*r)->mem);
        }
        (*r)->transformations[i] = strdup(found);
    }
    (*r)->ntransformations = i;
    (*r)->ret_fifo = strsep(&string, ";");
    (*r)->task = task_n++;
    (*r)->next = NULL;
    free(string);
}


void free_request(REQUEST r) {

    free(r->source_path);
    free(r->output_path);
    for (int i = 0; i<r->ntransformations; i++)
        free(r->transformations[i]);
    free(r->transformations);
    free(r->ret_fifo);
    free(r);

}


TRANSF init_transf (char* name, char *path, int max){
    TRANSF t = malloc(sizeof(struct transf));
    t->name = strdup(name);
    t->path = strdup(path);
    t->running = 0;
    t->max = max;

    return t;
}

void freeTransf (TRANSF t){
    free(t->name);
    free(t->path);
    free(t);
}

// Inicializa uma struct TRANSFS
TRANSFS init_transfs (){
    TRANSFS t = malloc(sizeof(struct transfs));
    t->transformations = malloc(sizeof(TRANSF) * 7);
    t->atual = 0;
    t->max = 7; 

    return t;
}

// Liberta uma struct TRANSFS
void freeTransfs (TRANSFS t){
    for (int i=0; i<t->atual; i++)
        freeTransf(t->transformations[i]);
    free(t->transformations);
    free(t);
}

// Adiciona um filter atraves dos seus parametros, a um TRANSFS t.
void add_filter(TRANSFS t, char *name, int max, char * transformations_folder) {

    char *path = malloc(MAX);
    snprintf(path, MAX, "%s/%s", transformations_folder, name);

    t->transformations[(t->atual)] = init_transf(name, path, max);
    t->atual++;
    free(path);
}

// Dado o nome do ficheiro de configuração e o caminho dos executaveis, cria a struct TRANSFS com a informação relativa a cada transformaçao.
TRANSFS read_config_file(char * config_file, char * path_to_execs){

    int fd = open (config_file, O_RDONLY);
    if (fd == -1){
        perror("Erro ao abrir o config_file");
        exit(1);
    }

    char *string = malloc(MAX);
    char *found, *transf;
    char buffer[MAX];
    int bytes_read, i = 0;
    TRANSFS transformations = init_transfs();

    while((bytes_read = readln(fd, buffer, MAX)) > 0 && i < 7){
        string = strdup(buffer);
        if((found = strsep(&string, " ")) == NULL){
            printf("Formato de ficheiro de configuração errado!\n");
            exit(1);
        }

        // guarda em transf, a string correspondente à transformação.
        transf = found;
        
        if((found = strsep(&string, " ")) == NULL){
            printf("Formato de ficheiro de configuracao errado!\n");
            exit(1);
        }

        //printf("[DEBUG] -> %s \n%s\n", transf, found);
        
        add_filter(transformations, transf, atoi(found), path_to_execs);
        
        free(string);
        free(transf);
        i++;
    }

    free(config_file); // faço free dos argumentos pq sao strdup'ed no main.
    free(path_to_execs);
    return transformations;
}

char * return_status(REQUEST reqs, TRANSFS t) {

    int i;
    char * buffer = malloc(MAX);
    REQUEST r;

    // Adds information on pending tasks
    for (r = reqs; r ; r = r->next) {

        snprintf(buffer, MAX, "Task #%d: ", r->task);
        if (r->task == 0)
            snprintf(buffer + strlen(buffer), MAX, "proc-file ");
        else
            snprintf(buffer + strlen(buffer), MAX, "status ");
        snprintf(buffer + strlen(buffer), MAX, "%s %s ", r->source_path, r->output_path);
        for (i = 0; i<reqs->ntransformations; i++) {
            snprintf(buffer + strlen(buffer), MAX, "%s ", reqs->transformations[i]);
        }
        snprintf(buffer + strlen(buffer), MAX, "\n");
    }

    // Adds information on usage of the transformations
    for (i = 0; i < t -> atual; i++){
        snprintf(buffer + strlen(buffer), MAX, "transf %s: %d/%d (running/max)\n", t->transformations[i]->name, t->transformations[i]->running, t->transformations[i]->max);
    }

    return buffer;
}


int main(int argc, char const *argv[]){
    
    if (argc != 3){
        printf("Número de argumentos incorreto!\n");
        return -1;
    }

    TRANSFS t = read_config_file( strdup(argv[1]), strdup(argv[2]) );


    int mkR = mkfifo(MAIN_FIFO, 0666);
    if (mkR == -1){
        printf("Erro na criação do FIFO!\n");
        return -1;
    }

    int fd, bytes_read, flag = 1;
    int fd_fake;
    char *buffer = malloc(MAX);
    memset(buffer, 0, MAX);
    
    // Abrir o MAIN_FIFO
    fd = open(MAIN_FIFO, O_RDONLY);
    if (fd == -1){
        perror("Erro ao abrir o MAIN_FIFO");
        exit(1);
    }

    // FD aberto para enganar o read a nunca retornar EOF ate que este fd_fake seja fechado.
    fd_fake = open(MAIN_FIFO, O_WRONLY);
    if (fd_fake == -1){
        perror("Erro ao abrir o FAKE_FIFO");
        exit(1);
    }
    
   REQUEST reqs = NULL;

    while(flag){
        if( (bytes_read = read(fd, buffer, MAX)) > 0 ){
            if (strcmp(buffer, "TERMINATE") == 0){
                flag = 0;
                close(fd_fake);
            }
            else if( *buffer == '1' ){ // status
                // Codigo para status.
                int fd_reply;
                char *string = strdup(buffer);
                char *found;
                strsep(&string, ";");
                found = strsep(&string, ";");
                char* status = return_status(reqs, t);
                printf("%s\n", status);
                /*
                fd_reply = open(found, O_WRONLY);
                if (fd_reply == -1){
                    perror("Error opening fd_reply");
                    exit(1);
                }
                write(fd_reply, status, strlen(status));
                close(fd_reply);*/
                free(string);
                free(status);

            }
            else {

                printf("String enviada->%s", buffer);
                add_request(&reqs, buffer);

            }

            

            memset(buffer, 0, MAX); // para limpar o buffer.
            
        }


    
    
    }
    

    free(buffer);
    freeTransfs(t);
    unlink (MAIN_FIFO);
    return 0;
}

/*
// Estrutura de dados que serve como Queue de pedidos enviados por clientes.
typedef struct requests {
    
    int task;           // identificador do número da task.
    //int type;           // 1 se for do tipo status, 0 caso contrario        
    char * source_path;
    char * output_path;
    char ** transformations;    // array com as strings relativas à transformação
    int ntransformations;       // numero de elementos do array 'transformations'
    int mem;            // indica a memoria alocada no array 'transformations'
    char * ret_fifo;    // string que indica o pipe que devem ser enviadas mensagens de reply ao cliente
    pid_t pid;          // campo para auxiliar o libertamento do request.
    struct requests * next;

} *REQUEST;
*/