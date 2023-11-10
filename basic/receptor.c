#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

//todo: valor temporal para que compile. Hay que buscarle sitio y valor adecuado
#define MAX_BYTES_RECVFROM 128

#include "../host/host.h"
#include "../host/client.h"
#include "../host/loging.h"

#define MAX_BYTES_RECV 128

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
    uint16_t *self_port;
    char **log_file;
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
 * @brief   Procesa los argumentos del main.
 *
 * Procesa los argumentos proporcionados al programa por línea de comandos,
 * e inicializa las variables del programa necesarias acorde a estos.
 *
 * @param args  Estructura con los argumentos del programa y punteros a las
 *              variables que necesitan inicialización.
 */
static void print_help(char *exe_name);

/**
 * @brief   Procesa los argumentos del main.
 *
 * Procesa los argumentos proporcionados al programa por línea de comandos,
 * e inicializa las variables del programa necesarias acorde a estos.
 *
 * @param args  Estructura con los argumentos del programa y punteros a las
 *              variables que necesitan inicialización.
 */
void handle_data(Host self, Host other);

int main(int argc, char *argv[])
{
    Host receiver;
    uint16_t self_port;
    char *log_file;
    struct arguments args = {
            .argc = argc,
            .argv = argv,
            .self_port = &self_port,
            .log_file = &log_file
    };

    set_colors();

    process_args(args);

    receiver = create_self_host(AF_INET, SOCK_DGRAM, 0, self_port, log_file);

    Host other;

    handle_data(receiver, other);

    close_host(&receiver);

    exit(EXIT_SUCCESS);
}

void handle_data(Host self, Host other)
{
    ssize_t received_bytes = 0;
    char received_messsage[MAX_BYTES_RECVFROM];

    int size_addr = sizeof(struct sockaddr);
    while ((received_bytes = recvfrom(self.socket, received_messsage, MAX_BYTES_RECV, 0, (struct sockaddr*) &(other.address), &size_addr) != 0)
    {
        // Se recibe hasta finalizar conexión
        if (received_bytes < 0)
        {
            fail("Error en la recepción del mensaje");
        }

        printf("Mensaje recibido : %s\n", server_message);
        printf("Han sido recibidos %ld bytes.\n", recv_bytes);

    }
}

static void print_help(char *executable_name)
{
    /** Cabecera y modo de ejecución **/
    printf("Uso: %s [-p] <port> [-h]");

    //todo: adáptame
    /** Lista de opciones de uso **/
    printf(" Opción\t\tOpción larga\t\tSignificado\n");
    printf(" -i/-I <IP>\t--ip/--IP <IP>\t\tIP del servidor al que conectarse, o \"localhost\" si el servidor se ejecuta en el mismo host que el cliente.\n");
    printf(" -p <port>\t--port <port>\t\tPuerto en el que escucha el servidor al que conectarse.\n");
    printf(" -h\t\t--help\t\t\tMostrar este texto de ayuda y salir.\n");

    /** Consideraciones adicionales **/
    printf("\nPueden especificarse los parámetros <IP> y <port> para la IP y puerto en los que escucha el servidor sin escribir las opciones '-I' ni '-p', siempre y cuando estos sean el primer y segundo parámetros que se pasan a la función, respectivamente.\n");
    printf("\nSi se especifica varias veces un argumento, el comportamiento está indefinido.\n");

}

static void process_args(struct arguments args)
{
    char *current_arg;
    uint8_t set_ip = 0, set_port = 0;   /* Flags para saber si se setearon la IP y puerto */
    int i;

    for (int i = 1; i < args.argc; i++)
    {
        /* Procesamos los argumentos (sin contar el nombre del ejecutable) */
        current_arg = args.argv[i];
        if (current_arg[0] == '-')
        { /* Flag de opción */
            /* Manejar las opciones largas */
            if (current_arg[1] == '-')
            { /* Opción larga */
                if (!strcmp(current_arg, "--port"))
                { current_arg = "-p"; }
                else if (!strcmp(current_arg, "--help"))
                { current_arg = "-h"; }
            }

            switch (current_arg[1])
            {
                case 'p':   /* Puerto */
                    if (++i < args.argc)
                    {
                        *args.server_port = atoi(args.argv[i]);
                        if (*args.server_port < 0)
                        {
                            fprintf(stderr, "El valor de puerto especificado (%s) no es válido.\n\n", args.argv[i]);
                            print_help(args.argv[0]);
                            exit(EXIT_FAILURE);
                        }
                        set_port = 1;
                    } else
                    {
                        fprintf(stderr, "Puerto no especificado tras la opción '-p'.\n\n");
                        print_help(args.argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    break;
                case 'h':   /* Ayuda */
                    print_help(args.argv[0]);
                    exit(EXIT_SUCCESS);
                default:
                    fprintf(stderr, "Opción '%s' desconocida\n\n", current_arg);
                    print_help(args.argv[0]);
                    exit(EXIT_FAILURE);
            }
    } else if (i == 1)
    {    /* Se especificó la IP como primer argumento */
        if (!strcmp(args.argv[i], "localhost"))
        { args.argv[i] = "127.0.0.1"; } /* Permitir al cliente indicar localhost como IP */
        strncpy(args.server_ip, args.argv[i], INET_ADDRSTRLEN);
        set_ip = 1;
    } else if (i == 2)
    {    /* Se especificó el puerto como segundo argumento */
        *args.server_port = atoi(args.argv[i]);
        if (*args.server_port < 0)
        {
            fprintf(stderr, "El valor de puerto especificado (%s) no es válido.\n\n", args.argv[i]);
            print_help(args.argv[0]);
            exit(EXIT_FAILURE);
        }
        set_port = 1;
    }
}
    if (!set_port)
    {
        fprintf(stderr, "%s%s\n", "No se especificó el puerto del servidor al que conectarse.\n");
        print_help(args.argv[0]);
        exit(EXIT_FAILURE);
    }
