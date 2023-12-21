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
#define COMMAND_LINE_SIZE 1024
#define ARGS_SIZE 64
#define N_JOBS 64

/*
 * Declaraciones
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
int DEBUG_FLAGS[]={0,0,1};
static char mi_shell[COMMAND_LINE_SIZE]; 

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
 * Función encargada de ejecutar la línea. En este nivel ya implementamos la
 * ejecución de comandos externos. Al inicio, lo que hacemos es guardar la 
 * linea dada en una variable auxiliar debido a que parse_args altera su contenido.
 * Después de esto llamaremos a parse_args, para que los tokens estén dentro de args.
 * 
 * Ahora que ya tenemos todo esto, comprobaremos si es un comando interno mediante
 * check_internals y en el caso de que lo sea será tratado por esta función.
 * 
 * En el caso de que no tengamos un comando interno, crearemos un hijo y tendremos
 * dos ramas de procesamiento:
 * 
 * Proceso HIJO:
 *  Realiza la llamada al sistema execvp(args[0], args) 
 *  para ejecutar el comando externo solicitado. 
 *
 * Proceso PADRE:
 *  Actualizará el jobs_list[0], debido a que el proceso hijo 
 *  estará ejecutando el comando. Después, esperará a que el hijo acabe 
 *  mediante wait y actualizará el jobs_list[0], porque habremos acabado.
 * 
 * line : puntero que apunta a la línea que pasamos por consola(stdin)
 * 
 *
 * retorna: siempre 0
 */
int execute_line(char *line){
    pid_t pid;
    int status;
    char *auxiliar;

    auxiliar = line;
    char *args[ARGS_SIZE];
    parse_args(args, line);
    int valorcheck= check_internal(args);
    if ((valorcheck==1||valorcheck==-1)) {
        return 0; 
    }

    pid = fork();

    if (pid == 0){
        
        execvp(args[0], args);
        printf("%s: no se encontro la orden.\n", args[0]);
        exit(-1);
    } else if (pid > 0){
        if (DEBUG_FLAGS[2]==1){
        printf("[execute_line()→ PID padre: %d (%s)]\n",getpid(),mi_shell);
        }
      

        
        jobs_list[0].estado = 'E';
        strncpy(jobs_list[0].cmd, auxiliar, COMMAND_LINE_SIZE - 1);
        jobs_list[0].cmd[COMMAND_LINE_SIZE - 1] = '\0';
        jobs_list[0].pid = pid;

        if (DEBUG_FLAGS[2]==1){
        printf("[execute_line()→ PID hijo: %d (%s)]\n",pid,jobs_list[0].cmd);
        }
       
        pid_t finished_pid = wait(&status);

        if (DEBUG_FLAGS[2]==1){
        printf("[execute_line()→ Proceso hijo %d (%s) finalizado con exit(), status: %d]\n",finished_pid,jobs_list[0].cmd,status);
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
 * retorna: devuelve 1 en el caso de que sea interna y vaya bien, -1 si hay un error dentro de la instrucción y 2 en el caso de que no lo sea.
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
    return 2;
    }
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
 *  
 *  Este cd tiene un formato de tipo : cd directorio, por tanto
 *  lo que realizaremos es comprobar si hay segundo argumento. Nosotros
 *  no nos preocupamos de comprobar si dicho argumento es un directorio, sino que
 *  ejecutamos chdir y si da error significa que no será un buen argumento.
 *
 *---------- CD de caso avanzado----------------
 *
 * Este cd admite el paso de directorios con espacios mediante una escritura especial
 * en este caso manejaremos estos casos : cd 'mini shell', cd "mini shell", cd mini/ shell.
 * Para detectar este caso lo único que hemos hecho es poner los dos anteriores casos antes, de
 * tal manera que si no es ninguno de los dos, por descarte será el tercero. 
 * 
 *  Lo que hemos decidido hacer en todos los casos es: concatenar los argumentos(mediante strcat) 
 *  ponerles espacio y meterlos en la variable dir  de tal manera que dir quedaría así en los anteriores
 *  anteriores ejemplos: 'mini shell', "mini shell", mini/ shell. Posteriormente,
 * 
 *  
 * args: array de arrays en el que están los tokens
 *
 * retorna:
 */
int internal_cd(char **args) {
    char cwd[COMMAND_LINE_SIZE];
    char dir[COMMAND_LINE_SIZE] = {0}; 

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

        for (int i = 1; args[i] != NULL; i++) {
            if (i > 1) {
                strcat(dir, " "); 
            }
            strcat(dir, args[i]); 
        }
  
        if (dir[0] == '\"' || dir[0] == '\'') {
            size_t len = strlen(dir);
            if (dir[len - 1] == dir[0]) {
                dir[len - 1] = '\0';
                memmove(dir, dir + 1, len - 1);
            }
        }
    }
    


    if (chdir(dir) != 0) {
        perror("chdir() error");
        return -1;
    }

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        if (DEBUG_FLAGS[1]==1){
        printf("Cambiado al directorio: %s\n", cwd);
        }
    } else {
        perror("getcwd() error");
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

  
    *value = '\0';  
    value++;       

   
    char *old_value = getenv(name);
     if (DEBUG_FLAGS[1]==1){
    printf("[internal_export()→ antiguo valor para %s: %s]\n", name, old_value ? old_value : "(null)");
     }
  
    if (setenv(name, value, 1) != 0) {
        perror("setenv() error");
        return -1;
    }

   
    char *new_value = getenv(name);
     if (DEBUG_FLAGS[1]==1){
    printf("[internal_export()→ nuevo valor para %s: %s]\n", name, new_value ? new_value : "(null)");
     }
    return 1;
}


/*
 * Función: internal_source  
 * 
 * Esta función lee un fichero y ejecuta los comandos encontrados
 * en el mismo.
 * 
 * Para hacer esto, primero comprobamos si existe segundo token,
 * en el caso de que exista, abrimos el fichero con el segundo de argumento,
 * si nos pasan algo que no sea un fichero la apertura nos dará error, si
 * esto no ocurre haremos un bucle en el que iremos leyendo línea por línea
 * el fichero. Después de esto, hacemos un fflush( para vaciar el buffer)
 * y ejecutamos la linea leída.
 * 
 * 
 *
 * 
 * 
 *
 * retorna: 1 si funciona, -1 si hay un error.
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
        if (DEBUG_FLAGS[2]==1){
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
 * Función:  main
 * -------------------
 *  El main de este nivel ya contiene más elementos
 *  aparte del bucle de funcionamiento del mini_shell.
 * 
 *  Antes de este bucle guardamos el comando de ejecución del minishell
 *  (el cual visualizaremos en execute_line). Posteriormente, inicializaremos
 *  los valores para la posición 0(foreground) del jobs_list. Por otro lado, 
 *  el memset lo usamos para inicializar a /0 el atributo cmd de jobs_list.
 *  
 *  
 *
 * 
 * 
 *
 * retorna: Siempre devuelve 0.
 */


int main(int argc, char *argv[] )
{
    strcpy(mi_shell, argv[0]); 

    jobs_list[0].pid = 0;
    jobs_list[0].estado = 'N';
    memset(jobs_list[0].cmd, 0, sizeof(jobs_list[0].cmd));

    char line[COMMAND_LINE_SIZE];
    while (1)
    {
        if (read_line(line))
        {
            execute_line(line);
        }
    }
    return 0;
}
