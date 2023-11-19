#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <wctype.h>
#include <locale.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include "host.h"
#include "loging.h"


#define DEFAULT_MAX_BYTES_RECV 2048
#define DEFAULT_SERVER_PORT 9200
#define DEFAULT_LOG_FILE "servidorUDP.log"

/**
 * Estructura de datos para pasar a la función process_args.
 * Contiene una cantidad variable de variables que se quieran inicializar
 * a partir de la entrada del programa.
 */
struct Arguments {
    uint16_t server_port;
    char *logfile;
};

/**
 * Estrucutra enumerada para manejar de forma más limpia las distintas opciones del programa.
 */
enum Option {
    OPT_NO_OPTION = '~',
    OPT_OPTION_FLAG = '-',
    OPT_SERVER_PORT = 'p',
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
 * @brief   Transforma una string a mayúsculas
 *
 * Transforma la string source a mayúsculas, utilizando para ello wstrings para poder transformar
 * caracteres especiales que ocupen más de un byte. Por tanto, permite pasar a mayúsculas strings en
 * el idioma definido en locale.
 *
 * @param source    String fuente a transformar en mayúsculas.
 *
 * @return  String dinámicamente alojada (por tanto, debe liberarse con un free) que contiene los mismos
 *          caracteres que source pero en mayúsculas.
 */
static char *toupper_string(const char *source);

/**
 * @brief   Maneja los mensajes desde el lado del servidor.
 *
 * Recibe una string de un cliente, la pasa a mayúsculas y se la reenvía.
 *
 * @param server    Servidor que maneja la conexión.
 */
void handle_message(Host *server);



int main(int argc, char** argv) {
    Host server;
    struct Arguments args = {
            .server_port = DEFAULT_SERVER_PORT,
            .logfile = DEFAULT_LOG_FILE
    };


    if (!setlocale(LC_ALL, "")) fail("ERROR: No se pudo establecer la locale del programa");   /* Establecer la locale según las variables de entorno */

    process_args(&args, argc, argv);

    printf("Ejecutando servidor de mayúsculas con parámetros: PUERTO=%u, LOG=%s\n", args.server_port, args.logfile);
    server = create_own_host(AF_INET, SOCK_DGRAM, 0, args.server_port, args.logfile);

    while (!terminate) {
        printf("\nEsperando mensaje...\n");
        if (!socket_io_pending) pause();    /* Pausamos la ejecución hasta que se reciba una señal de I/O o de terminación */
        handle_message(&server);
    }

    printf("\nCerrando el servidor y saliendo...\n");
    close_host(&server);

    exit(EXIT_SUCCESS);
}


void handle_message(Host *server) {
    struct sockaddr_in client_address; 
    char input[DEFAULT_MAX_BYTES_RECV];
    char* output;
    ssize_t recv_bytes, sent_bytes;
    socklen_t client_addr_size = sizeof(struct sockaddr_in);

    recv_bytes = recvfrom(server->socket, input, DEFAULT_MAX_BYTES_RECV, 0, (struct sockaddr *) &client_address, &client_addr_size);
    if (recv_bytes == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {  /* Hemos marcado al socket con O_NONBLOCK; no hay conexiones pendientes, así que lo registramos y salimos */
            socket_io_pending = 0;
            return;
        }
        fail("ERROR: Error al recibir la línea de texto");
    }

    printf("Paquete recibido de %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
    log_printf(server->log, "Paquete recibido de %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

    if (!recv_bytes) return;    /* Se recibió una orden de cerrar la conexión */

    printf("\tMensaje recibido: %s\n", input);

    output = toupper_string(input);

    sent_bytes = sendto(server->socket, output, strlen(output) + 1, 0, (struct sockaddr *) &client_address, client_addr_size);
    if (sent_bytes  < 0) {
        if (output) free(output);
        log_printf_err(server->log, "Error al enviar línea de texto al cliente.\n");
        fail("ERROR: Error al enviar la línea de texto al cliente");
    }

    printf("Enviado: %s\n", output);

    if (output) free(output);

    socket_io_pending--;
}


static char *toupper_string(const char *source) {
    wchar_t *wide_source;
    wchar_t *wide_destination;
    ssize_t wide_size, size;

    wide_size = mbstowcs(NULL, source, 0); /* Calcular el número de wchar_t que ocupa el string source */

    /* Alojar espacio para wide_source y wide_destiny */
    wide_source = (wchar_t *) calloc(wide_size + 1, sizeof(wchar_t));
    wide_destination = (wchar_t *) calloc(wide_size + 1, sizeof(wchar_t));

    /* Transformar la fuente en un wstring */
    mbstowcs(wide_source, source, wide_size + 1);

    /* Transformar wide_source a mayúsculas y guardarlo en wide_destiny */
    for (int i = 0; wide_source[i]; i++) {
        wide_destination[i] = towupper(wide_source[i]);
    }

    /* Transoformar de vuelta a un string normal */
    size = wcstombs(NULL, wide_destination, 0); /* Calcular el número de char que ocupa el wstring wide_destiny */

    char *destination = (char *) calloc(size + 1, sizeof(char));
    wcstombs(destination, wide_destination, size + 1);

    if (wide_source) free(wide_source);

    if (wide_destination) free(wide_destination);

    return destination;
}



static void print_help(char *exe_name) {
    /** Cabecera y modo de ejecución **/
    printf("Uso: %s [[-p] <puerto>] [-l <log> | --no-log] [-h]\n\n", exe_name);

    /** Lista de opciones de uso **/
    printf(" Opción\t\tOpción larga\t\tSignificado\n");

    printf(" -p <puerto>\t--puerto <puerto>\t\tPuerto en el que escuchará el servidor.\n");

    printf(" -l <log>\t--log <log>\t\tNombre del archivo en el que guardar el registro de actividad del servidor.\n");
    printf(" -n\t\t--no-log\t\tNo crear archivo de registro de actividad.\n");
    printf(" -h\t\t--help\t\t\tMostrar este texto de ayuda y salir.\n");

    /** Consideraciones adicionales **/
    printf("\nPuede especificarse el parámetro <puerto> para el puerto en el que escucha el servidor sin escribir la opción '-p', siempre y cuando este sea el primer parámetro que se pasa a la función.\n");
    printf("\nSi no se especifica alguno de los argumentos, el servidor se ejecutará con su valor por defecto, a saber: DEFAULT_PORT=%u; DEFAULT_LOG=%s\n", DEFAULT_SERVER_PORT, DEFAULT_LOG_FILE);
    printf("\nSi se especifica varias veces un argumento, o se especifican las opciones \"--log\" y \"--no-log\" a la vez, el comportamiento está indefinido.\n");
}


static uint16_t getPortOrFail(char** argv, int pos) {
    uint16_t read_number = atoi(argv[pos]);

    if (read_number <= 0 || read_number > 65535) {
        fprintf(stderr, "ERROR: El valor de puerto especificado (%s) no es válido\n", argv[pos]);
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    return read_number;
}


static void process_args(struct Arguments *args, int argc, char** argv) {
    char* current_arg_str;

    /* Procesamos los argumentos (sin contar el nombre del ejecutable) */
    for (int pos = 1; pos < argc; pos++)
    {
        current_arg_str = argv[pos];
        if (current_arg_str[0] == OPT_OPTION_FLAG) { /* Flag de opción */
            /* Manejar las opciones largas */
            if (current_arg_str[1] == OPT_OPTION_FLAG) { /* Opción larga */
                if (!strcmp(current_arg_str, "--puerto"))
                {
                    current_arg_str = "-p";
                } else if (!strcmp(current_arg_str, "--log"))
                {
                    current_arg_str = "-l";
                } else if (!strcmp(current_arg_str, "--no-log"))
                {
                    current_arg_str = "-n";
                } else if (!strcmp(current_arg_str, "--help"))
                {
                    current_arg_str = "-h";
                }
            }

            /* Flag de opción */
            enum Option current_option = (enum Option) current_arg_str[1];

            switch (current_option) {
                case OPT_SERVER_PORT: // 'p' /* Puerto del Servidor */
                    if (++pos < argc) {
                        args->server_port = getPortOrFail(argv, pos);
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
                    fprintf(stderr, "ERROR: Opción '%s' desconocida\n\n", current_arg_str);
                    print_help(argv[0]);
                    exit(EXIT_FAILURE);
            }
        } else if (pos == 1) {    /* Se especificó el puerto como primer argumento */
            args->server_port = getPortOrFail(argv, pos);
        }
    }
}
