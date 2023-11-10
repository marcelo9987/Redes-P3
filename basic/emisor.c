#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

#include "../host/host.h"
#include "../host/loging.h"
// --

#define MESSAGE_SIZE 128

#define DEFAULT_RECEIVER_IP "127.0.0.1"
#define DEFAULT_PORT 8000
#define DEFAULT_BACKLOG 16
#define DEFAULT_LOG "log"
//--

/**
 * Estructura de datos para pasar a la función process_args.
 * Debe contener siempre los campos int argc, char** argv, provenientes de main,
 * y luego una cantidad variable de punteros a las variables que se quieran inicializar
 * a partir de la entrada del programa.
 */
struct arguments
{
    int argc;
    char **argv;
    uint16_t *port;
    char *receiver_ip;
    int *backlog;
    char **logfile;
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
static void print_help(char *exe_name);


// --
int main(int argc, char *argv[])
{
    Host self;
    Host other;
    uint16_t port;
    int backlog;
    char *logfile;
    struct arguments args = {
            .argc = argc,
            .argv = argv,
            .port = &port,
            .backlog = &backlog,
            .logfile = &logfile
    };
    set_colors();

    process_args(args);

    self = create_own_host(AF_INET, SOCK_DGRAM, 0, port, logfile);

    other = create_remote_host(AF_INET, SOCK_DGRAM, 0, args.receiver_ip, port);

    sendto(self.socket, "Hola", 4, 0, (struct sockaddr *) &other.address, sizeof(other.address));

    close_host(&self);

    return 0;
}

static void print_help(char *exe_name)
{
    /** Cabecera y modo de ejecución **/
    printf("Uso: %s [[-i] <ip>] [[-p] <port>] [-b <backlog>] [-l <log> | --no-log] [-h]\n\n", exe_name);

    /** Lista de opciones de uso **/
    printf(" Opción\t\tOpción larga\t\tSignificado\n");
    printf(" -i <ip>\t--ip <ip>\t\tIP del servidor al que conectarse.\n");
    printf(" -p <port>\t--port <port>\t\tPuerto en el que escuchará el servidor.\n");
    printf(" -b <backlog>\t--backlog <backlog>\tTamaño máximo de la cola de conexiones pendientes.\n");
    printf(" -l <log>\t--log <log>\t\tNombre del archivo en el que guardar el registro de actividad del servidor.\n");
    printf(" -n\t\t--no-log\t\tNo crear archivo de registro de actividad.\n");
    printf(" -h\t\t--help\t\t\tMostrar este texto de ayuda y salir.\n");

    /** Consideraciones adicionales **/
    printf("\nSi se especifica la opción '-i', debe especificarse también la opción '-p'.\n");
    printf("\nPuede especificarse el parámetro <port> para el puerto en el que escucha el servidor sin escribir la opción '-p', siempre y cuando este sea el primer parámetro que se pasa a la función.\n");
    printf("\nSi no se especifica alguno de los argumentos, el servidor se ejecutará con su valor por defecto, a saber: DEFAULT_PORT=%u; DEFAULT_BACKLOG=%d, DEFAULT_LOG=%s\n", DEFAULT_PORT, DEFAULT_BACKLOG, DEFAULT_LOG);
    printf("\nSi se especifica varias veces un argumento, o se especifican las opciones \"--log\" y \"--no-log\" a la vez, el comportamiento está indefinido.\n");
}


static void process_args(struct arguments args)
{
    int i;
    char *current_arg;

    /* Inicializar los valores de puerto y backlog a sus valores por defecto */
    args.receiver_ip=DEFAULT_RECEIVER_IP;
    *args.port = DEFAULT_PORT;
    *args.backlog = DEFAULT_BACKLOG;
    *args.logfile = DEFAULT_LOG;

    _Bool set_ip = 0;

    for (i = 1; i < args.argc; i++)
    { /* Procesamos los argumentos (sin contar el nombre del ejecutable) */
        current_arg = args.argv[i];
        if (current_arg[0] == '-')
        { /* Flag de opción */
            /* Manejar las opciones largas */
            if (current_arg[1] == '-')
            { /* Opción larga */
                if (!strcmp(current_arg, "--IP") || !strcmp(current_arg, "--ip"))
                { current_arg = "-i"; }
                if (!strcmp(current_arg, "--port"))
                { current_arg = "-p"; }
                else if (!strcmp(current_arg, "--backlog"))
                { current_arg = "-b"; }
                else if (!strcmp(current_arg, "--log"))
                { current_arg = "-l"; }
                else if (!strcmp(current_arg, "--no-log"))
                { current_arg = "-n"; }
                else if (!strcmp(current_arg, "--help"))
                { current_arg = "-h"; }
            }
            switch (current_arg[1])
            {
                case 'p':   /* Puerto */
                    if (++i < args.argc)
                    {
                        *args.port = atoi(args.argv[i]);
                        if (*args.port < 0)
                        {
                            fprintf(stderr, "El valor de puerto especificado (%s) no es válido.\n\n", args.argv[i]);
                            print_help(args.argv[0]);
                            exit(EXIT_FAILURE);
                        }
                    } else
                    {
                        fprintf(stderr, "Puerto no especificado tras la opción '-p'.\n\n");
                        print_help(args.argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    break;
                case 'b':   /* Backlog */
                    if (++i < args.argc)
                    {
                        *args.backlog = atoi(args.argv[i]);
                        if (*args.backlog < 0)
                        {
                            fprintf(stderr, "El valor de backlog especificado (%s) no es válido.\n\n", args.argv[i]);
                            print_help(args.argv[0]);
                            exit(EXIT_FAILURE);
                        }
                    } else
                    {
                        fprintf(stderr, "Tamaño del backlog no especificado tras la opción '-b'.\n\n");
                        print_help(args.argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    break;
                case 'l':   /* Log */
                    if (++i < args.argc)
                    {
                        *args.logfile = args.argv[i];
                    } else
                    {
                        fprintf(stderr, "Nombre del log no especificado tras la opción '-l'.\n\n");
                        print_help(args.argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    break;
                case 'n':   /* No-log */
                    *args.logfile = NULL;
                    break;
                case 'h':   /* Ayuda */
                    print_help(args.argv[0]);
                    exit(EXIT_SUCCESS);
                case 'I':   /* IP */
                case 'i':
                    if (++i < args.argc)
                    {
                        if (!strcmp(args.argv[i], "localhost"))
                        {
                            args.argv[i] = "127.0.0.1"; } /* Permitir al cliente indicar localhost como IP */
                            args.receiver_ip=malloc(INET_ADDRSTRLEN);
                            strncpy(args.receiver_ip, args.argv[i], INET_ADDRSTRLEN);
                        set_ip = 1;
                    }
                    break;
                default:
                    fprintf(stderr, "Opción '%s' desconocida\n\n", current_arg);
                    print_help(args.argv[0]);
                    exit(EXIT_FAILURE);
            }
        } else if (i == 1)
        {    /* Se especificó el puerto como primer argumento */
            *args.port = atoi(args.argv[i]);
            if (*args.port < 0)
            {
                fprintf(stderr, "El valor de puerto especificado como primer argumento (%s) no es válido.\n\n", args.argv[i]);
                print_help(args.argv[0]);
                exit(EXIT_FAILURE);
            }
        }
    }
    if (!set_ip)
    {
        fprintf(stderr, "%s\n", "No se especificó la IP del servidor al que conectarse.\n");
        print_help(args.argv[0]);
        exit(EXIT_FAILURE);
    }
}

