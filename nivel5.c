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
#include <fcntl.h>
#include <sys/stat.h>


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
int jobs_list_find(pid_t pid);
int jobs_list_remove(int pos);
void ctrlz(int signum);
int is_output_redirection(char **args);
int internal_bg(char **args);
int internal_fg(char **args);

/*
 * Declaraciones de variables
 */
int  n_job=0;
static char mi_shell[COMMAND_LINE_SIZE]; 
int DEBUG_FLAGS[]={0,0,0,0,1};

/*
 * Struct:  info_job
 * -------------------
 * 
 */
struct info_job {
    int job_number;
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
 * Función encargada de imprimir el prompt y leer una linea de
 * la consola con la función fgets. Cuando ya obtenemos esa línea y
 * comprobamos que tiene contenido, cambiaremos el salto de línea por NULL
 * (cosa que nos servirá para el execute line en un futuro)
 * 
 * También usamos la función fflush, que fuerza el vaciado del buffer para que
 * el buffer expulse sus contenidos(por si tiene mensajes en cola) y así mostrar
 * el prompt cuanto antes.
 * 
 *
 * line: puntero que apunta a la línea de la consola(stdin)
 * 
 *
 * retorna: puntero que apunta a la línea pero en vez de tener salto de línea
 * tendra null.
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
 * Función: jobs_list_add() 
 * -------------------
 * 
 * Metodo que añade un trabajo al jobs_list en la última posición disponible
 * del array sino se puede porque el array esta lleno devolvemos -1.
 * 
 */

int jobs_list_add(pid_t pid, char *cmd, char estado) {
  
  n_job++;
  if (n_job < N_JOBS) {
    jobs_list[n_job].pid = pid;
    strncpy(jobs_list[n_job].cmd, cmd, COMMAND_LINE_SIZE - 1);
    jobs_list[n_job].cmd[COMMAND_LINE_SIZE - 1] = '\0';
    jobs_list[n_job].estado = estado;
    jobs_list[n_job].job_number = n_job;
    printf("Job %s added to job_list with index %d\n", cmd, n_job);
    return 1;
  } else {
    return -1; // No empty slot found
  }
}


/*
 * Función: jobs_list_remove() 
 * -------------------
 * 
 * Metodo que elimina el trabajo del jobs_list con el mismo pid pasado
 * por parametros. Eliminar significa poner el último proceso en su posición, si
 * se ejecuta correctamente devuelve un 0 sino un -1.
 * 
 */

int jobs_list_remove(int pos) {
    
    jobs_list[pos].pid = jobs_list[n_job].pid;
    jobs_list[pos].estado = jobs_list[n_job].estado;
    strcpy(jobs_list[pos].cmd,jobs_list[n_job].cmd);
    n_job--;
    return 0; // Successfully removed
        
    
    
}

/*
 * Función: jobs_list_find() 
 * -------------------
 * 
 * Metodo que busca el indice del array de jobs_list en el que se encuentra
 * el pid buscado.
 * 
 */

int jobs_list_find(pid_t pid) {
    for (int i = 1; i < n_job+1; i++) {
        if (jobs_list[i].pid == pid) {
            return i; // Return pointer to the found job
        }
    }
    return 0; // Job not found
}

/*
 * Función:execute_line  
 * -------------------
 * 
 * Función encargada de ejecutar la línea. En este nivel ya implementamos la
 * ejecución de comandos externos. Al inicio, lo que hacemos es guardar la 
 * linea dada en una variable auxiliar debido a que parse_args altera su contenido.
 * Después de esto llamaremos a parse_args, para que los tokens estén dentro de args.
 * 
 * Ahora que ya tenemos todo esto, comprobaremos si es un comando interno mediante
 * check_internals y en el caso de que lo sea será tratado por esta función.
 
 * Llamar a la función is_background() para analizar si en la línea de comandos hay un & al final.
 
 * En el caso de que no tengamos un comando interno, crearemos un hijo y tendremos
 * dos ramas de procesamiento:
 * 
 * Proceso HIJO:
 *  Realiza la llamada al sistema execvp(args[0], args) 
 *  para ejecutar el comando externo solicitado. 
 * Añadiremos en el proceso hijo las llamadas a dos señales SIGCHLD y SIGINT. 
 * La primera se ejecutará cuando un proceso hijo haya finalizado con el metodo reaper()
 * y la segunda cuando presionemos el Control ^C.
 *
 * Proceso PADRE:
 *  Comprobaremos si estamos en el backgorud si es así metemos el trabajo en el
 *  jobs_list con el metodo add sino, actualizará el jobs_list[0], debido a que el proceso hijo 
 *  estará ejecutando el comando. Después, esperará a que el hijo acabe 
 *  mediante wait y actualizará el jobs_list[0], porque habremos acabado.
 * Más tarde en el proceso padre pondremos un
 * pause, cuando un proceso hijo se este ejecutando en primer plano para esperar a que llegue
 * la señal.
 *
 * line : puntero que apunta a la línea que pasamos por consola(stdin)
 * 
 *
 * retorna: siempre 0
 * 
 */
int execute_line(char *line) {
    char *args[ARGS_SIZE];
    int background;
    char *saved_line = line;
    pid_t pid;

    int num_args = parse_args(args, line);

    if (num_args==0)
    {
        return 0;
    }

    int valorcheck = check_internal(args);
    if (valorcheck == 1 || valorcheck == -1)
    {
        return 0;
    }    

    background = is_background(args); // Check for background execution


    pid = fork();
    if (pid == 0) {
        signal(SIGCHLD, SIG_DFL);
        signal(SIGINT, SIG_IGN);  // Ignore SIGINT (Ctrl+C)
        signal(SIGTSTP, SIG_IGN); // Ignore SIGTSTP (Ctrl+Z)
        execvp(args[0], args);
        printf("%s: no se encontro la orden.\n", args[0]);
        exit(-1);

    } else if (pid > 0) {
        
       if (background) {
            printf("[Running in background] PID: %d\n", pid);
            jobs_list_add(pid, saved_line, 'E'); // Add job to jobs_list
            printf("[Added job %s to job_list with PID %d and index of job %d]\n", saved_line,pid,n_job);
        } else {
            // Foreground process
            jobs_list[0].pid = pid;
            jobs_list[0].estado = 'E';
            strncpy(jobs_list[0].cmd, line, COMMAND_LINE_SIZE - 1);
            jobs_list[0].cmd[COMMAND_LINE_SIZE - 1] = '\0';

            // Wait for the foreground process to finish
            if(jobs_list[0].pid>0){
                pause();
            }
        
        }
    } else {
        perror("fork");
        exit(-1);
    }

    return 0;
}


/*
 * Función:is_background(char **args)  
 * -------------------
 * 
 * Función que devuelve 1 si es un comando en background y 0 sino.
 * Si es un metodo en background elimina el último caracter de
 * los argumentos es decir el "&"
 *
 * retorna:
 */

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
 * Función: parse_args 
 * -------------------
 * Trocea la linea obtenida en tokens mediante la función strtok, en nuestro caso los tokens
 * estarán delimitados por los caracteres espacio, tab, salto de línea y return. Por otro lado,
 * tenemos en cuenta que el # representa un comentario, por tanto ignoramos los tokens posteriores a 
 * este carácter.
 * 
 *
 * args: array de arrays en el que introduciremos los tokens
 * line: linea introducida en consola (stdin)
 *
 * retorna: el número de tokens encontrados
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
 * Función:  internal_cd
 *
 * Esta función será realizará el comando cd, dicho comando nos cambiará el directorio de trabajo.
 * Lo primero que realizaremos es dos arrays con dimension COMMAND_LINE_SIZE. El primero lo utilizaremos
 * para comprobar si hemos cambiado de directorio. En el segundo, almacenaremos el directorio indicado
 * según los casos. Los casos son(hay que recalcar que cambiamos de directorio mediante la función chdir
 * en todos):
 *
 * ---- CD para volver al directorio home--------
 *
 *  Este cd no tiene un segundo argumento, es decir el comando sería así: cd NULL,
 *  para este caso simplemente detectamos que no hay segundo argumento y almacenamos
 * en la variable dir el directorio home, mediante la función getenv que nos devuelve
 * la variable de entorno indicada en el parámetro. Posteriormente comprobamos
 * si dir tiene algún contenido, y cambiamos de directorio(se hace en la línea 232)
 *
 * -------- CD para un directorio específico---------
 * Entraremos en un bucle en el que comprobaremos si hay comillas en los argumentos,
 * saldremos de dicho bucle sin ningún cambio y el if de la \ no se cumplirá, así que
 * no tocaremos los argumentos y haremos chdir al final.
 *
 *
 *---------- CD de caso avanzado----------------
 *
 * Este cd admite el paso de directorios con espacios mediante una escritura especial
 * en este caso manejaremos estos casos : cd 'mini shell', cd "mini shell", cd mini/ shell.
 *
 * Primero comprobaremos las comillas, haremos un bucle en el que pasaremos argumento por
 * argumento detectando si hay comillas al inicio o al final.
 *
 *
 * args: array de arrays en el que están los tokens
 *
 * retorna: devuelve 1, -1 si hay error
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
    char dir[COMMAND_LINE_SIZE] = {0};

    if (args[1] == NULL) {
        // Sin argumentos, ir al directorio HOME
        strcpy(dir, getenv("HOME"));
    } else {
        // Manejar argumentos con comillas y posibles espacios escapados
        int i = 1;
        while (args[i] != NULL) {
            char *arg = args[i];
            int inQuote = 0;
            char quoteChar = '\0';

            while (*arg) {
                if ((*arg == '\'' || *arg == '\"') && !inQuote) {
                    // Inicio de una cadena entrecomillada
                    inQuote = 1;
                    quoteChar = *arg;
                } else if (*arg == quoteChar && inQuote) {
                    // Fin de una cadena entrecomillada
                    inQuote = 0;
                } else {
                    // Agregar el carácter si no es comilla de inicio o fin
                    if (!inQuote || (*arg != quoteChar)) {
                        size_t len = strlen(dir);
                        dir[len] = *arg;
                        dir[len + 1] = '\0';
                    }
                }
                arg++;
            }

            // Si el último carácter del argumento actual es una barra invertida
            // y el siguiente argumento no es NULL (caso de espacio escapado)
            if (args[i][strlen(args[i]) - 1] == '\\' && args[i + 1] != NULL) {
                // Reemplazar la barra invertida por un espacio
                dir[strlen(dir) - 1] = ' ';
            } else if (args[i + 1] != NULL) {
                // Agregar un espacio si el siguiente argumento no es NULL
                strcat(dir, " ");
            }
            i++;
        }
    }

    // Eliminar el espacio final si existe
    if (dir[strlen(dir) - 1] == ' ') {
        dir[strlen(dir) - 1] = '\0';
    }

    // Cambiar de directorio
    if (chdir(dir) != 0) {
        perror("Error en chdir()");
        return -1;
    }

    // Obtener e imprimir el directorio actual de trabajo
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Cambiado al directorio: %s\n", cwd);
    } else {
        perror("Error en getcwd()");
        return -1;
    }

    return 1;
}




/*
 * Función:  internal_export
 * 
 * Esta función lo que hace es modificar una variable de entorno con el valor pasado
 * por parámetro, el formato sería así: export HOME=hola. 
 * 
 * En el caso de que no haya segundo argumento, significará que hay un error de sintaxis
 * por tanto lo notificaremos al usuario. 
 * 
 * Si tiene segundo argumento, lo que haremos es:
 * guardar el nombre en una variable y mediante la función strchar, tener un puntero que apunte
 * al = , lo que significará que los siguientes caracteres serán el valor pasado. Posteriormente,
 * detectaremos otro error de sintaxis, en el cual nombre es igual al valor dado por el usuario.
 * 
 * Por otro lado, si no hay errores, pondremos que el = será /0(para facilitar la parte del nombre) y 
 * nos moveremos al siguiente caracter (el primero del valor). Mediante la función getenv conseguiremos
 * la variable pedida y mediante setenv cambiaremos su contenido.
 * 
 *
 * args = array de arrays en el que tenemos todos los tokens
 * 
 *
 * retorna:-1 si da error, 1 si funciona correctamente.
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
 * Función:internal_jobs()  
 * -------------------
 * 
 * Metodo que muestra el array de jobs_list que esten en estado
 * detenido o ejecutando.
 */
int internal_jobs() {
    printf("ID\tPID\tStatus\tCommand\n");
    for (int i = 1; i <= n_job; i++) {
    printf("[%d]\t%d\t%c\t%s\n", jobs_list[i].job_number, jobs_list[i].pid, jobs_list[i].estado, jobs_list[i].cmd);
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
 * -------------------
 * Función que se ejecutará cuando un proceso hijo haya terminado si se ejecuta en primer plano
 * visulizamos el pid, los comandos y la señal y finalizamos el proceso.Sino buscamos donde se posiciona
 * en el array de jobs lo visualizamos y lo eliminamos.
 * 
 */
void reaper(int signum)
{
    signal(SIGCHLD, reaper); // Reinstall the signal handler
    int status = 0;
    pid_t ended;

    while ((ended = waitpid(-1, &status, WNOHANG)) > 0)
    {
        if (ended == jobs_list[0].pid)
        { // Check if the ended process is the foreground process
            if (DEBUG_FLAGS[3])
            {
                printf("[Proceso hijo %d (%s) finalizado por señal %d]\n", ended, jobs_list[0].cmd, WTERMSIG(status));
            }
            jobs_list[0].pid = 0;                           // Reset the foreground job slot
            jobs_list[0].estado = 'N';                      // Set estado to 'N' for Ninguno, since no job is running
            memset(jobs_list[0].cmd, 0, COMMAND_LINE_SIZE); // Clear the command
        }
        else
        {                                    // The ended process is a background process
            int pos = jobs_list_find(ended); // Find the job in the jobs_list
            // If the process was found
            printf("[Background process %d (%s) finished with status %d]\n", jobs_list[pos].pid, jobs_list[pos].cmd, WEXITSTATUS(status));
            jobs_list_remove(pos); // Remove the job from the jobs_list
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
    signal(SIGINT, ctrlc); // Re-establish the signal handler

    if (jobs_list[0].pid > 0) {
        if (strcmp(mi_shell, jobs_list[0].cmd) != 0) {
            printf("[Señal %d enviada a %d (%s) por %d (%s)]\n", signum, jobs_list[0].pid, jobs_list[0].cmd, getpid(), mi_shell);
            kill(jobs_list[0].pid, SIGTERM); // Terminate the foreground process
            jobs_list[0].pid = 0; // Reset the foreground process PID
            jobs_list[0].estado = 'N'; // Reset the state
        } else {
            printf("[Señal SIGTERM no enviada debido a que el proceso en foreground es el shell]\n");
        }
    } else {
        printf("[Señal %d no enviada a %d (%s) debido a que no hay proceso en foreground]\n", signum, getpid(), mi_shell);
    }
    
    printf("\n");
    imprimir_prompt(); // Re-print the prompt
    fflush(stdout);
}


/*
 * Función:ctrlz()  
 * -------------------
 * Funcion que implementa el comando CONTROL Z(^Z) en el que si estamos en un proceso
 * foreground y no es el minishell acabamos la ejecución del programa mandando una señal
 * a partir del metodo kill.
 * 
 *
 */
void ctrlz(int signum) {
    signal(SIGTSTP, ctrlz);

    if (jobs_list[0].pid > 0) {
        if (strcmp(mi_shell, jobs_list[0].cmd) != 0) {
            kill(jobs_list[0].pid, SIGSTOP); // Send SIGTSTP to the foreground job
            printf("[Sent STP signal to PID %d]",jobs_list[0].pid);

            if (DEBUG_FLAGS[4] == 1) {
                printf("\n[+] Job detained: %s\n", jobs_list[0].cmd);
            }
            jobs_list[0].estado='D';
            // Add to jobs_list as 'Detained' before resetting jobs_list[0]
            jobs_list_add(jobs_list[0].pid, jobs_list[0].cmd, jobs_list[0].estado);
            
            // Resetting jobs_list[0]
            jobs_list[0].pid = 0;
            jobs_list[0].estado = 'N';
            memset(jobs_list[0].cmd, '\0', sizeof(jobs_list[0].cmd));
        } else {
            if (DEBUG_FLAGS[4] == 1) {
                printf("[Señal SIGSTOP no enviada debido a que el proceso en foreground es el shell]\n");
            }
        }
    } else {
        if (DEBUG_FLAGS[4] == 1) {
            printf("[Señal SIGSTOP no enviada debido a que no hay proceso en foreground]\n");
        }
    }

    imprimir_prompt();
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
    memset(jobs_list[0].cmd, '\0', sizeof(jobs_list[0].cmd));

    // Set signal handlers
    signal(SIGCHLD, reaper);
    signal(SIGINT, ctrlc);
    signal(SIGTSTP, ctrlz);  // For handling Ctrl+Z
    char line[COMMAND_LINE_SIZE];
    while (1) {
        if (read_line(line)) {
            execute_line(line);
        }
    }
    return 0;
}
