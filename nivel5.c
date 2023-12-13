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
int is_background(char **args);
int jobs_list_add(pid_t pid, char *cmd, char estado);
struct info_job* jobs_list_find(pid_t pid);
int jobs_list_remove(pid_t pid);
void ctrlz(int signum);

/*
 * Declaraciones de variables
 */
static char mi_shell[COMMAND_LINE_SIZE]; 
int FLAG_DEBUG_1 = 0;
int FLAG_DEBUG_2 = 0;
int FLAG_DEBUG_3 = 0;
/*
 * Struct:  info_job
 * -------------------
 * 
 */
struct info_job {
    int job_number;
    pid_t pid;
    char estado; // 'N', 'E', 'D', 'F'
    char cmd[COMMAND_LINE_SIZE];
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

int jobs_list_add(pid_t pid, char *cmd, char estado) {
    for (int i = 0; i < N_JOBS; i++) {
        if (jobs_list[i].estado == 'N') { // Empty slot found
            jobs_list[i].pid = pid;
            strncpy(jobs_list[i].cmd, cmd, COMMAND_LINE_SIZE - 1);
            jobs_list[i].cmd[COMMAND_LINE_SIZE - 1] = '\0';
            jobs_list[i].estado = estado;
            jobs_list[i].job_number = i; // Assign job number
            return i; // Return job number
        }
    }
    return -1; // No empty slot found
}

int jobs_list_remove(pid_t pid) {
    for (int i = 0; i < N_JOBS; i++) {
        if (jobs_list[i].pid == pid) {
            jobs_list[i].pid = 0;
            jobs_list[i].estado = 'N';
            memset(jobs_list[i].cmd, 0, sizeof(jobs_list[i].cmd));
            return 0; // Successfully removed
        }
    }
    return -1; // Job not found
}


struct info_job* jobs_list_find(pid_t pid) {
    for (int i = 0; i < N_JOBS; i++) {
        if (jobs_list[i].pid == pid) {
            return &jobs_list[i]; // Return pointer to the found job
        }
    }
    return NULL; // Job not found
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
int execute_line(char *line) {
    char *args[ARGS_SIZE];
    int background;
    pid_t pid;
    parse_args(args, line);

    background = is_background(args); // Check for background execution

    if (check_internal(args) == 0) {
        return 0; // Command was internal and handled
    }

    pid = fork();
    if (pid == 0) {
        // Child process
        // Set default signal handlers
        signal(SIGCHLD, SIG_DFL);
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        execvp(args[0], args);
        fprintf(stderr, "%s: Command not found\n", args[0]);
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Parent process
        if (background) {
            printf("[Running in background] PID: %d\n", pid);
            jobs_list_add(pid, line, 'E'); // Add job to jobs_list
        } else {
            // Foreground process
            jobs_list[0].pid = pid;
            jobs_list[0].estado = 'E';
            strncpy(jobs_list[0].cmd, line, COMMAND_LINE_SIZE - 1);
            jobs_list[0].cmd[COMMAND_LINE_SIZE - 1] = '\0';

            // Wait for the foreground process to finish
            pause();

            // Update job status to 'F' after completion
            jobs_list[0].pid = 0;
            jobs_list[0].estado = 'F';
            memset(jobs_list[0].cmd, 0, sizeof(jobs_list[0].cmd));
        }
    } else {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    return 0;
}



int is_background(char **args) {
    int i;
    for(i = 0; args[i] != NULL; i++);

    if (i > 0 && strcmp(args[i-1], "&") == 0) {
        args[i-1] = NULL;  // Remove '&' from args
        return 1;          // True: It's a background job
    }
    return 0;              // False: It's not a background job
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
    char *token = strtok(line, " \t");

    while (token != NULL && num_tokens < ARGS_SIZE - 1) {
        if (token[0] == '#') { // Ignore the rest of the line if it starts with '#'
            break;
        }
        args[num_tokens++] = token;
        token = strtok(NULL, " \t");

        if (FLAG_DEBUG_1==1)
        {
            printf("[parse_args() → token %d: %s]\n",num_tokens,args[num_tokens-1]);
        }
        

    }
    args[num_tokens] = NULL; // End the arguments array with a NULL pointer
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
        exit(0);
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
    return -1;
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
        if (FLAG_DEBUG_2==1)
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

    if (FLAG_DEBUG_2==1)
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
    if (FLAG_DEBUG_2==1)
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
        if (FLAG_DEBUG_3==1)
        {
            printf("[internal_source()→ LINE: %s]\n",line);
        }
        
        

        // Execute each line using execute_line function
        execute_line(line);
    }

    fclose(file);
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
int internal_jobs() {
    printf("ID\tPID\tStatus\tCommand\n");
    for (int i = 0; i < N_JOBS; i++) {
        if (jobs_list[i].estado != 'N') { // Job exists
            printf("%d\t%d\t%c\t%s\n", jobs_list[i].job_number, jobs_list[i].pid, jobs_list[i].estado, jobs_list[i].cmd);
        }
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
 * 
 */
void reaper(int signum) {
    pid_t ended;
    int status;

    while ((ended = waitpid(-1, &status, WNOHANG)) > 0) {
        struct info_job *job = jobs_list_find(ended);
        if (job) {
            job->estado = 'F'; // Mark job as finished
            printf("\n[Job %d] %d finished\n", job->job_number, job->pid);
            jobs_list_remove(ended); // Remove job from jobs_list
        }
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
 */
void ctrlc(int signum) {
    if (jobs_list[0].pid > 0) {
        kill(jobs_list[0].pid, SIGTERM);  // Terminate the foreground process
        jobs_list[0].pid = 0;             // Reset the foreground process PID
        jobs_list[0].estado = 'N';        // Reset the state
    } else {
        printf("\n");
        imprimir_prompt();                // Re-print the prompt
        fflush(stdout);
    }
}

void ctrlz(int signum) {
    if (jobs_list[0].pid > 0) {
        kill(jobs_list[0].pid, SIGTSTP); // Stop the foreground job
        jobs_list[0].estado = 'D';       // Update status to 'Detained'
        // Add job to jobs_list
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


int main(int argc, char *argv[]) {
    strcpy(mi_shell, argv[0]);

    // Initialization of jobs_list
    for (int i = 0; i < N_JOBS; i++) {
        jobs_list[i].pid = 0;
        jobs_list[i].estado = 'N';
        memset(jobs_list[i].cmd, 0, sizeof(jobs_list[i].cmd));
    }

    // Set signal handlers
    signal(SIGCHLD, reaper); // For handling completed child processes
    signal(SIGINT, ctrlc);   // For handling Ctrl+C
    signal(SIGTSTP, ctrlz);  // For handling Ctrl+Z

    char line[COMMAND_LINE_SIZE];
    while (1) {
        if (read_line(line)) {
            execute_line(line);
        }
    }
    return 0;
}



