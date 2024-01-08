/*
Práctica 3 
Dylan Luigi Canning García
Juan Marí González
Antonio Marí González
Descripción:
1. Este programa en C lee una pila de datos desde un archivo
    y realiza operaciones sobre sus elementos. Procesa cada
    elemento de la pila, calculando la suma, el valor mínimo,
    el valor máximo y el promedio de los datos. Luego de procesar
    y mostrar estos valores, libera los recursos utilizados por la pila. 

*/

#include "my_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

/*
 * Función principal del programa.
 * Argumentos:
 *  argc - Número de argumentos de la línea de comandos.
 *  argv - Array de cadenas con los argumentos de la línea de comandos.
 */
int main(int argc, char *argv[]) {
    // Verifica que se haya pasado el nombre del archivo de la pila como argumento
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s <stack_file>\n", argv[0]);
        return 1;
    }

    char *filename = argv[1];
    // Lee la pila desde el archivo
    struct my_stack *stack = my_stack_read(filename);
    // Verifica si la lectura de la pila fue exitosa
    if (!stack) {
        fprintf(stderr, "Error reading stack from file: %s\n", filename);
        return 1;
    }

    // Variables para calcular la suma, mínimo, máximo y promedio de los valores en la pila
    int count = 0, sum = 0, min = INT_MAX, max = INT_MIN;
    // Procesa cada elemento en la pila
    while (stack->top != NULL) {
        int *data = (int *)my_stack_pop(stack);
        printf("Value: %d\n", *data);
        sum += *data;
        if (*data < min) min = *data;
        if (*data > max) max = *data;
        free(data); // Libera el elemento extraído de la pila
        count++;
    }

    // Calcula el promedio de los valores
    float average = (count > 0) ? (float)sum / count : 0;
    // Imprime la suma, el mínimo, el máximo y el promedio
    printf("Sum: %d Min: %d Max: %d Average: %.2f\n", sum, min, max, average);

    // Limpia y libera los recursos de la pila
    my_stack_purge(stack);
    return 0;
}
