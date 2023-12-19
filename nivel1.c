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

#define COMMAND_LINE_SIZE 1024
#define MAX_ARGS 64
#define COMMAND_LINE_SIZE 1024
#define MAX_ARGS 64

int check_internal(char **args);
int parse_args(char **args, char *line);
int execute_line(char *line);
char *read_line(char *line);
void imprimir_prompt();
int internal_cd(char **args);
int internal_export(char **args);
int internal_source(char **args);
int internal_jobs();

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
    fflush(stdout);
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
 * Función: execute_line 
 * Función encargada de ejecutar la línea, en este nivel
 * solo instanciaremos el array, separaremos los argumentos
 * por tokens, mediante el parse_args y haremos comandos internos
 * mediante check_inernal
 * 
 *
 * line : puntero que apunta a la línea que pasamos por consola(stdin)
 * 
 *
 * retorna: valor dado por check_internal (si es un comando interno 1 si no lo es 0)
 */
int execute_line(char *line) {

    char *args[MAX_ARGS];
    parse_args(args, line);
    return check_internal(args);
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
    while (token != NULL && num_tokens < MAX_ARGS - 1) {
        if (token[0] == '#') { 
            break;
        }
       
        args[num_tokens++] = token;
        token = strtok(NULL,  " \t\n\r");
  printf("parse_args()->El token número: %d es: %s \n",num_tokens,args[num_tokens-1]);
    }
    printf("parse_args()-> el número de tokens es: %d  \n",num_tokens);
    args[num_tokens] = NULL; //Acabamos con un null
    
    return num_tokens;
}

/*
 * Función: check_internal
 * -------------------
 * Función encargada de detectar si el comando pasado es interno
 * simplemente comprueba si el primer argumento es igual al comando interno,
 * en el caso que lo sea realizaremos dicha función. En el caso de que el comando
 * no sea ninguna función interna se devolverá 0.
 * 
 *
 * args: array de arrays con los tokens 
 * 
 *
 * retorna: devuelve 1 en el caso de que sea interna y 0 en el caso de que no lo sea.
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
    return 0;
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
 printf("internal_cd()->cambia de directorio actual de trabajo\n");
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
printf("internal_export()->Asigna variables de entorno\n");
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
    printf("internal_source()->Ejecuta las líneas de comandos del fichero nombre_fichero \n" );
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
    printf("internal_jobs()->Muestra el PID, el estado y el nombre de los procesos que se están ejecutando en background o que han sido detenidos\n");

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
    printf("internal_fg()->Envía un trabajo del background al foreground, o reactiva la ejecución en foreground de un trabajo que había sido detenido.\n");
    
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
    printf("internal_bg->Reactiva un proceso detenido para que siga ejecutándose pero en segundo plano.\n");
    
    return 1;
}

/*
 * Función:  main
 * -------------------
 *  El main de este nivel únicamente contiene el bucle infinito
 *  que realizará el funcionamiento básico del mini shell. Simplemente
 *  leerá las líneas(read_line) y posteriormente las ejecutará(execute_line).
 *
 * 
 * 
 *
 * retorna: Siempre devuelve 0.
 */
int main() {
    char line[COMMAND_LINE_SIZE];
    while (1) {
        if (read_line(line)) {
            execute_line(line);
        }
    }
    return 0;
}
