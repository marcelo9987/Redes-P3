#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#include "host.h"
#include "loging.h"


#define DEFAULT_MAX_BYTES_RECV 2048
#define DEFAULT_RECEIVER_PORT 8200
#define DEFAULT_LOG_FILE "receptor.log"


/**
 * Estructura de datos para pasar a la función process_args.
 * Debe contener siempre los campos int argc, char **argv, provenientes de main,
 * y luego una cantidad variable de punteros a las variables que se quieran inicializar
 * a partir de la entrada del programa.
 */
struct Arguments {
    uint16_t receiver_port;
    size_t max_bytes_to_read;
    char *logfile;
};

/**
 * Enumeración para manejar de forma más limpia las distintas opciones del programa.
 */
enum Option {
    OPT_NO_OPTION = '~',
    OPT_OPTION_FLAG = '-',
    OPT_RECEIVER_PORT = 'p',
    OPT_MAX_BYTES_TO_READ = 'b',
    OPT_LOG_FILE_NAME = 'l',
    OPT_NO_LOG = 'n',
    OPT_HELP = 'h'
};


/**
 * @brief   Procesa los argumentos del main.
 *
 * Procesa los argumentos proporcionados al programa por línea de comandos,
 * e inicializa las variables del programa necesarias acorde a estos.
 *
 * @param args  Estructura con los argumentos del programa y punteros a las
 *              variables que necesitan inicialización.
 * @param argc  Número de argumentos que recibe la función principal del programa.
 * @param argv  Lista con los argumentos de la función principal del programa.
 */
static void process_args(struct Arguments *args, int argc, char **argv);

/**
 * @brief   Imprime la ayuda del programa.
 *
 * @param exe_name  Nombre del ejecutable (argv[0]).
 */
static void print_help(char *exe_name);

/**
 * @brief   Obtiene un puerto de los argumentos del programa.
 *
 * Función auxiliar para convertir un argumento del programa en un número de puerto.
 *
 * @param argv  Lista con los argumentos del programa.
 * @param pos   Posición en argv en la que se encuentra la string que se quiere interpretar como puerto.
 *
 * @return  Número de puerto leído de los argumentos del programa; falla si el número de puerto no es válido.
 */
static uint16_t getPortOrFail(char **argv, int pos);

/**
 * @brief   Obtiene el número máximo de bytes a leer de los argumentos del programa.
 *
 * Función auxiliar para convertir un argumento del programa en el número de bytes máximo a recibir.
 *
 * @param argv  Lista con los argumentos del programa.
 * @param pos   Posición en argv en la que se encuentra la string que se quiere
 *              interpretar como máximo número de bytes a recibir.
 *
 * @return  Máximo número de bytes a recibir, leído de los argumentos del programa; falla si el número no es válido.
 */
static size_t getMaxBytesToReadOrFail(char **argv, int pos);


/**
 * @brief   Maneja el mensaje recibido por el receptor.
 *
 * Hace que el host escuche por su socket asociado por un mensaje.
 * Cuando lo recibe, lo imprime y muestra también desde qué IP y puerto se envió.
 *
 * @param local_receiver  Host que escuche por el mensaje.
 * @param max_bytes_to_read Número de bytes máximo que aceptar en el mensaje.
 *
 * @return  Número de bytes recibidos.
 */
static ssize_t handle_message(Host *local_receiver, size_t max_bytes_to_read);


int main(int argc, char **argv) {
    Host local_receiver;
    ssize_t received_bytes = 0;

    /* Inicializamos los parámetros a sus valores por defecto */
    struct Arguments args = {
            .receiver_port = DEFAULT_RECEIVER_PORT,
            .max_bytes_to_read = DEFAULT_MAX_BYTES_RECV,
            .logfile = DEFAULT_LOG_FILE
    };

    set_colors();

    /* Recogemos los parámetros recibidos en la línea de comandos */
    process_args(&args, argc, argv);

    local_receiver = create_own_host(AF_INET, SOCK_DGRAM, 0, args.receiver_port, args.logfile);


    log_and_stdout_printf(local_receiver.log, "IPs v4 del receptor     : %s\n", local_receiver.local_ips_v4);
    log_and_stdout_printf(local_receiver.log, "IPs v6 del receptor     : %s\n", local_receiver.local_ips_v6);
    log_and_stdout_printf(local_receiver.log, "Puerto del receptor     : %d UDP\n", local_receiver.port);
    log_and_stdout_printf(local_receiver.log, "IP pública del receptor : %s\n", local_receiver.public_ip);
    log_and_stdout_printf(local_receiver.log, "Máximo de bytes a leer  : %ld (apartado c)\n", args.max_bytes_to_read);

    log_and_stdout_printf(local_receiver.log, "\n==============================\n");

    log_and_stdout_printf(local_receiver.log, "Escuchando en el puerto : %d UDP...\n", local_receiver.port);

    while (!terminate) {

        if (!socket_io_pending) {
            /* Pausamos la ejecución hasta que se reciba una señal de I/O o de terminación */
            pause();
        }

        log_and_stdout_printf(local_receiver.log, "\n==============================\n");
        log_and_stdout_printf(local_receiver.log, "Posible mensaje recibido...\n");

        received_bytes = handle_message(&local_receiver, args.max_bytes_to_read);

        if (received_bytes == -1) {
            /* Falsa alarma, no había mensajes pendientes o se recibió una señal de terminación */
//            log_and_stdout_printf(local_receiver.log, "Falsa alarma, no había mensajes pendientes o se recibió una señal de terminación\n");
            continue;
        }

//        if (received_bytes == args.max_bytes_to_read) {
//            log_and_stdout_printf(local_receiver.log, "Como hemos recibido el máximo de bytes (%ld), es posible que haya más datos pendientes de recibir.\n", received_bytes);
//            log_and_stdout_printf(local_receiver.log, "Volvamos a llamar a recvfrom()...\n\n");
//            continue;
//        }

        terminate = true;  /* Ya hemos recibido un mensaje completo, por lo que podemos terminar */
    };

    log_and_stdout_printf(local_receiver.log, "\n==============================\n");

    printf("\nCerrando el receptor y saliendo...\n");

    close_host(&local_receiver);

    exit(EXIT_SUCCESS);
}


static ssize_t handle_message(Host *local_receiver, size_t max_bytes_to_read) {
    char received_message[max_bytes_to_read + 1];
    struct sockaddr_in remote_connection_info;
    ssize_t received_bytes;
    ssize_t total_received_bytes = 0;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    while (true) {
//    received_bytes = recvfrom(local_receiver->socket, received_message, MAX_BYTES_RECVFROM, 0, (struct sockaddr *) &(remote_connection_info), &addr_len);
        received_bytes = recvfrom(local_receiver->socket, received_message, max_bytes_to_read, 0, (struct sockaddr *) &(remote_connection_info), &addr_len);

        if (received_bytes == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (total_received_bytes > 0) {
                    log_and_stdout_printf(local_receiver->log, "    Falsa alarma, no había mensajes pendientes o se recibió una señal de terminación\n");
                    return total_received_bytes;
                }

                /* Hemos marcado al socket con O_NONBLOCK; no hay mensajes pendientes, así que lo registramos y salimos */
                socket_io_pending--;
                return received_bytes;
            }

            log_printf_err(local_receiver->log, "ERROR: Se produjo un error en la recepción del mensaje\n");
            fail("ERROR: Se produjo un error en la recepción del mensaje");
        }

        log_and_stdout_printf(local_receiver->log, "==============================\n");

        received_message[received_bytes] = '\0';

        log_and_stdout_printf(local_receiver->log, "Mensaje recibido  : \"%s\"\n", received_message);
        log_and_stdout_printf(local_receiver->log, "Bytes recibidos   : %ld\n", received_bytes);
        log_and_stdout_printf(local_receiver->log, "IP del emisor     : %s\n", inet_ntoa(remote_connection_info.sin_addr));
        log_and_stdout_printf(local_receiver->log, "Puerto del emisor : %d UDP\n", ntohs(remote_connection_info.sin_port));

        total_received_bytes += received_bytes;

        if (received_bytes == max_bytes_to_read) {
            log_and_stdout_printf(local_receiver->log, "    Como hemos recibido el máximo de bytes (%ld), es posible que haya más datos pendientes de recibir.\n", received_bytes);
            log_and_stdout_printf(local_receiver->log, "    Volvamos a llamar (por si acaso) de nuevo a recvfrom()...\n\n");
            continue;
        } else {
            socket_io_pending--;
            break;
        }

    }

    return total_received_bytes;
}


static void print_help(char *exe_name) {
    printf("\n");

    /** Cabecera y modo de ejecución **/
    printf("Modo de uso: %s [...opciones]\n\n", exe_name);
    printf("     o bien: %s <puerto> [...opciones]\n\n", exe_name);
    printf("     o bien: %s -p <puerto> [...opciones]\n\n", exe_name);

    printf("\n");

    printf("Ejemplos de uso: Las tres siguientes ejecuciones son equivalentes:\n");
    printf("  $ %s # Tomará los parámetros por defecto\n", exe_name);
    printf("  $ %s %d\n", exe_name, DEFAULT_RECEIVER_PORT);
    printf("  $ %s -p %d\n", exe_name, DEFAULT_RECEIVER_PORT);

    /** Lista de opciones de uso **/
    printf("\n");

    /** Lista de parámetros "importantes" **/
    printf("Parámetros \tParámetro largo \tPor defecto \tDescripción\n");

    printf("  -p <puerto>\t--puerto <puerto>\t%d\t\tPuerto en el que se espera recibir el mensaje.\n", DEFAULT_RECEIVER_PORT);
    printf("  -b <bytes>\t--max-bytes <bytes>\t%d\t\tBytes máximos a leer por recvfrom (para el apartado c).\n", DEFAULT_MAX_BYTES_RECV);

    printf("\n");

    /** Lista de opciones de uso **/
    printf("Más opciones \tOpción larga \t\tPor defecto \tDescripción\n");

    printf("  -l <log>\t--log <log>\t\t\"%s\" \tNombre del archivo en el que guardar el registro de actividad del receptor.\n", DEFAULT_LOG_FILE);
    printf("  -l <log>\t--log <log>\t\t\"%s\" \tNombre del archivo en el que guardar el registro de actividad del receptor.\n", DEFAULT_LOG_FILE);
    printf("  -n\t\t--no-log\t\t\t\tNo crear archivo de registro de actividad.\n");
    printf("  -h\t\t--help\t\t\t\t\tMostrar este texto de ayuda y salir.\n");

//  printf("\n");
//
//  /** Consideraciones adicionales **/
//  printf("    - Puede especificarse el parámetro <port> para el puerto en los que escucha el receptor sin escribir la opción '-p', siempre y cuando esta sea el primer parámetro que se pasa a la función.\n");
//
//  printf("    - Si no se especifica alguno de los argumentos, el emisor se ejecutará con su valor por defecto, a saber:\n");
//  printf("\tDEFAULT_RECEIVER_PORT: %u\n", DEFAULT_RECEIVER_PORT);
//  printf("\tDEFAULT_LOG_FILE:      %s\n", DEFAULT_LOG_FILE);
//
//  printf("    - Si se especifica varias veces un argumento, o se especifican las opciones \"--log\" y \"--no-log\" a la vez, el comportamiento está indefinido.\n");

    printf("\n");
}


static uint16_t getPortOrFail(char **argv, int pos) {
    long read_number = atol(argv[pos]);

    if (read_number <= 0 || read_number > 65535) {
        fprintf(stderr, "ERROR: El valor de puerto especificado (%s) no es válido\n", argv[pos]);
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    return read_number;
}

static size_t getMaxBytesToReadOrFail(char **argv, int pos) {
    size_t read_number = atol(argv[pos]);

    if (read_number <= 0) {
        fprintf(stderr, "ERROR: El valor (para el apartado c) de bytes máximos a leer especificado (%s) no es válido\n", argv[pos]);
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    return read_number;
}


static void process_args(struct Arguments *args, int argc, char **argv) {
    bool allow_unnamed_basic_params = true;

    enum Option next_unnamed_basic_param = OPT_RECEIVER_PORT; // 'p'

    /* Procesamos los argumentos (sin contar el nombre del ejecutable) */
    for (int pos = 1; pos < argc; pos++) {
        enum Option current_option = OPT_NO_OPTION;

        char *current_arg_str = argv[pos];
        if (current_arg_str[0] == OPT_OPTION_FLAG) { // '-'
            allow_unnamed_basic_params = false;

            /* Flag de opción */
            current_option = (enum Option) current_arg_str[1];

            /* Manejar las opciones largas */
            if (current_option == OPT_OPTION_FLAG) { // '-'
                if (!strcmp(current_arg_str, "--puerto")) {
                    current_option = OPT_RECEIVER_PORT; // 'p'
                }
                if (!strcmp(current_arg_str, "--max-bytes")) {
                    current_option = OPT_MAX_BYTES_TO_READ; // 'b'
                } else if (!strcmp(current_arg_str, "--log")) {
                    current_option = OPT_LOG_FILE_NAME; // 'l'
                } else if (!strcmp(current_arg_str, "--no-log")) {
                    current_option = OPT_NO_LOG; // 'n'
                } else if (!strcmp(current_arg_str, "--help")) {
                    current_option = OPT_HELP; // 'h'
                }
            }
        } else if (allow_unnamed_basic_params) {
            switch (next_unnamed_basic_param) {
                case OPT_RECEIVER_PORT: // 'p'
                    current_option = OPT_RECEIVER_PORT; // 'p'
                    next_unnamed_basic_param = OPT_NO_OPTION;
                    --pos;
                    break;
                default:
                    fprintf(stderr, "ERROR: Se ha recibido un parámetro (%s) no esperado\n", argv[pos]);
                    print_help(argv[0]);
                    exit(EXIT_FAILURE);
            }
        }

//        printf("current_option: %c \"%s\"\n", current_option, argv[pos+1]);

        switch (current_option) {
            case OPT_RECEIVER_PORT: // 'p' /* Puerto Receptor */
                if (++pos < argc) {
                    args->receiver_port = getPortOrFail(argv, pos);
                } else {
                    fprintf(stderr, "ERROR: Puerto no especificado tras la opción '-p'\n");
                    print_help(argv[0]);
                    exit(EXIT_FAILURE);
                }
                break;

            case OPT_MAX_BYTES_TO_READ: // 'b' /* Limitación del número de bytes a leer (Para el apartado c) */
                if (++pos < argc) {
                    args->max_bytes_to_read = getMaxBytesToReadOrFail(argv, pos);
                } else {
                    fprintf(stderr, "ERROR: Número máximo de bytes a leer (para el apartado c) no especificado tras la opción '-b'\n");
                    print_help(argv[0]);
                    exit(EXIT_FAILURE);
                }
                break;

            case OPT_LOG_FILE_NAME: // 'l' /* Log */
                if (++pos < argc) {
                    args->logfile = argv[pos];
                } else {
                    fprintf(stderr, "ERROR: Nombre del log no especificado tras la opción '-l'\n");
                    print_help(argv[0]);
                    exit(EXIT_FAILURE);
                }
                break;

            case OPT_NO_LOG: // 'n' /* No-log */
                args->logfile = NULL;
                break;

            case OPT_HELP: // 'h' /* Ayuda */
                print_help(argv[0]);
                exit(EXIT_SUCCESS);

            default:
                fprintf(stderr, "ERROR: Opción '%s' desconocida\n", current_arg_str);
                print_help(argv[0]);
                exit(EXIT_FAILURE);
        }

    }

}
