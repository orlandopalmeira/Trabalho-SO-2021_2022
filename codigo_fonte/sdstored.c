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

#define MAX 1024
#define MAIN_FIFO "../tmp/main_fifo"


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
void add_filter(TRANSFS t, char *name, int max, char * filters_folder) {

    char *path = malloc(MAX);
    snprintf(path, MAX, "%s/%s", filters_folder, name);

    t->transformations[(t->atual)] = init_transf(name, path, max);
    t->atual++;
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

    int fd, fd_fake, bytes_read;
    char *buffer = malloc(MAX);

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

    while( (bytes_read = read(fd, buffer, MAX)) > 0){
        if (strcmp(buffer, "TERMINATE") == 0){
            close(fd_fake);
        }
        else {
            //write(1, buffer, bytes_read);
            printf("%s\n", buffer);
        }
        memset(buffer, 0, MAX); // para limpar o buffer.
    }



    free(buffer);
    freeTransfs(t);
    unlink (MAIN_FIFO);
    return 0;
}
