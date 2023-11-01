/* lib.h librería con las funciones equivalentes a las
de <string.h> y las funciones y estructuras para el
manejo de una pila */

#include <stdio.h>     /* para printf en depurarión */
#include <string.h>    /* para funciones de strings  */
#include <stdlib.h>    /* Funciones malloc(), free(), y valor NULL */
#include <fcntl.h>     /* Modos de apertura de función open()*/
#include <sys/stat.h>  /* Permisos función open() */
#include <sys/types.h> /* Definiciones de tipos de datos como size_t*/
#include <unistd.h>    /* Funciones read(), write(), close()*/
#include <errno.h>     /* COntrol de errores (errno) */

//declaraciones funciones libreria string
size_t my_strlen(const char *str);
int my_strcmp(const char *str1, const char *str2);
char *my_strcpy(char *dest, const char *src);

size_t my_strlen(const char *str) {
   size_t len = 0;
   int i = 0;


   while (str[i]) { // o while (str[i]!='\0')
           i++;
           len++;
   }
   return len;
}

//Metodo para copiar un array de caracteres en otro array
char *my_strcpy(char *dest, const char *src){
   //Inicializamos el indice
    int i=0;
    //Bucle para pasar por todas los caracteres del array
    while (*(src+i)) { 
       //asignamos en la direccion correspondiente el caracter
        *(dest+i)=*(src+i);
       //Aumentamos el indice
        i++;
           
   }
   //Finalmente ponemos el \0 al final
   *(dest+i)='\0';
   
   return dest;
}

int my_strcmp(const char *str1, const char *str2) {
    for (int i = 0; ; i++) {
        // Si ambos caracteres son iguales y no son el carácter de terminación
        if (str1[i] == str2[i] && str1[i] != '\0') {
            continue;
        }
        
        // Devolvemos la diferencia entre los códigos ASCII
        return str1[i] - str2[i];
    }

}





char *my_strncpy(char *dest, const char *src, size_t n) {
    size_t i;

    for (i = 0; i < n && src[i] != '\0'; ++i) {
        dest[i] = src[i];
    }
    
    while(i<n){
        dest[i]='\0';
        i++;
    }
    
    return dest;
}
char *my_strcat(char *dest, const char *src){
    char *ptr = dest;
    //Fijamos el puntero del destino a la ultima posicion.
    while (*ptr != '\0')
    {
        ++ptr;
    }
    while (*src != '\0')
    {
        *ptr = *src;
        ++ptr;
        ++src;
    }
    *ptr='\0';
    return dest;
}
char *my_strchr(const char *str, int c) {
    while (*str != '\0') {
        if (*str == c) {
            return (char *)str;
        }
        ++str;
    }
    if (*str == c) {
        return (char *)str;
    }

    return NULL;
}

// char *my_strncat(char *dest, const char *src, size_t n);

//structuras para gestor de pila
struct my_stack_node {      // nodo de la pila (elemento)
    void *data;
    struct my_stack_node *next;
};

struct my_stack {   // pila
    int size;       // tamaño de data, nos lo pasarán por parámetro
    struct my_stack_node *top;  // apunta al nodo de la parte superior
};  

//declaraciones funciones gestor de pila
struct my_stack *my_stack_init(int size);

int my_stack_push(struct my_stack *stack, void *data);

void *my_stack_pop(struct my_stack *stack) {
    if (stack == NULL || stack->top == NULL) {
        return NULL; 
    }

    // Coger el top-node actual  
    struct my_stack_node *currentTop = stack->top;

    // Actualizar puntero
    stack->top = currentTop->next;

    //Extraer dato del nodo actual
    void *data = currentTop->data;

    // Liberar nodo superior
    free(currentTop);
    // Dato extraido
    return data;
}

int my_stack_len(struct my_stack *stack){
    //Mira si el stack es null, length == 0
    if (stack == NULL) {
        return 0; 
    }

    int len = 0;
    //Emepezamos desde arriba del stack
    struct my_stack_node *current = stack->top;

    while (current != NULL) {
        len++;
        current = current->next;
    }
    
    return len;
}
int my_stack_purge(struct my_stack *stack){
    my_stack_node auxiliar;
    int bytes=0;
    for(int i=stack->size-1,i>=0,i--){
        auxiliar=stack->top;
        bytes += sizeof(auxiliar);
        auxiliar.free();
        stack->top=stack->top->next;
        
    }

    
    bytes += sizeof(stack);
   stack.free();
    return bytes;
}
struct my_stack *my_stack_read(char *filename);

int my_stack_write(struct my_stack *stack, char *filename);
    
