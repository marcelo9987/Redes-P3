#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <wctype.h>
#include <locale.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include "../host/host.h"
#include "../host/loging.h"

// --


//#define DEFAULT_MAX_BYTES_RECV 2056
#define DEFAULT_MAX_BYTES_RECV 1000

#define DEFAULT_SERVER_PORT 9200

#define DEFAULT_LOG_FILE "servidorUDP.log"

// --

/**
 * Estructura de datos para pasar a la función process_args.
 * Debe contener siempre los campos int argc, char** argv, provenientes de main,
 * y luego una cantidad variable de punteros a las variables que se quieran inicializar
 * a partir de la entrada del programa.
 */
struct Arguments
{
    uint16_t server_port;
    char *logfile;
};

enum Option
{
    OPT_NO_OPTION = '~',
    OPT_OPTION_FLAG = '-',
    OPT_SERVER_PORT = 'p',
//    OPT_MAX_BYTES_TO_READ = 'b',
    OPT_LOG_FILE_NAME = 'l',
    OPT_NO_LOG = 'n',
    OPT_HELP = 'h'
};

// --

/**
 * @brief Imprime la ayuda del programa
 *
 * @param exe_name  Nombre del ejecutable (argv[0])
 */
static void print_help(char *exe_name)
{
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

/***** TODO: comentar **/
long getPortOrFail(char *argv[], int pos)
{
    long read_number = atol(argv[pos]);

    if (read_number <= 0 || read_number > 65535)
    {
        fprintf(stderr, "ERROR: El valor de puerto especificado (%s) no es válido\n", argv[pos]);
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    return read_number;
}

/**
 * @brief   Procesa los argumentos del main
 *
 * Procesa los argumentos proporcionados al programa por línea de comandos,
 * e inicializa las variables del programa necesarias acorde a estos.
 *
 * @param args  Estructura con los argumentos del programa y punteros a las
 *              variables que necesitan inicialización.
 */
static void process_args(struct Arguments *args, int argc, char *argv[])
{

    /* Procesamos los argumentos (sin contar el nombre del ejecutable) */
    for (int pos = 1; pos < argc; pos++)
    {
        char *current_arg_str = argv[pos];
        if (current_arg_str[0] == OPT_OPTION_FLAG)
        { /* Flag de opción */
            /* Manejar las opciones largas */
            if (current_arg_str[1] == OPT_OPTION_FLAG)
            { /* Opción larga */
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

            switch (current_option)
            {
                case OPT_SERVER_PORT: // 'p' /* Puerto del Servidor */
                    if (++pos < argc)
                    {
                        args->server_port = getPortOrFail(argv, pos);
                    } else
                    {
                        fprintf(stderr, "ERROR: Puerto no especificado tras la opción '-p'\n");
                        print_help(argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    break;

                case OPT_LOG_FILE_NAME: // 'l' /* Log */
                    if (++pos < argc)
                    {
                        args->logfile = argv[pos];
                    } else
                    {
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
        } else if (pos == 1)
        {    /* Se especificó el puerto como primer argumento */
            args->server_port = getPortOrFail(argv, pos);
        }
    }
}


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
 *          caractereres que source pero en mayúsculas.
 */

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
static char *toupper_string(const char *source)
{
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
    for (int i = 0; wide_source[i]; i++)
    {
        wide_destination[i] = towupper(wide_source[i]);
    }

    /* Transoformar de vuelta a un string normal */
    size = wcstombs(NULL, wide_destination, 0); /* Calcular el número de char que ocupa el wstring wide_destiny */

    char *destination = (char *) calloc(size + 1, sizeof(char));
    wcstombs(destination, wide_destination, size + 1);

    if (wide_source)
    {
        free(wide_source);
    }

    if (wide_destination)
    {
        free(wide_destination);
    }

    return destination;
}

/**
 * @brief   Maneja la conexión desde el lado del servidor.
 *
 * Recibe strings del cliente, las pasa a mayúsculas y se las reenvía.
 *
 * @param server    Servidor que maneja la conexión.
 */
void handle_connection(Host *server)
{
    Host client; // = create_remote_host(AF_INET, SOCK_DGRAM, 0, NULL, 0);

    char input[DEFAULT_MAX_BYTES_RECV];
//    ssize_t recv_bytes, sent_bytes;

//  printf("\nManejando la conexión del cliente %s:%u...\n", client->ip, client->port);
    printf("\nEsperando mensaje..\n");
//  log_printf("Manejando la conexión del cliente %s:%u...\n", client.ip, client.port); //todo: buscar arreglo

    while (true)
    {
//        if ( (recv_bytes = recv(client.socket, input, MAX_BYTES_RECV, 0)) < 0) fail("ERROR: Error al recibir la línea de texto");
//        socklen_t client_addr_size = sizeof(client->address);
        socklen_t client_addr_size = sizeof(struct sockaddr_in);

        ssize_t recv_bytes = recvfrom(server->socket, input, DEFAULT_MAX_BYTES_RECV, 0, (struct sockaddr *) &client.address, &client_addr_size);
        if (recv_bytes < 0)
        {
            fail("ERROR: Error al recibir la línea de texto");
        }

        printf("[Servidor] Paquete recibido de %s:%d\n", inet_ntoa(client.address.sin_addr), ntohs(client.address.sin_port));

        if (!recv_bytes)
        {  /* Se recibió una orden de cerrar la conexión */

            return;
        }

        printf("\t [Servidor] Mensaje recibido: %s\n", input);

        char *output = toupper_string(input);

//        ssize_t sent_bytes = send(client.socket, output, strlen(output) + 1, 0);
        ssize_t sent_bytes = sendto(server->socket, output, strlen(output) + 1, 0, (struct sockaddr *) &client.address, client_addr_size);
        if (sent_bytes  < 0)
        {
            if (output)
            {
                free(output);
            }

//            log_printf(ANSI_COLOR_RED "Error al enviar línea de texto al cliente.\n");
            fail("ERROR: Error al enviar la línea de texto al cliente");
        }

        printf("[Servidor] Enviado: %s\n", output);

        if (output)
        {
            free(output);
        }
    }
}


int main(int argc, char *argv[])
{
    set_colors();

    struct Arguments args = {
            .server_port = OPT_SERVER_PORT,
            .logfile = DEFAULT_LOG_FILE
    };


    if (!setlocale(LC_ALL, ""))
    {
        fail("ERROR: No se pudo establecer la locale del programa");   /* Establecer la locale según las variables de entorno */
    }

    process_args(&args, argc, argv);

    printf("Ejecutando servidor de mayúsculas con parámetros: PUERTO=%u, LOG=%s\n", args.server_port, args.logfile);
    Host server = create_own_host(AF_INET, SOCK_DGRAM, 0, args.server_port, args.logfile);

    while (true)
    {

        handle_connection(&server);

    }

    printf("\nCerrando el servidor y saliendo...\n");
//    close_server(&server);

    close_host(&server);

    exit(EXIT_SUCCESS);
}
