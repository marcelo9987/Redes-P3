#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "loging.h"

#define BUFFER_LEN 128

/* String global en memoria estática a devolver por la función identify */
char identify_buffer[BUFFER_LEN];

/**
 * @brief   Devuelve una string formateada para identificar cuándo se produce un evento.
 *
 * Devuelve una string propiamente formateada con el instante temporal en que se invoca
 * y el PID del proceso que la llama. Sirve para identificar y localizar temporalmente las acciones.
 *
 * @return  String con el instante de tiempo en el momento de ejecución y el PID del proceso que la invoca.
 */
char* identify(void) {
    struct timeval current_time;
    struct tm* timestamp;
    
    if (gettimeofday(&current_time, NULL) == -1)
        perror("No se pudo obtener el tiempo");

    timestamp = localtime(&(current_time.tv_sec));

    strftime(identify_buffer, BUFFER_LEN, ANSI_COLOR_CYAN "[%a, %d %b %Y, %H:%M:%S.", timestamp);
    sprintf(identify_buffer + strlen(identify_buffer), "%06lu; PID=%d]" ANSI_COLOR_RESET, current_time.tv_usec, getpid());   /* Añadimos al final los microsegundos y el PID */
    
    return identify_buffer;
}
