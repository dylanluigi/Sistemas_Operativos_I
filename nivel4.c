/*
Entrega 2
Dylan Luigi Canning García
Juan Marí González
Antonio Marí González
Descrición:
1. Implementación Mini-Shell
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

#define COMMAND_LINE_SIZE 1024
#define ARGS_SIZE 64
#define N_JOBS 64

/*
 * Declaraciones de funciones
 */
int check_internal(char **args);
int parse_args(char **args, char *line);
int execute_line(char *line);
char *read_line(char *line);
void imprimir_prompt();
int internal_cd(char **args);
int internal_export(char **args);
int internal_source(char **args);
int internal_jobs();
void reaper(int signum);
void ctrlc(int signum);

/*
 * Declaraciones de variables
 */
static char mi_shell[COMMAND_LINE_SIZE]; 
int DEBUG_FLAGS[]={0,0,0,1};

/*
 * Struct:  info_job
 * -------------------
 * 
 */
struct info_job {
   pid_t pid;
   char estado; // ‘N’, ’E’, ‘D’, ‘F’ (‘N’: Ninguno, ‘E’: Ejecutándose y ‘D’: Detenido, ‘F’: Finalizado) 
   char cmd[COMMAND_LINE_SIZE]; // línea de comando asociada
};


struct info_job jobs_list [N_JOBS]; 

/*
 * Función:  imprimir_prompt
 * -------------------
 * Imprime el directorio actual + '$'
 */
void imprimir_prompt() {
    char cwd[COMMAND_LINE_SIZE];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s $ ", cwd); // Print del dir. actual + $.
    } else {
        perror("getcwd() error");
    }
}

/*
 * Función:  read_line
 * -------------------
 * 
 *
 * dest:
 * src:
 *
 * retorna:
 */
char *read_line(char *line) {
    imprimir_prompt();
    fflush(stdout); // Vaciamos el buffer de salida.
    if (fgets(line, COMMAND_LINE_SIZE, stdin) == NULL) {
        if (feof(stdin)) { // Comprueba si es EOF aka (Ctrl+D).
            printf("\nLogout\n");
            exit(0);
        } else {
            perror("fgets() error");
            exit(1);
        }
    }
    size_t length = strlen(line);
    if (line[length - 1] == '\n') {
        line[length - 1] = '\0'; // Sustituye el carácter de nueva línea por un carácter nulo.
    }
    return line;
}

/*
 * Función:  
 * -------------------
 * 
 * Añadiremos en el proceso hijo las llamadas a dos señales SIGCHLD y SIGINT. 
 * La primera se ejecutará cuando un proceso hijo haya finalizado con el metodo reaper()
 * y la segunda cuando presionemos el Control ^C.Más tarde en el proceso padre pondremos un
 * pause, cuando un proceso hijo se este ejecutando en primer plano para esperar a que llegue
 * la señal.
 * 
 *
 *
 * retorna:
 */
int execute_line(char *line) {
    pid_t pid;
    char *auxiliar = line;
    char *args[ARGS_SIZE];
    parse_args(args, line);
    int valorcheck=check_internal(args);
    if (valorcheck == 1||valorcheck==-1) {
        return 0; 
    }

    pid = fork();
    if (pid == 0) {
        
        signal(SIGCHLD, SIG_DFL);
        signal(SIGINT, SIG_IGN);
        execvp(args[0], args);
        printf("%s: no se encontro la orden.\n", args[0]);
        exit(-1);
    } else if (pid > 0) {
        
        jobs_list[0].estado = 'E';
        strncpy(jobs_list[0].cmd, auxiliar, COMMAND_LINE_SIZE - 1);
        jobs_list[0].cmd[COMMAND_LINE_SIZE - 1] = '\0';
        jobs_list[0].pid = pid;

        if(jobs_list[0].pid>0){
         pause(); 
        } 

        jobs_list[0].estado = 'F';
        jobs_list[0].pid = 0;
        memset(jobs_list[0].cmd, 0, sizeof(jobs_list[0].cmd));
    } else {
        perror("fork");
        exit(-1);
    }

    return 0;
}



/*
 * Función:  
 * -------------------
 * 
 *
 * dest:
 * src:
 *
 * retorna:
 */
int parse_args(char **args, char *line) {
    int num_tokens = 0;
    char *token = strtok(line,  " \t\n\r");
    while (token != NULL && num_tokens < ARGS_SIZE - 1) {
        if (token[0] == '#') { 
            break;
        }
       
        args[num_tokens++] = token;
        token = strtok(NULL,  " \t\n\r");
        if (DEBUG_FLAGS[0]==1){
  printf("parse_args()->El token número: %d es: %s \n",num_tokens,args[num_tokens-1]);
        }
    }
      if (DEBUG_FLAGS[0]==1){
    printf("parse_args()-> el número de tokens es: %d  \n",num_tokens);
      }
    args[num_tokens] = NULL; 
    
    return num_tokens;
}

/*
 * Función:  
 * -------------------
 * 
 *
 * dest:
 * src:
 *
 * retorna:
 */
int check_internal(char **args) {
    if(strcmp(args[0],"exit")==0){
        exit(-1);
        return 1;
    }else{
        if (strcmp(args[0], "cd") == 0) {
        return internal_cd(args);
    } else if (strcmp(args[0], "export") == 0) {
        return internal_export(args);
    } else if (strcmp(args[0], "source") == 0) {
        return internal_source(args);
    } else if (strcmp(args[0], "jobs") == 0) {
        return internal_jobs();
    }
    return 2;
    }
}


/*
 * Función:  
 * -------------------
 * 
 *
 * dest:
 * src:
 *
 * retorna:
 */
int internal_cd(char **args) {
    char cwd[COMMAND_LINE_SIZE];
    char dir[COMMAND_LINE_SIZE] = {0}; // Inicializo array a zero

    // Caso 1 (Sin argumentos): Cambia al directorio HOME
    if (args[1] == NULL) {
        strcpy(dir, getenv("HOME"));
        if (dir[0] == '\0') {
            fprintf(stderr, "Error: No se pudo obtener el HOME\n");
            return -1;
        }
    } 
    // Caso 2 (Un argumento): Cambia al directorio especificado
    else if (args[2] == NULL) {
        strcpy(dir, args[1]);
    } 
    // Caso 3 (Más de un argumento): Manejar cd avanzado
    else {
        //Concatenamos los argumentos para formar el nombre completo del directorio
        for (int i = 1; args[i] != NULL; i++) {
            if (i > 1) {
                strcat(dir, " "); // Añadimos espacios entre argumentos
            }
            strcat(dir, args[i]); // Añade el argumento actual
        }
        // Manejo comillas y escapes aquí
        if (dir[0] == '\"' || dir[0] == '\'') {
            size_t len = strlen(dir);
            if (dir[len - 1] == dir[0]) {
                dir[len - 1] = '\0';
                memmove(dir, dir + 1, len - 1);
            }
        }
    }
    

    // Cambio de directorio
    if (chdir(dir) != 0) {
        perror("chdir() error");
        return -1;
    }

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        if (DEBUG_FLAGS[1]==1)
        {
            printf("[internal_cd() → PWD: %s]\n", cwd);
        }
        

    } else {
        perror("getcwd() error");
        return -1;
    }

    

    return 1;
}



/*
 * Función:  
 * -------------------
 * 
 *
 * dest:
 * src:
 *
 * retorna:
 */
int internal_export(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Error de sintaxis. Uso: export Nombre=Valor\n");
        return -1;
    }

    char *name = args[1];
    char *value = strchr(args[1], '=');

    if (value == NULL || value == name) {
        fprintf(stderr, "Error de sintaxis. Uso: export Nombre=Valor\n");
        return -1;
    }

    // Dividir el nombre y el valor
    *value = '\0';  // Divide la cadena en el signo igual
    value++;        // Avanza al valor real
    char *old_value = getenv(name);

    if (DEBUG_FLAGS[1]==1)
    {
        printf("[internal_export()→ nombre: %s]\n",name);
        printf("[internal_export()→ valor: %s]\n",old_value);
    }
    
    // Asigna el nuevo valor
    if (setenv(name, value, 1) != 0) {
        perror("setenv() error");
        return -1;
    }

    // Muestra el nuevo valor
    char *new_value = getenv(name);
    if (DEBUG_FLAGS[1]==1)
    {
        printf("[internal_export()→ antiguo valor para %s: %s]\n", name, old_value ? old_value : "(null)");
        printf("[internal_export()→ nuevo valor para %s: %s]\n", name, new_value ? new_value : "(null)");
    }
    
    
    

    return 1;
}


/*
 * Función:  
 * -------------------
 * 
 *
 * dest:
 * src:
 *
 * retorna:
 */
int internal_source(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Error de sintaxis. Uso: source <nombre_fichero>\n");
        return -1;
    }

    FILE *file = fopen(args[1], "r");
    if (file == NULL) {
        perror("fopen");
        return -1;
    }

    char line[COMMAND_LINE_SIZE];
    while (fgets(line, COMMAND_LINE_SIZE, file) != NULL) {
        size_t length = strlen(line);
        if (line[length - 1] == '\n') {
            line[length - 1] = '\0';
        }

        fflush(file); // Flush file stream before executing the line
        if (DEBUG_FLAGS[2]==1)
        {
            printf("[internal_source()→ LINE: %s]\n",line);
        }
        
        

        // Execute each line using execute_line function
        execute_line(line);
    }

    fclose(file);
    return 1;
}


/*
 * Función:  
 * -------------------
 * 
 *
 * dest:
 * src:
 *
 * retorna:
 */
int internal_jobs() {
    printf("Listar todos los trabajos en segundo plano.\n");
    return 1;
}

/*
 * Función:  
 * -------------------
 * 
 *
 * dest:
 * src:
 *
 * retorna:
 */
int internal_fg(char **args) {
    printf("Moviendo el trabajo '%s' al primer plano...\n", args[1]);
    
    return 1;
}

/*
 * Función:  
 * -------------------
 * 
 *
 * dest:
 * src:
 *
 * retorna:
 */
int internal_bg(char **args) {
    printf("Continuando con el trabajo '%s' en segundo plano...\n", args[1]);
    
    return 1;
}


/*
 * Función: Repaer()
 * -------------------
 * Función que se ejecutará cuando un proceso hijo haya terminado si se ejecuta en primer plano
 * visulizamos el pid, los comandos y la señal y finalizamos el proceso
 * 
 */
void reaper(int signum) {
    signal(SIGCHLD, reaper);
    int status=0;
    pid_t ended;
    
        
    while(-1, &status, WNOHANG) > 0) {
        if(ended==jobs_list[0].pid){
            if(DEBUG_FLAGS[3]==1){
                printf("[Proceso hijo %d (%s) finalizado por señal %d]",ended,jobs_list[0].cmd,status);
            }
            jobs_list[0].pid=0;
            jobs_list[0].estado='F';
            for(int i=0;i<COMMAND_LINE_SIZE;i++){
                jobs_list[0].cmd[i]='\0';
            }
        
    
        }
    }
}

/*
 * Función:ctrlc()  
 * -------------------
 * Funcion que implementa el control c, este comprueba si
 * estamos en un proceso foreground si es así lo matamos y
 * restablecemos los valores del pid y del estado, si no no hacemos
 * nada.
 * 
 *
 */
void ctrlc(int signum) {
    signal(SIGINT, ctrlc);
    if(DEBUG_FLAGS[3]==1){
            printf("\n[Soy el proceso con PID %d (%s), el proceso en foreground es %d (%s)]\n",getpid(),mi_shell,jobs_list[0].pid,jobs_list[0].cmd);
    }
    if (jobs_list[0].pid > 0) {
        if(strcmp(mi_shell,jobs_list[0].cmd)!=0){
            if(DEBUG_FLAGS[3]==1){
                printf("[Señal %d enviada a %d (%s) por %d (%s)]\n",signum,jobs_list[0].pid,jobs_list[0].cmd,getpid(),mi_shell);

            }
            kill(jobs_list[0].pid, SIGTERM);  // Terminate the foreground process
            jobs_list[0].pid = 0;             // Reset the foreground process PID
            jobs_list[0].estado = 'N';        // Reset the state
        }else{
            if(DEBUG_FLAGS[3]==1){
                printf("[Señal SIGTERM no enviada debido a que el proceso en foreground es el shell]\n");

            }
        }
    } else {
        if(DEBUG_FLAGS[3]==1){
            printf("[Señal %d no enviada a %d (%s) debido a que no hay proceso en foreground]\n",signum,getpid(),mi_shell);

        }
        
    }
        printf("\n");                
        fflush(stdout);
}


/*
 * Función:  
 * -------------------
 * Añadimos las llamadas de las señales respectivas a cada metodo.
 *
 * dest:
 * src:
 *
 * retorna:
 */


int main(int argc, char *argv[]) {
    strcpy(mi_shell, argv[0]);

    // Initialization of jobs_list[0]
    jobs_list[0].pid = 0;
    jobs_list[0].estado = 'N';
    memset(jobs_list[0].cmd, 0, sizeof(jobs_list[0].cmd));

    // Set signal handlers
    signal(SIGCHLD, reaper);
    signal(SIGINT, ctrlc);

    char line[COMMAND_LINE_SIZE];
    while (1) {
        if (read_line(line)) {
            execute_line(line);
        }
    }
    return 0;
}
