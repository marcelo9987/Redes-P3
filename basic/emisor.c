#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include "host.h"
#include "loging.h"

#define MAX_MESSAGE_SIZE 2048

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
};

/**
 * Enumeración para manejar de forma más limpia las distintas opciones del programa.
 */
enum Option {
    OPT_NO_OPTION = '~',
    OPT_OPTION_FLAG = '-',
    OPT_SOURCE_PORT = 'o',
    OPT_RECEIVER_IP = 'i',
    OPT_RECEIVER_PORT = 'p',
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
static uint16_t getPortOrFail(char **argv, int pos);

/**
 * @brief   Envía un mensaje a un host remoto.
 *
 * Hace que el host local_sender envíe un mensaje de saludo al host remote_receiver.
 *
 * @param self      Host que envía el mensaje.
 * @param remote_receiver    Host al que enviar el mensaje.
 */
static void send_message(Host *local_sender, Host *remote_receiver);


int main(int argc, char **argv) {
    Host local_sender, remote_receiver;

    /* Inicializamos los parámetros a sus valores por defecto */
    struct Arguments args = {
            .local_port = DEFAULT_SENDER_PORT,
            .remote_ip = DEFAULT_RECEIVER_IP,
            .remote_port = DEFAULT_RECEIVER_PORT,
            .logfile = DEFAULT_LOG_FILE
    };

    set_colors();

    /* Recogemos los parámetros recibidos en la línea de comandos */
    process_args(&args, argc, argv);

    local_sender = create_own_host(AF_INET, SOCK_DGRAM, 0, args.local_port, args.logfile);

    remote_receiver = create_remote_host(AF_INET, SOCK_DGRAM, 0, args.remote_ip, args.remote_port);

    send_message(&local_sender, &remote_receiver);

    close_host(&local_sender);
    close_host(&remote_receiver);

    return 0;
}


static void send_message(Host *local_sender, Host *remote_receiver) {
    char message_to_send[MAX_MESSAGE_SIZE];
    ssize_t sent_bytes;

    log_and_stdout_printf(local_sender->log, "IPs v4 del emisor     : %s\n", local_sender->local_ips_v4);
    log_and_stdout_printf(local_sender->log, "IPs v6 del emisor     : %s\n", local_sender->local_ips_v6);
    log_and_stdout_printf(local_sender->log, "Puerto del emisor     : %d UDP\n", local_sender->port);
    log_and_stdout_printf(local_sender->log, "IP pública del emisor : %s\n", local_sender->public_ip);

    log_and_stdout_printf(local_sender->log, "---------------------\n");

    log_and_stdout_printf(local_sender->log, "IP del receptor       : %s\n", inet_ntoa(remote_receiver->address.sin_addr));
    log_and_stdout_printf(local_sender->log, "Puerto del receptor   : %d UDP\n", ntohs(remote_receiver->address.sin_port));

//    sprintf(message_to_send, "El host %s en %s:%u te saluda.", local_sender->hostname, local_sender->ip, local_sender->port);
    sprintf(message_to_send, "El host %s en %s:%u (%s) te saluda.", local_sender->hostname, local_sender->local_ips_v4, local_sender->port, local_sender->public_ip);

    // Enviamos el mensaje al cliente
    sent_bytes = sendto(local_sender->socket, message_to_send, strlen(message_to_send), /*__flags*/ 0, (struct sockaddr *) &remote_receiver->address, sizeof(remote_receiver->address));

    if (sent_bytes == -1) {
        log_printf_err(local_sender->log, "ERROR: Se produjo un error cuando se intentaba enviar el mensaje\n");
        fail("ERROR: Se produjo un error cuando se intentaba enviar el mensaje");
    }

    log_and_stdout_printf(local_sender->log, "Mensaje enviado       : \"%s\"\n", message_to_send);
    log_and_stdout_printf(local_sender->log, "Bytes enviados        : %ld\n", sent_bytes);
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

static uint16_t getPortOrFail(char **argv, int pos) {
    long read_number = atol(argv[pos]);

    if (read_number <= 0 || read_number > 65535) {
        fprintf(stderr, "ERROR: El valor de puerto especificado (%s) no es válido\n", argv[pos]);
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    return read_number;
}


static void process_args(struct Arguments *args, int argc, char **argv) {
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
                if (!strcmp(current_arg_str, "--origen")) {
                    current_option = OPT_SOURCE_PORT; // 'o'
                } else if (!strcmp(current_arg_str, "--ip")) {
                    current_option = OPT_RECEIVER_IP; // 'i'
                } else if (!strcmp(current_arg_str, "--puerto")) {
                    current_option = OPT_RECEIVER_PORT; // 'p'
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
