#include <stdio.h>     /* para printf en depurarión */
#include <string.h>    /* para funciones de strings  */
#include <stdlib.h>    /* Funciones malloc(), free(), y valor NULL */
#include <fcntl.h>     /* Modos de apertura de función open()*/
#include <sys/stat.h>  /* Permisos función open() */
#include <sys/types.h> /* Definiciones de tipos de datos como size_t*/
#include <unistd.h>    /* Funciones read(), write(), close()*/
#include <errno.h>     /* COntrol de errores (errno) */
#include <pthread.h>

#define NUM_THREADS  10
pthread_mutex_t semaforo=PTHREAD_MUTEX_INITIALIZER;

pthread_t hilos [10];
// structuras para gestor de pila
struct my_stack_node
{ // nodo de la pila (elemento)
    void *data;
    struct my_stack_node *next;
};

struct my_stack
{                              // pila
    int size;                  // tamaño de data, nos lo pasarán por parámetro
    struct my_stack_node *top; // apunta al nodo de la parte superior
};
struct my_stack *pila;
/*
 * Función:  my_stack_init
 * -----------------------
 * Inicializa una pila con un tamaño específico.
 *
 * size: tamaño para cada nodo de la pila.
 *
 * retorna: un puntero a la pila recién creada o NULL si la memoria no pudo ser asignada.
 */
struct my_stack *my_stack_init(int size)
{
    struct my_stack *pila = (struct my_stack *)malloc(sizeof(struct my_stack));

    pila->top = NULL;
    pila->size = size;

    return pila; // Devuelve la pila.
}
int my_stack_push(struct my_stack *stack, void *data);
void *my_stack_pop(struct my_stack *stack);
int my_stack_len(struct my_stack *stack);
int my_stack_purge(struct my_stack *stack);
struct my_stack *my_stack_read(char *filename);
int my_stack_write(struct my_stack *stack, char *filename);
void *worker(void *ptr);
/*
 * Función:  my_stack_push
 * -----------------------
 * Agrega un elemento al tope de la pila.
 *
 * stack: puntero a la pila donde se agregará el elemento.
 * data: puntero al dato que se agregará a la pila.
 *
 * retorna: 0 si el elemento se agregó con éxito, o -1 si la pila no existe o no se pudo asignar memoria.
 */
int my_stack_push(struct my_stack *stack, void *data)
{
    // para meter un nuevo nodo a la pila debemos mirar si la pila
    // está inicializada
    if ((stack != NULL) && (stack->size > 0))
    {
        // declaramos un nodo auxiliar, guardamos memoria para el y le asignamos
        // lo necesario para añadirlo a la pila
        struct my_stack_node *nodo = (struct my_stack_node *)malloc(sizeof(struct my_stack_node));

        nodo->data = data;
        nodo->next = stack->top;
        stack->top = nodo;
        // Fue bien
        return 0;
    }
    else
    {
        // hubo un error
        return -1;
    }
}

/*
 * Función:  my_stack_pop
 * ----------------------
 * Elimina y devuelve el elemento del tope de la pila.
 *
 * stack: puntero a la pila de la cual se sacará el elemento.
 *
 * retorna: puntero al dato del tope de la pila o NULL si la pila está vacía o no existe.
 */
void *my_stack_pop(struct my_stack *stack)
{
    if (stack == NULL || stack->top == NULL)
    {
        return NULL;
    }

    // Toma el nodo superior actual.
    struct my_stack_node *currentTop = stack->top;

    // Actualiza el top al siguiente nodo.
    stack->top = currentTop->next;

    // Guarda el dato del nodo superior.
    void *data = currentTop->data;

    // Libera la memoria del nodo superior.
    free(currentTop);

    return data; // Devuelve el dato.
}

/*
 * Función:  my_stack_len
 * ----------------------
 * Devuelve el número de elementos en la pila.
 *
 * stack: puntero a la pila cuya longitud se calculará.
 *
 * retorna: la cantidad de elementos en la pila.
 */
int my_stack_len(struct my_stack *stack)
{
    if (stack == NULL)
    {
        return 0;
    }

    int len = 0;

    struct my_stack_node *current = stack->top;
    while (current != NULL)
    {
        len++;
        current = current->next;
    }

    return len;
}

/*
 * Función:  my_stack_purge
 * ------------------------
 * Libera toda la memoria utilizada por la pila.
 *
 * stack: puntero a la pila que será liberada.
 *
 * retorna: el número total de bytes liberados.
 */
int my_stack_purge(struct my_stack *stack)
{ // Verificamos que el stack no este vacio
    if (stack == NULL)
    {
        return 0;
    }

    int freed_bytes = 0;
    // Bucle de iteracion hasta dejar la pila vacia
    while (stack->top != NULL)
    { // Nodo auxiliar para limpiar el espacio de memoria
        struct my_stack_node *current_node = stack->top;
        stack->top = current_node->next;

        freed_bytes += stack->size;
        free(current_node->data);

        freed_bytes += sizeof(struct my_stack_node);
        free(current_node);
    }

    // Limpiar el espacio de memoria de la pila
    freed_bytes += sizeof(struct my_stack);
    free(stack);

    return freed_bytes;
}

/*
 * Función:  my_stack_write
 * ------------------------
 * Escribe los contenidos de la pila en un archivo.
 *
 * stack: puntero a la pila cuyos contenidos se escribirán.
 * filename: nombre del archivo donde se escribirá la pila.
 *
 * retorna: el número de elementos escritos o -1 si ocurre un error.
 */
int my_stack_write(struct my_stack *stack, char *filename)
{
    if (stack == NULL || filename == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    // Abrimos el archivo para escritura, con permisos de usuario para leer y escribir
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
        perror("Error abriendo el archivo para escribir");
        return -1;
    }

    // Escribir el tamaño de la pila en el archivo y verificar si se escribió correctamente
    if (write(fd, &(stack->size), sizeof(int)) != sizeof(int))
    {
        perror("Error escribiendo el tamaño de la pila al archivo");
        close(fd);
        return -1;
    }

    // Escribir los datos de la pila en el archivo
    int count = 0;
    for (struct my_stack_node *current = stack->top; current != NULL; current = current->next)
    {
        if (write(fd, current->data, stack->size) != stack->size)
        {
            perror("Error escribiendo los datos de la pila al archivo");
            close(fd);
            return -1;
        }
        count++;
    }

    if (close(fd) == -1)
    {
        perror("Error cerrando el archivo después de escribir");
        return -1;
    }
    return count; // Devolver la cantidad de nodos escritos
}

/*
 * Función:  my_stack_read
 * -----------------------
 * Lee los contenidos de una pila desde un archivo.
 *
 * filename: nombre del archivo de donde se leerá la pila.
 *
 * retorna: un puntero a la pila leída o NULL si ocurre un error durante la lectura.
 */
struct my_stack *my_stack_read(char *filename)
{
    if (filename == NULL)
    {
        errno = EINVAL;
        return NULL;
    }

    int fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        perror("Error abriendo archivo para leer");
        return NULL;
    }

    int size;
    if (read(fd, &size, sizeof(int)) != sizeof(int))
    {
        perror("Error leyendo el tamaño de la pila del archivo");
        close(fd);
        return NULL;
    }

    // Leer los datos de la pila del archivo y almacenarlos
    void **dataPointers = NULL;
    int count = 0;
    void *data = malloc(size);
    if (data == NULL)
    {
        perror("Error asignando memoria para los datos");
        close(fd);
        return NULL;
    }

    // Continuar leyendo y asignando memoria para cada elemento de datos
    while (read(fd, data, size) == size)
    {
        void **newDataPointers = realloc(dataPointers, (count + 1) * sizeof(void *));
        if (newDataPointers == NULL)
        {
            perror("Error reasignando memoria para los punteros de datos");
            free(data);
            // Liberamos punteros de datos previamente asignados antes de salir.
            while (count-- > 0)
            {
                free(dataPointers[count]);
            }
            free(dataPointers);
            close(fd);
            return NULL;
        }
        dataPointers = newDataPointers;
        dataPointers[count++] = data;
        data = malloc(size);
        if (data == NULL)
        {
            perror("Error asignando memoria para los próximos datos");
            // Liberamos correctamente toda la memoria asignada antes de salir.
            while (count-- > 0)
            {
                free(dataPointers[count]);
            }
            free(dataPointers);
            close(fd);
            return NULL;
        }
    }
    free(data); // Liberar el último bloque asignado, ya que no es necesario.

    // Inicializar una nueva pila y verificar si se hizo correctamente
    struct my_stack *new_stack = my_stack_init(size);
    if (new_stack == NULL)
    {
        perror("Error inicializando nueva pila");
        // Liberar todos los punteros de datos asignados.
        while (count-- > 0)
        {
            free(dataPointers[count]);
        }
        free(dataPointers);
        close(fd);
        return NULL;
    }

    // Introducir los datos leídos en la nueva pila
    for (int i = count - 1; i >= 0; --i)
    {
        if (my_stack_push(new_stack, dataPointers[i]) == -1)
        {
            perror("Error apilando datos en la nueva pila");
            // Realizar la limpieza necesaria.
            my_stack_purge(new_stack);
            while (count-- > 0)
            {
                free(dataPointers[count]);
            }
            free(dataPointers);
            close(fd);
            return NULL;
        }
    }
    free(dataPointers); // Liberar los punteros de datos.
    if (close(fd) == -1)
    {
        perror("Error cerrando archivo después de leer");
    }

    return new_stack;
}
void *worker(void *ptr){
    void *data;
    int N=1000000;
    
    for(;N;N--){
        
        pthread_mutex_lock(&semaforo); 
        data=my_stack_pop(pila);
        data++;
        my_stack_push(pila,data);
        pthread_mutex_unlock(&semaforo);
        pthread_exit(NULL);
    }
}
int main(int argc, char *argv[])
{
    if (argv[1]==NULL){
        printf("Formato incorrecto: ./[programa] [nombre_fichero]");
    }
    // Comprobamos si la pila existe
    int existe = access(argv[1], F_OK);
     

    if (!existe)
    //si no existe la inicializamos a 0
    {

        struct my_stack *pila = my_stack_init(NUM_THREADS);
        for (int i = 0; i < 10; i++)
        {

            int *dato = malloc(sizeof(int));
            dato = 0;
            my_stack_push(pila, dato);
        }
    }else{
        //si existe pero no hay 10 elementos, metemos los restantes
        struct my_stack *pila= my_stack_read(argv[1]);
        int longitud= my_stack_len(pila);
        int auxiliar;
        if ((auxiliar=NUM_THREADS-longitud) > 0){
            for (int i=auxiliar;auxiliar!=0;auxiliar--){
                 int *dato = malloc(sizeof(int));
                    dato = 0;
                  my_stack_push(pila, dato);
            }
        }
    }

    // PASO 2
   for(int i=0;i<NUM_THREADS;i++){
    pthread_create(&hilos[i],NULL,worker,NULL);
   }
}


