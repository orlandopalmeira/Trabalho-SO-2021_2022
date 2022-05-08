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
    int prio;           // prioridade do pedido.      
    char * source_path;
    char * output_path;
    char ** transformations;    // array com as strings relativas à transformação
    int n_transformations;       // numero de elementos do array 'transformations'
    int mem;            // indica a memoria alocada no array 'transformations'
    char * ret_fifo;    // string que indica o pipe que devem ser enviadas mensagens de reply ao cliente
    pid_t pid;          // 0 se ainda nem foi verificado para processar, -1 se ja foi verificado para processar, >0 se esta em processamento com o numero do pid que o esta a processar.
    struct requests * next;

} *REQUEST;

// Adiciona ao fim da lista ligada r, um request codificado pela string request.
void add_request(REQUEST * r, char * request) {

    char *string, *found;
    string = strdup(request);
    int i;

    REQUEST new = malloc(sizeof(struct requests));
    new->mem = 10;
    new->pid = 0;

    // Parses the request
    new->prio = atoi(strsep(&string, ";"));
    new->source_path = strdup(strsep(&string, ";")); //printf("entry_path:%s\n", new->source_path); // para verificar o entry path.
    new->output_path = strdup(strsep(&string, ";"));
    new->transformations = malloc(new->mem * sizeof(char*));
     
    for (i = 0; strcmp( (found = strsep(&string, ";")), "end") ; i++){
        if (i == new->mem){
            new->mem += 10;
            new->transformations = realloc(new->transformations, new->mem * sizeof(char*));
        }
        new->transformations[i] = strdup(found);
    }
    new->n_transformations = i;
    new->ret_fifo = strdup( strsep(&string, ";\n") );
    new->task = task_n++;
    //new->next = NULL;

    free(string); // estava em comentario.

    REQUEST ant = (*r);
    // Appends the request
    while ( (*r) && (*r)->prio > new->prio ) {
        ant = (*r);
        r = &(*r)->next;
    }

    if ((*r) == NULL){
        (*r) = new;
        new->next = NULL;
    }
    else if ((*r) == ant){
        new->next = (*r);
        (*r) = new;
    }
    else{
        new->next = (*r);
        ant->next = new;
    }
}

/*
// FUNCAO INACABADA DE ADICIONAR UM NODO DE UM REQUEST A UM APONTADOR DE REQUEST
void add(REQUEST * r, REQUEST rp){
    int i;
    // Appends the request
    while (*r) {
        r = &(*r)->next;
    }
}
*/

void free_request(REQUEST r) {

    free(r->source_path);
    free(r->output_path);
    for (int i = 0; i<r->n_transformations; i++)
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


// Adiciona uma transformaçao atraves dos seus parametros, a um TRANSFS t.
void add_transf(TRANSFS t, char *name, int max, char * transformations_folder) {

    char *path = malloc(MAX);
    snprintf(path, MAX, "%s/%s", transformations_folder, name);

    t->transformations[(t->atual)] = init_transf(name, path, max);
    t->atual++;
    free(path);
}

// Retorna 1 se for valido, 0 caso contrario
int verify(TRANSFS tr, REQUEST req) {

    int i, j, ret = 1;
    for (i = 0; i < req->n_transformations; i++){
        for (j = 0; j < tr->atual && ret; j++){
            if (strcmp(tr->transformations[j]->name, req->transformations[i]) == 0 && tr->transformations[j]->running == tr->transformations[j]->max ){
                ret = 0;
                break;
            }
        }
    } 
    return ret;
}

// Verifica se as transformações estão no seu uso máximo.
int isTableFull(TRANSFS tr){
    int i, r = 1;
    for (i=0; i<tr->atual && r == 1; i++){
        if (tr->transformations[i]->running < tr->transformations[i]->max)
            r = 0;
    }
    return r;
}

// Altera a tabela transfs atualizando o valor de utilizaçao das transformaçoes(aumenta se flg=1, diminui caso contrario)
void alter_usage(TRANSFS tr, REQUEST req, int flg) {

    int i, j;
    for (i = 0; i < req->n_transformations; i++){
        for (j = 0; j < tr->atual; j++){ 
            if ( strcmp(tr->transformations[j]->name, req->transformations[i]) == 0 ){
                if (flg == 1) {tr->transformations[j]->running++;}
                else {tr->transformations[j]->running--;}
                break;
            }
        }
    }
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
        
        add_transf(transformations, transf, atoi(found), path_to_execs);
        
        free(string);
        free(transf);
        i++;
    }

    free(config_file); // faço free dos argumentos pq sao strdup'ed no main.
    free(path_to_execs);
    return transformations;
}

// Retorna a string a mostrar ao cliente sobre o estado do servidor.
char * return_status(REQUEST reqs, TRANSFS t) {

    int i;
    char * buffer = malloc(MAX);
    REQUEST r;

    // Adds information on pending tasks
    for (r = reqs; r ; r = r->next) {

        snprintf(buffer, MAX, "Task #%d: proc-file %d ", r->task, r->prio);
        //snprintf(buffer + strlen(buffer), MAX, "proc-file ");
        
        snprintf(buffer + strlen(buffer), MAX, "%s %s ", r->source_path, r->output_path);
        for (i = 0; i<r->n_transformations; i++) {
            snprintf(buffer + strlen(buffer), MAX, "%s ", r->transformations[i]);
        }
        snprintf(buffer + strlen(buffer), MAX, "\n");
    }

    // Adds information on usage of the transformations
    for (i = 0; i < t -> atual; i++){
        snprintf(buffer + strlen(buffer), MAX, "transf %s: %d/%d (running/max)\n", t->transformations[i]->name, t->transformations[i]->running, t->transformations[i]->max);
    }

    return buffer;
}

// vai as transfs buscar a path do executável de uma certa tranformação
char *path_executable_tranformation(char *transf_name,TRANSFS trfs){
    char *pet = NULL;
    int i;
    for(i = 0; i < 7 && strcmp(trfs->transformations[i]->name,transf_name); i++);
    if(i >= 0 && i < 7) pet = trfs->transformations[i]->path;
    return pet;
}

// executa uma transformação do request
int exec_tranformation(char *transf, TRANSFS t){
    char *tr_exec_path = path_executable_tranformation(transf,t);
    execl(tr_exec_path,tr_exec_path,NULL);
    _exit(-1);
    return -1;
}

// executa um request
int exec_request(TRANSFS t, int n_trnsfs, char *transfs[], char *source_path, char *output_path){
    if(n_trnsfs == 1){
        pid_t pid = fork();
        if(pid == -1){
            perror("ERRO no fork exec_request"); return -1;
        }
        else if(pid == 0){ // processo filho encarregue de fazer a trasnformação
            int fd_source, fd_output;
            if((fd_source = open(source_path,O_RDONLY)) < 0){
                perror("ERRO ao abrir fd de entrada");
                return -1;
            }
            if ( (fd_output = open(output_path, O_CREAT | O_TRUNC | O_WRONLY, 0644) ) < 0){
                printf ("outpath -> %s\n", output_path);
                perror("ERRO ao abrir fd de saida");
                return -1;
            }
            
            dup2(fd_source,STDIN_FILENO);
            dup2(fd_output, STDOUT_FILENO);
            exec_tranformation(transfs[0],t);
        }
        wait(NULL);
    } else if(n_trnsfs > 1){
        int pipes[n_trnsfs-1][2]; //{read,write}
        pid_t pid;  
        int fd_source, fd_output;
        if((fd_source = open(source_path,O_RDONLY)) < 0){
            perror("ERRO ao abrir fd de entrada");
            return -1;
        }
        if ( (fd_output = open(output_path, O_CREAT | O_TRUNC | O_WRONLY, 0644) ) < 0){
            printf ("outpath -> %s\n", output_path);
            perror("ERRO ao abrir fd de saida");
            return -1;
        }

        for(int c = 0; c < n_trnsfs; c++){
            if(c >= 0 && c < n_trnsfs-1){
                if(pipe(pipes[c]) == -1) return -1;
            }

            if(c == 0){ // primeira tranformação
                pid = fork();
                if(pid == -1) return -1;
                else if(pid == 0){ // processo filho 
                    close(pipes[c][0]);
                    dup2(pipes[c][1],STDOUT_FILENO);
                    close(pipes[c][1]);
                    dup2(fd_source,STDIN_FILENO); // -> penso que seja assim
                    exec_tranformation(transfs[c],t);
                    _exit(-1);
                }
                close(pipes[c][1]);
            }
            else if(c == n_trnsfs-1){ // última transformação
                pid = fork();
                if(pid == -1) return -1;
                else if(pid == 0){ // processo filho
                    close(pipes[c-1][1]);
                    dup2(pipes[c-1][0],STDIN_FILENO);
                    close(pipes[c-1][0]);
                    dup2(fd_output,STDOUT_FILENO); //-> penso que seja assim
                    exec_tranformation(transfs[c],t);
                    _exit(-1);
                }
                close(pipes[c-1][0]);
            }
            else{ // tranformações intermédias
                pid = fork();
                if(pid == -1) return -1;
                else if(pid == 0){ // processo filho
                    dup2(pipes[c-1][0],STDIN_FILENO);
                    close(pipes[c-1][0]);
                    dup2(pipes[c][1],STDOUT_FILENO);
                    close(pipes[c][1]);
                    exec_tranformation(transfs[c],t);
                    _exit(-1);
                }
                close(pipes[c-1][0]);
                close(pipes[c][1]);
            }
        }
        for(int i = 0; i < n_trnsfs; i++) wait(NULL);
        close(fd_source);
        close(fd_output);
    }
    return 0;
}

void free_concluded_request(REQUEST *reqs, TRANSFS t){
    REQUEST req = *reqs, ant = req;
    int r_wpid;
    while(req){
        // caso em que o pedido ainda não começou a ser processado.
        if(req->pid == 0){
            ant = req;
            req = req->next;
        }
        // caso em que o pedido começou a processar mas ainda não acabou.
        else if ( (r_wpid = waitpid(req->pid, NULL, WNOHANG) ) != req->pid){
            //printf("[DEBUG] WAITPID RETORNOU [%d] no caso em que devia retornar 0\n", r_wpid);
            ant = req;
            req = req->next;
            //printf("TASK: %d\n", reqs->task);
        }
        // caso em que o pedido ja foi atendido.
        else {
            
            alter_usage(t, req, 0);
            REQUEST temp = req;
            if (ant == req){
                ant = ant->next;
                (*reqs) = (*reqs)->next;
            }                        
            else
                ant->next = req->next;
            
            req = req->next;
            free_request(temp);
        }
    }
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

    int fd, fd_reply, bytes_read, flag = 1;
    pid_t pid_pr;
    char *buffer = malloc(MAX);
    memset(buffer, 0, MAX);
    
    // Abrir o MAIN_FIFO
    fd = open(MAIN_FIFO, O_RDONLY);
    if (fd == -1){
        perror("Erro ao abrir o MAIN_FIFO");
        exit(1);
    }

    
    // FD aberto para enganar o read a nunca retornar EOF ate que este fd_fake seja fechado.
    int fd_fake = open(MAIN_FIFO, O_WRONLY);
    if (fd_fake == -1){
        perror("Erro ao abrir o FAKE_FIFO");
        exit(1);
    }
    if (fork() == 0){
        int c = 1;
        while(c){
            sleep(10);
            c = write(fd_fake, "n\n", 3); // para ir acordando o read.
        }
        _exit(0);
    }
    
    
    REQUEST reqs = NULL, req = NULL;
    while(flag || reqs){
        
        if( flag && (bytes_read = read(fd, buffer, MAX)) > 0 ){

            // Libertacao de pedidos ja atendidos.
            free_concluded_request(&reqs,t);

            // Separação de possiveis varios comandos dentro duma mensagem, previamente delimitados por '\n'.
            char *message, *command;
            message = strdup(buffer);
            while( strlen(command = strsep(&message, "\n")) > 0){

                if (*command == 'n'); // comando de acordar o read.
                else if (*command == 't'){
                    flag = 0;
                    close(fd_fake);
                }
                else if ( *command == 's' ){ // status

                    char *string = strdup(command);
                    char *found, *stats = NULL;
                    strsep(&string, ";");   // descarta o '1' que é indicativo de ser uma instruçao status.
                    found = strsep(&string, ";");
                    stats = return_status(reqs, t);
                    //printf("%s\n", stats);
                    //printf ("A ABRIR O FD_REPLY PRA ESCRITA\n");
                    fd_reply = open(found, O_WRONLY);
                    if (fd_reply == -1){
                        perror("Error opening fd_reply");
                        exit(1);
                    }
                    write(fd_reply, stats, strlen(stats));
                    close(fd_reply);
                    free(string);
                    free(stats);

                }
                else { // *command == '0'

                    add_request(&reqs, command);

                }
            }
            free(message);            
            //printf("String recebida -> %sCom tamanho: %ld\n", buffer, strlen(buffer));
            memset(buffer, 0, strlen(buffer));
        }
        else{
            // Libertacao de pedidos ja atendidos.
            free_concluded_request(&reqs,t);
        }
            
            // Atendimento dos requests
            req = reqs; // para percorrer a lista sem alterar o apontador de reqs
            while (req){
                
                while ( (fd_reply = open(req->ret_fifo, O_WRONLY)) == -1 );
                
                if( (req->pid == 0 || req->pid == -1) && verify(t, req) ){ // verifica se ja foi atendido o pedido e se é valido tendo em conta as transfs maximas.
                    alter_usage(t, req, 1);
                    if ( (pid_pr = fork()) == 0){
                        write(fd_reply, "processing\n", 12);
                        //sleep(90000); // SLEEP TEST FOR QUEUE.
                        // processar o pedido.
                        char *transfs[req->n_transformations]; // separar as tranformações
                        for(int i = 0; i < req->n_transformations; i++){
                            transfs[i] = req->transformations[i];
                        }                    
                        
                        if(exec_request(t,req->n_transformations,transfs,req->source_path,req->output_path) < 0){
                            // ????FAZER ALGUMA CENA CASO O exec_request DER ERRO??????????
                            printf("Nao foi possivel executar o request\n");
                        }

                        //printf("\n[DEBUG] Acabou de processar o request %d!!!\n\n", req->task);
                        //write(fd_reply, "concluded\n", 11);
                        int fd_in,fd_out;
                        if((fd_in = open(req->source_path,O_RDONLY)) < 0){
                            perror("ERRO a abrir o fd_in"); return -1;
                        }
                        if((fd_out = open(req->output_path,O_RDONLY)) < 0){
                            perror("ERRO a abrir o fd_in"); return -1;
                        }
                        snprintf(buffer,MAX,"concluded (bytes-input: %ld, bytes-output: %ld)\n",lseek(fd_in,0,SEEK_END),lseek(fd_out,0,SEEK_END));
                        close(fd_in); close(fd_out);
                        write(fd_reply,buffer,strlen(buffer));
                        _exit(0);
                    }
                    close(fd_reply);
                    req->pid = pid_pr;
                }
                else if (req->pid == 0){
                    write(fd_reply, "pending\n", 9);
                    req->pid = -1;
                    close(fd_reply);
                }
                else{ // else if(req->pid == -1)
                    close(fd_reply);
                }
                req = req->next;

            }

    }
    
    free(buffer);
    freeTransfs(t);
    unlink (MAIN_FIFO);
    return 0;
}
