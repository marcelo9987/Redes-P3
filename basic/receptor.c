#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

//TODO: valor temporal para que compile. Hay que buscarle sitio y valor adecuado
#define MAX_BYTES_RECVFROM 128

#include "host.h"
#include "loging.h"

#define MAX_BYTES_RECV 128
#define DEFAULT_OWN_PORT 8080
#define DEFAULT_LOG_FILE "receptor.log"

/**
 * Estructura de datos para pasar a la función process_args.
 * Debe contener siempre los campos int argc, char** argv, provenientes de main,
 * y luego una cantidad variable de punteros a las variables que se quieran inicializar
 * a partir de la entrada del programa.
 */
struct arguments {
    int argc;
    char** argv;
    uint16_t* own_port;
    char** log_file;
};


/**
 * @brief   Procesa los argumentos del main.
 *
 * Procesa los argumentos proporcionados al programa por línea de comandos,
 * e inicializa las variables del programa necesarias acorde a estos.
 *
 * @param args  Estructura con los argumentos del programa y punteros a las
 *              variables que necesitan inicialización.
 */
static void process_args(struct arguments args);


/**
 * @brief Imprime la ayuda del programa.
 *
 * @param exe_name  Nombre del ejecutable (argv[0]).
 */
static void print_help(char *exe_name);


/***** TODO: comentar **/
void handle_data(Host self);



int main(int argc, char** argv) {
    Host receiver;
    uint16_t own_port;
    char *log_file;
    struct arguments args = {
        .argc = argc,
        .argv = argv,
        .own_port = &own_port,
        .log_file = &log_file
    };

    set_colors();

    process_args(args);

    receiver = create_own_host(AF_INET, SOCK_DGRAM, 0, own_port, log_file);

    handle_data(receiver);

    close_host(&receiver);

    exit(EXIT_SUCCESS);
}


void handle_data(Host self) {
    ssize_t received_bytes = 0;
    char received_messsage[MAX_BYTES_RECVFROM];
    char sender_ip[INET_ADDRSTRLEN];
    struct sockaddr_in other_address;
    socklen_t size_addr = sizeof(struct sockaddr_in);

    if( (received_bytes = recvfrom(self.socket, received_messsage, MAX_BYTES_RECV, 0, (struct sockaddr *) &(other_address), &size_addr)) < 0 ) {
        log_printf_err(self.log, "Error en la recepción del mensaje\n");
        fail("Error en la recepción del mensaje");
    }

    printf("Han sido recibidos %lu bytes desde %s:%u\n", received_bytes, inet_ntop(self.domain, &other_address, sender_ip, INET_ADDRSTRLEN), ntohs(other_address.sin_port));
    printf("Mensaje recibido : %s\n", received_messsage);
    log_printf(self.log, "Han sido recibidos %lu bytes desde %s:%u.\n", received_bytes, sender_ip, ntohs(other_address.sin_port));
}


static void print_help(char *exe_name) {
    /** Cabecera y modo de ejecución **/
    printf("Uso: %s [-p] <port> [-l <log> | --no-log] [-h]", exe_name);

    /** Lista de opciones de uso **/
    printf(" Opción\t\tOpción larga\t\tSignificado\n");
    printf(" -p <port>\t--port <port>\t\tPuerto en el que escucha el receptor al que conectarse.\n");
    printf(" -l <log>\t--log <log>\t\tNombre del archivo en el que guardar el registro de actividad del receptor.\n");
    printf(" -n\t\t--no-log\t\tNo crear archivo de registro de actividad.\n");
    printf(" -h\t\t--help\t\t\tMostrar este texto de ayuda y salir.\n");

    /** Consideraciones adicionales **/
    printf("\nPueden especificarse los parámetros <port> para el puerto en los que escucha el receptor sin escribir la opción '-p', siempre y cuando esta sea el primer parámetro que se pasa a la función.\n");
    printf("\nSi se especifica varias veces un argumento, o se especifican las opciones \"--log\" y \"--no-log\" a la vez, el comportamiento está indefinido.\n");
}


static void process_args(struct arguments args) {
    char *current_arg;
    int i;

    /* Inicializar los valores de puerto y backlog a sus valores por defecto */
    *args.own_port = DEFAULT_OWN_PORT;
    *args.log_file = DEFAULT_LOG_FILE;

    for (i = 1; i < args.argc; i++) {
        /* Procesamos los argumentos (sin contar el nombre del ejecutable) */
        current_arg = args.argv[i];
        if (current_arg[0] == '-') { /* Flag de opción */
            /* Manejar las opciones largas */
            if (current_arg[1] == '-') { /* Opción larga */
                if (!strcmp(current_arg, "--port"))
                { current_arg = "-p"; }
                else if (!strcmp(current_arg, "--help"))
                { current_arg = "-h"; }
            }

            switch (current_arg[1]) {
                case 'p':   /* Puerto */
                    if (++i < args.argc) {
                        *args.own_port = atoi(args.argv[i]);
                        if (*args.own_port < 0) {
                            fprintf(stderr, "El valor de puerto especificado (%s) no es válido.\n\n", args.argv[i]);
                            print_help(args.argv[0]);
                            exit(EXIT_FAILURE);
                        }
                    } else {
                        fprintf(stderr, "Puerto no especificado tras la opción '-p'.\n\n");
                        print_help(args.argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    break;
                case 'l':   /* Log */
                    if (++i < args.argc) {
                        *args.log_file = args.argv[i];
                    } else {
                        fprintf(stderr, "Nombre del log no especificado tras la opción '-l'.\n\n");
                        print_help(args.argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    break;
                case 'n':   /* No-log */
                    *args.log_file = NULL;
                    break;
                case 'h':   /* Ayuda */
                    print_help(args.argv[0]);
                    exit(EXIT_SUCCESS);
                default:
                    fprintf(stderr, "Opción '%s' desconocida\n\n", current_arg);
                    print_help(args.argv[0]);
                    exit(EXIT_FAILURE);
            }
        } else if (i == 1) {    /* Se especificó el puerto como primer argumento */
            *args.own_port = atoi(args.argv[i]);
            if (*args.own_port < 0) {
                fprintf(stderr, "El valor de puerto especificado (%s) no es válido.\n\n", args.argv[i]);
                print_help(args.argv[0]);
                exit(EXIT_FAILURE);
            }
        }
    }
}
