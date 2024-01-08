/*
Práctica 3 
Dylan Luigi Canning García
Juan Marí González
Antonio Marí González
Descripción:
- Se define el número de hilos (NUM_THREADS) y el número de iteraciones por hilo (ITERATIONS).

- Se inicializa un mutex (pthread_mutex_t) para garantizar el acceso exclusivo a una pila compartida (stack) entre múltiples hilos.

- Se define una función llamada 'worker' que representa la tarea realizada por cada hilo. Esta función se encarga de manipular datos en la pila de manera concurrente. Aunque recibe un puntero 'ptr', en este caso específico no se utiliza, pero se mantiene por compatibilidad con la biblioteca pthread.

- La función 'main' del programa realiza las siguientes acciones:
  1. Lee o inicializa la pila desde un archivo, y si no existe, la crea e inicializa con datos.
  2. Crea un conjunto de hilos (threads) que ejecutan la función 'worker' de manera concurrente.
  3. Espera a que todos los hilos terminen su ejecución antes de continuar.
  4. Guarda los datos de la pila en un archivo y libera los recursos utilizados.

Este programa se utiliza para demostrar el uso de hilos en la manipulación de una pila dinámica. Los hilos acceden a la pila de manera concurrente, utilizando un mutex para garantizar la exclusión mutua y evitar condiciones de carrera.
*/


#include "my_lib.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define NUM_THREADS 10  // Número de hilos
#define ITERATIONS 1000000  // Número de iteraciones por hilo

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex para control de acceso concurrente a la pila

struct my_stack *stack;

/*
 * Función:  worker
 * ----------------
 * Función ejecutada por cada hilo para manipular datos en la pila.
 *
 * ptr: puntero a los datos pasados al hilo. En este caso específico, este
 *      parámetro no se utiliza. La firma de la función se mantiene en este
 *      formato estándar para compatibilidad con la biblioteca pthread. 
 *      
 *
 * retorna: Nada. La función retorna NULL tras completar su ejecución.
 */
void *worker(void *ptr)
{

    pthread_t id = pthread_self();
    for (int i = 0; i < ITERATIONS; i++)
    {
        pthread_mutex_lock(&mutex);
        int *data = (int *)my_stack_pop(stack);
        pthread_mutex_unlock(&mutex);
        if (data != NULL)
        {
            (*data)++;
        }
        pthread_mutex_lock(&mutex);
        if (data != NULL)
        {
            my_stack_push(stack, data);
        }
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    printf("Threads: %d, Iterations: %d\n", NUM_THREADS, ITERATIONS);
    if (argc != 2)
    {
        fprintf(stderr, "USAGE: %s <stack_file>\n", argv[0]);
        return 1;
    }

    char *filename = argv[1];

    /*
     * Paso 1: Preparar la pila
     * Lee o inicializa la pila y llena con datos si es necesario.
     */
    stack = my_stack_read(filename);
    if (stack == NULL)
    {
        printf("Error abriendo archivo para leer: No such file or directory\n");
        stack = my_stack_init(sizeof(int));
        for (int i = 0; i < NUM_THREADS; ++i)
        {
            int *data = (int *)malloc(sizeof(int));
            *data = 0;
            my_stack_push(stack, data);
        }
    }
    else
    {
        printf("initial stack content:\n");
        struct my_stack_node *current = stack->top;
        while (current != NULL)
        {
            printf("%d\n", *(int *)current->data);
            current = current->next;
        }
    }

    printf("stack->size: %d\n", stack->size);
    printf("initial stack length: %d\n", my_stack_len(stack));

    /*
     * Paso 2: Crear hilos
     * Inicializa y lanza los hilos que trabajan sobre la pila.
     */
    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; ++i)
    {
        if (pthread_create(&threads[i], NULL, worker, NULL) != 0)
        {
            perror("Failed to create the thread");
        }
        else
        {
            printf("%d) Thread %lu created\n", i, threads[i]);
        }
    }

    /*
     * Paso 3: Esperar a que los hilos terminen
     * Realiza join de todos los hilos para asegurar que terminen su ejecución.
     */
    for (int i = 0; i < NUM_THREADS; ++i)
    {
        if (pthread_join(threads[i], NULL) != 0)
        {
            perror("Failed to join the thread");
        }
    }

    printf("new stack length: %d\n", my_stack_len(stack));
    printf("stack content after threads iterations:\n");
    struct my_stack_node *current = stack->top; // Define 'current' here
    while (current != NULL)
    {
        printf("%d\n", *(int *)current->data);
        current = current->next;
    }

    /*
     * Paso 4: Volcar la pila al archivo y limpiar
     * Guarda los datos de la pila en un archivo y libera los recursos.
     */
    my_stack_write(stack, filename);
    printf("Written elements from stack to file: %d\n", my_stack_len(stack));
    int released_bytes = my_stack_purge(stack);
    printf("Released bytes: %d\n", released_bytes);

    pthread_mutex_destroy(&mutex);

    printf("Bye from main\n");
    return 0;
}
