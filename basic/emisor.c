#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include "host.h"
#include "loging.h"

#define DEFAULT_MAX_BYTES_SENT 1000
#define DEFAULT_SENDER_PORT 8100
#define IP_LOCALHOST "127.0.0.1"
#define DEFAULT_RECEIVER_IP IP_LOCALHOST
#define DEFAULT_RECEIVER_PORT 8200

#define DEFAULT_LOG_FILE "emisor.log"

/**
 * Estructura de datos para pasar a la función process_args.
 * Contiene una cantidad variable de variables que se quieran inicializar
 * a partir de la entrada del programa.
 */
struct Arguments {
    uint16_t local_port;
    char *remote_ip;
    uint16_t remote_port;
    char *logfile;
    size_t max_bytes_to_send;
};

/**
 * Estrucutra enumerada para manejar de forma más limpia las distintas opciones del programa.
 */
enum Option {
    OPT_NO_OPTION = '~',
    OPT_OPTION_FLAG = '-',
    OPT_SOURCE_PORT = 'o',
    OPT_RECEIVER_IP = 'i',
    OPT_RECEIVER_PORT = 'p',
    OPT_MAX_BYTES_TO_SEND = 'b',
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
static void process_args(struct Arguments *args, int argc, char** argv);

/**
 * @brief   Imprime la ayuda del programa
 *
 * @param exe_name  Nombre del ejecutable (argv[0])
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
static uint16_t getPortOrFail(char** argv, int pos);

/**
 * @brief   Obtiene el número máximo de bytes a enviar de los argumentos del programa.
 *
 * Función auxiliar para convertir un argumento del programa en el número de bytes máximo a enviar.
 *
 * @param argv  Lista con los argumentos del programa.
 * @param pos   Posición en argv en la que se encuentra la string que se quiere 
 *              interpretar como máximo número de bytes a enviar.
 *
 * @return  Máximo número de bytes a enviar, leído de los argumentos del programa; falla si el número no es válido.
 */
static size_t getMaxBytesToSendOrFail(char** argv, int pos);

/**
 * @brief   Envía un mensaje a un host remoto.
 *
 * Hace que el host sender envíe un mensaje con floats aleatorios al host remote.
 *
 * @param self      Host que envía el mensaje.
 * @param remote    Host al que enviar el mensaje.
 * @param max_bytes_to_send     Número máximo de bytes a enviar.
 */
static void send_message(Host *sender, Host *remote, size_t max_bytes_to_send);


int main(int argc, char** argv) {
    Host sender, remote;
    struct Arguments args = {
            .local_port = DEFAULT_SENDER_PORT,
            .remote_ip = DEFAULT_RECEIVER_IP,
            .remote_port = DEFAULT_RECEIVER_PORT,
            .logfile = DEFAULT_LOG_FILE,
            .max_bytes_to_send = DEFAULT_MAX_BYTES_SENT
    };

    set_colors();

    /* Inicializar los parámetros a sus valores por defecto */
    process_args(&args, argc, argv);

    sender = create_own_host(AF_INET, SOCK_DGRAM, 0, args.local_port, args.logfile);

    remote = create_remote_host(AF_INET, SOCK_DGRAM, 0, args.remote_ip, args.remote_port);

    send_message(&sender, &remote, args.max_bytes_to_send);

    close_host(&sender);
    close_host(&remote);

    return 0;
}


static void send_message(Host *sender, Host *remote, size_t max_bytes_to_send) {
    float message_to_send[max_bytes_to_send / sizeof(float)];
    ssize_t sent_bytes;
    int i;

    log_and_stdout_printf(sender->log, "Puerto del emisor   : %d UDP\n", sender->port);
    log_and_stdout_printf(sender->log, "IP     del receptor : %s\n", remote->ip);
    log_and_stdout_printf(sender->log, "Puerto del receptor : %d UDP\n", remote->port);

    for (i = 0; i < max_bytes_to_send / sizeof(float) + 1; i++) {
        message_to_send[i] = drand48();
    }

    // Enviamos el mensaje al cliente
    sent_bytes = sendto(sender->socket, message_to_send, max_bytes_to_send, /*__flags*/ 0, (struct sockaddr *) &remote->address, sizeof(remote->address));

    if (sent_bytes == -1) {
        log_printf_err(sender->log, "ERROR: Se produjo un error cuando se intentaba enviar el mensaje\n");
        fail("ERROR: Se produjo un error cuando se intentaba enviar el mensaje");
    }

    log_and_stdout_printf(sender->log, "Mensaje enviado     : ");
    for (i = 0; i < max_bytes_to_send / sizeof(float); i++) {
        printf("%f; ", message_to_send[i]);
    }
    printf("\b\b  \n");
    log_and_stdout_printf(sender->log, "Bytes enviados      : %ld\n", sent_bytes);
}



static void print_help(char *exe_name) {
    printf("\n");

    /** Cabecera y modo de ejecución **/
    printf("Modo de uso: %s [...opciones]\n", exe_name);
    printf("     o bien: %s <origen> [ <ip> [<puerto>] ] [...opciones]\n", exe_name);
    printf("     o bien: %s [-o <origen>] [-i <ip>] [-p <puerto>] [...opciones]\n", exe_name);

    printf("\n");

    printf("Ejemplos de uso: Las tres siguientes ejecuciones son equivalentes:\n");
    printf("  $ %s # Tomará los parámetros por defecto\n", exe_name);
    printf("  $ %s %d %s %d\n", exe_name, DEFAULT_SENDER_PORT, DEFAULT_RECEIVER_IP, DEFAULT_RECEIVER_PORT);
    printf("  $ %s -o %d -i %s -p %d\n", exe_name, DEFAULT_SENDER_PORT, DEFAULT_RECEIVER_IP, DEFAULT_RECEIVER_PORT);

    printf("\n");

    /** Lista de parámetros "importantes" **/
    printf("Parámetros \tParámetro largo \tPor defecto \tDescripción\n");

    printf("  -o <origen>\t--origen <puerto_org> \t%d \t\tPuerto desde donde se enviará el mensaje.\n", DEFAULT_SENDER_PORT);
    printf("  -i <ip>\t--ip <ip_dest>\t\t%s \tIP del receptor del mensaje.\n", DEFAULT_RECEIVER_IP);
    printf("  -p <puerto>\t--puerto <puerto_dest>\t%d \t\tPuerto del receptor al que se enviará el mensaje.\n", DEFAULT_RECEIVER_PORT);
    printf("  -b <bytes>\t--max-bytes <bytes>\t%d\t\tBytes máximos a enviar con sendto (para el apartado d).\n", DEFAULT_MAX_BYTES_SENT);

    printf("\n");

    /** Lista de opciones de uso **/
    printf("Más opciones \tOpción larga \t\tPor defecto \tDescripción\n");

    printf("  -l <log>\t--log <log>\t\t\"%s\" \tNombre del archivo en el que guardar el registro de actividad del emisor.\n", DEFAULT_LOG_FILE);
    printf("  -n\t\t--no-log\t\t\t\tNo crear archivo de registro de actividad.\n");
    printf("  -h\t\t--help\t\t\t\t\tMostrar este texto de ayuda y salir.\n");

//  printf("\n");
//
//  /** Consideraciones adicionales **/
//  printf("\n\nNota: Si se especifica varias veces un argumento, o se especifican las opciones \"--log\" y \"--no-log\" a la vez, el comportamiento está indefinido.\n\n");

    printf("\n");
}

static uint16_t getPortOrFail(char** argv, int pos) {
    long read_number = atoi(argv[pos]);

    if (read_number <= 0 || read_number > 65535) {
        fprintf(stderr, "ERROR: El valor de puerto especificado (%s) no es válido\n", argv[pos]);
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    return read_number;
}

static size_t getMaxBytesToSendOrFail(char** argv, int pos) {
    size_t send_number = atol(argv[pos]);

    if (send_number <= 0) {
        fprintf(stderr, "ERROR: El valor (para el apartado d) de bytes máximos a enviar especificado (%s) no es válido\n", argv[pos]);
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    return send_number;
}

static void process_args(struct Arguments *args, int argc, char** argv) {
    bool allow_unnamed_basic_params = true;

    enum Option next_unnamed_basic_param = OPT_SOURCE_PORT; // 'o'

    for (int pos = 1; pos < argc; pos++) {
        enum Option current_option = OPT_NO_OPTION;

        /* Procesamos los argumentos (sin contar el nombre del ejecutable) */
        char *current_arg_str = argv[pos];
        if (current_arg_str[0] == OPT_OPTION_FLAG) { // '-'
            allow_unnamed_basic_params = false;

            /* Flag de opción */
            current_option = (enum Option) current_arg_str[1];

            /* Manejar las opciones largas */
            if (current_option == OPT_OPTION_FLAG) { // '-'
                if (!strcmp(current_arg_str, "--origen"))
                {
                    current_option = OPT_SOURCE_PORT; // 'o'
                } else if (!strcmp(current_arg_str, "--ip"))
                {
                    current_option = OPT_RECEIVER_IP; // 'i'
                } else if (!strcmp(current_arg_str, "--puerto"))
                {
                    current_option = OPT_RECEIVER_PORT; // 'p'
                } else if (!strcmp(current_arg_str, "--max-bytes"))
                {
                    current_option = OPT_MAX_BYTES_TO_SEND; // 'b'
                }
                else if (!strcmp(current_arg_str, "--log"))
                {
                    current_option = OPT_LOG_FILE_NAME; // 'l'
                } else if (!strcmp(current_arg_str, "--no-log"))
                {
                    current_option = OPT_NO_LOG; // 'n'
                } else if (!strcmp(current_arg_str, "--help"))
                {
                    current_option = OPT_HELP; // 'h'
                }
            }
        } else if (allow_unnamed_basic_params) {
            switch (next_unnamed_basic_param) {
                case OPT_SOURCE_PORT: // 'o'
                    current_option = OPT_SOURCE_PORT; // 'o'
                    next_unnamed_basic_param = OPT_RECEIVER_IP;
                    --pos;
                    break;
                case OPT_RECEIVER_IP: // 'i'
                    current_option = OPT_RECEIVER_IP; // 'i'
                    next_unnamed_basic_param = OPT_RECEIVER_PORT;
                    --pos;
                    break;
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
            case OPT_SOURCE_PORT: // 'o' /* Puerto Emisor */
                if (++pos < argc) {
                    args->local_port = getPortOrFail(argv, pos);
                } else {
                    fprintf(stderr, "ERROR: Puerto no especificado tras la opción '-o'\n");
                    print_help(argv[0]);
                    exit(EXIT_FAILURE);
                }
                break;

            case OPT_RECEIVER_IP: // 'i' /* IP Receptor */
                if (++pos < argc) {
                    if (!strcmp(argv[pos], "localhost")) {
                        // Permitimos al cliente indicar localhost como IP
                        args->remote_ip = IP_LOCALHOST;
                    } else {
                        args->remote_ip = argv[pos];
                    }
                }
                break;

            case OPT_RECEIVER_PORT: // 'p' /* Puerto Receptor */
                if (++pos < argc) {
                    args->remote_port = getPortOrFail(argv, pos);
                } else {
                    fprintf(stderr, "ERROR: Puerto no especificado tras la opción '-p'\n");
                    print_help(argv[0]);
                    exit(EXIT_FAILURE);
                }
                break;

            case OPT_MAX_BYTES_TO_SEND: // 'b' /* Limitación del número de bytes a mandar (Para el apartado d) */
                if (++pos < argc) {
                    args->max_bytes_to_send = getMaxBytesToSendOrFail(argv, pos);
                } else {
                    fprintf(stderr, "ERROR: Número máximo de bytes a mandar (para el apartado d) no especificado tras la opción '-b'\n");
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
