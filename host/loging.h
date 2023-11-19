#ifndef LOGING_H
#define LOGING_H

#include <stdio.h>

/* Colores estándar de ANSI para impresión */
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
/* Macro para establecer los colores de los dispositivos de salida:
 * stderr en rojo, y stdout con colores por defecto. */
#define set_colors() { fprintf(stderr, ANSI_COLOR_RED); fprintf(stdout, ANSI_COLOR_RESET); }


/* Macro para imprimir mensaje de error y salir */
#define fail(message) { perror(ANSI_COLOR_RED message); exit(EXIT_FAILURE); }

/* Macro para imprimir en el log */
#define log_printf(log, format, ...) { if (log) fprintf(log, "%s " format, identify(), ##__VA_ARGS__); }

/* Macro para imprimir en el log */
#define log_and_stdout_printf(log, format, ...) { fprintf(stdout, format, ##__VA_ARGS__); if (log) fprintf(log, "%s " format , identify(), ##__VA_ARGS__); }

/* Macro para imprimir errores en el log */
#define log_printf_err(log, format, ...) { log_printf(log, ANSI_COLOR_RED format ANSI_COLOR_RESET, ##__VA_ARGS__); }


/**
 * @brief   Devuelve una string formateada para identificar cuándo se produce un evento.
 *
 * Devuelve una string propiamente formateada con el instante temporal en que se invoca
 * y el PID del proceso que la llama. Sirve para identificar y localizar temporalmente las acciones.
 * La string devuelta está alojada estáticamente, por lo que no hace falta liberarla, pero se sobrescribe en
 * sucesivas llamadas.
 *
 * @return  String con el instante de tiempo en el momento de ejecución y el PID del proceso que la invoca.
 */
char* identify(void);

#endif /* LOGING_H */
