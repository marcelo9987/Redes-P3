#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

#include "host.h"
#include "loging.h"
// --

#define MESSAGE_SIZE 128

#define DEFAULT_OWN_PORT 7000
#define DEFAULT_RECEIVER_IP "127.0.0.1"
#define DEFAULT_RECEIVER_PORT 8080
#define DEFAULT_LOG_FILE "emisor.log"
//--

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
    uint16_t* receiver_port;
    char** receiver_ip;
    char** log_file;
};

/**
 * @brief   Procesa los argumentos del main
 *
 * Procesa los argumentos proporcionados al programa por línea de comandos,
 * e inicializa las variables del programa necesarias acorde a estos.
 *
 * @param args  Estructura con los argumentos del programa y punteros a las
 *              variables que necesitan inicialización.
 */
static void process_args(struct arguments args);

/**
 * @brief   Imprime la ayuda del programa
 *
 * @param exe_name  Nombre del ejecutable (argv[0])
 */
static void print_help(char* exe_name);


/**** TODO: documentar */
void send_message(Host own, Host remote);

// --
int main(int argc, char** argv) {
    Host self;
    Host other;
    uint16_t own_port, receiver_port;
    char* log_file;
    char* receiver_ip;
    struct arguments args = {
            .argc = argc,
            .argv = argv,
            .own_port = &own_port,
            .receiver_port = &receiver_port,
            .receiver_ip = &receiver_ip,
            .log_file = &log_file,
    };

    set_colors();

    process_args(args);

    self = create_own_host(AF_INET, SOCK_DGRAM, 0, own_port, log_file);

    other = create_remote_host(AF_INET, SOCK_DGRAM, 0, receiver_ip, receiver_port);

    send_message(self, other);

    close_host(&self);
    close_host(&other);

    return 0;
}


void send_message(Host own, Host remote) {
    char message[MESSAGE_SIZE] = {0};
    ssize_t sent_bytes;

    printf("\nEnviando mensaje al receptor %s:%u...\n", remote.ip, remote.port);
    log_printf(own.log, "Enviando mensaje al receptor %s:%u...\n", remote.ip, remote.port);

    snprintf(message, MESSAGE_SIZE, "El host %s en %s:%u te saluda.\n", own.hostname, own.ip, own.port);

    /* Enviar el mensaje al cliente */
    if ( (sent_bytes = sendto(own.socket, message, strlen(message) + 1, 0, (struct sockaddr *) &remote.address, sizeof(remote.address))) < 0) {
        log_printf_err(own.log, "No se pudo enviar el mensaje\n");
        fail("No se pudo enviar el mensaje");
    }


    printf("Enviados %lu bytes en el mensaje para %s:%u.\n", sent_bytes, remote.ip, remote.port);
    log_printf(own.log, "Enviados %lu bytes en el mensaje para %s:%u.\n", sent_bytes, remote.ip, remote.port);
}


static void print_help(char* exe_name) {
    /** Cabecera y modo de ejecución **/
    printf("Uso: %s [[-p] <port>] [[-i] <ip>] [[-r] <port>] [-l <log> | --no-log] [-h]\n\n", exe_name);

    /** Lista de opciones de uso **/
    printf(" Opción\t\tOpción larga\t\tSignificado\n");
    printf(" -p <port>\t--port <port>\t\tPuerto por el que enviará mensajes el emisor.\n");
    printf(" -i <ip>\t--ip <ip>\t\tIP del receptor al que enviar el mensaje.\n");
    printf(" -r <port>\t--remote <port>\tPuerto del host remoto por el que enviar el mensaje.\n");
    printf(" -l <log>\t--log <log>\t\tNombre del archivo en el que guardar el registro de actividad del emisor.\n");
    printf(" -n\t\t--no-log\t\tNo crear archivo de registro de actividad.\n");
    printf(" -h\t\t--help\t\t\tMostrar este texto de ayuda y salir.\n");

    /** Consideraciones adicionales **/
    printf("\nSi se especifica cualquiera de las opciones '-p', '-i' o '-r', deben especificarse también las demás con sus respectivos flags de opción.\n");
    printf("\nPuede especificarse el parámetro <port> para el puerto por el que se comunica el emisor sin escribir la opción '-p', siempre y cuando este sea el primer parámetro que se pasa a la función. De igual forma, se pueden especificar la IP y puerto de destino sin escribir los flags '-i' ni '-r'.\n");
    printf("\nSi no se especifica alguno de los argumentos, el servidor se ejecutará con sus valores por defecto, a saber: DEFAULT_OWN_PORT=%u; DEFAULT_RECEIVER_IP=%s; DEFAULT_RECEIVER_PORT=%u; DEFAULT_LOG_FILE=%s\n", DEFAULT_OWN_PORT, DEFAULT_RECEIVER_IP, DEFAULT_RECEIVER_PORT, DEFAULT_LOG_FILE);
    printf("\nSi se especifica varias veces un argumento, o se especifican las opciones \"--log\" y \"--no-log\" a la vez, el comportamiento está indefinido.\n");
}


static void process_args(struct arguments args) {
    int i;
    char *current_arg;

    /* Inicializar los valores de puertos, IP y log a sus valores por defecto */
    *args.own_port = DEFAULT_OWN_PORT;
    *args.receiver_port = DEFAULT_RECEIVER_PORT;
    *args.receiver_ip = DEFAULT_RECEIVER_IP;
    *args.log_file = DEFAULT_LOG_FILE;

    for (i = 1; i < args.argc; i++) { /* Procesamos los argumentos (sin contar el nombre del ejecutable) */
        current_arg = args.argv[i];
        if (current_arg[0] == '-') { /* Flag de opción */
            /* Manejar las opciones largas */
            if (current_arg[1] == '-') { /* Opción larga */
                if (!strcmp(current_arg, "--IP") || !strcmp(current_arg, "--ip"))
                { current_arg = "-i"; }
                if (!strcmp(current_arg, "--port"))
                { current_arg = "-p"; }
                else if (!strcmp(current_arg, "--remote"))
                { current_arg = "-r"; }
                else if (!strcmp(current_arg, "--log"))
                { current_arg = "-l"; }
                else if (!strcmp(current_arg, "--no-log"))
                { current_arg = "-n"; }
                else if (!strcmp(current_arg, "--help"))
                { current_arg = "-h"; }
            }
            switch (current_arg[1]) {
                case 'p':   /* Puerto propio */
                    if (++i < args.argc) {
                        *args.own_port = atoi(args.argv[i]);
                        if (*args.own_port < 0) {
                            fprintf(stderr, "El valor de puerto propio especificado (%s) no es válido.\n\n", args.argv[i]);
                            print_help(args.argv[0]);
                            exit(EXIT_FAILURE);
                        }
                    } else {
                        fprintf(stderr, "Puerto no especificado tras la opción '-p'.\n\n");
                        print_help(args.argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    break;
                case 'I':   /* IP */
                case 'i':
                    if (++i < args.argc) {
                        if (!strcmp(args.argv[i], "localhost")) args.argv[i] = "127.0.0.1";  /* Permitir al usuario indicar localhost como IP */
                        *args.receiver_ip = args.argv[i];
                    } else {
                        fprintf(stderr, "IP no especificada tras la opción '-i'.\n\n");
                        print_help(args.argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    break;
                case 'r':   /* Puerto remoto */
                    if (++i < args.argc) {
                        *args.receiver_port = atoi(args.argv[i]);
                        if (*args.receiver_port < 0) {
                            fprintf(stderr, "El valor de puerto remoto especificado (%s) no es válido.\n\n", args.argv[i]);
                            print_help(args.argv[0]);
                            exit(EXIT_FAILURE);
                        }
                    } else {
                        fprintf(stderr, "Puerto no especificado tras la opción '-r'.\n\n");
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
        } else if (i == 1) {    /* Se especificó el puerto propio como primer argumento */
            *args.own_port = atoi(args.argv[i]);
            if (*args.own_port < 0) {
                fprintf(stderr, "El valor de puerto propio especificado como primer argumento (%s) no es válido.\n\n", args.argv[i]);
                print_help(args.argv[0]);
                exit(EXIT_FAILURE);
            }
        } else if (i == 2) {    /* Se especificó la IP del destinatario como segundo argumento */
            if (!strcmp(args.argv[i], "localhost")) args.argv[i] = "127.0.0.1";  /* Permitir al usuario indicar localhost como IP */
            *args.receiver_ip = args.argv[i];
        } else if (i == 3) {    /* Se especificón el puerto del destinatario como tercer argumento */
            *args.receiver_port = atoi(args.argv[i]);
            if (*args.receiver_port < 0) {
                fprintf(stderr, "El valor de puerto remoto especificado como tercer argumento (%s) no es válido.\n\n", args.argv[i]);
                print_help(args.argv[0]);
                exit(EXIT_FAILURE);
            }
        }
    }
}

