#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include "host.h"
#include "loging.h"


#define DEFAULT_MAX_BYTES_RECV 2048

#define DEFAULT_INPUT_FILE_NAME "leeme.txt"

#define DEFAULT_LOCAL_PORT 9100

#define IP_LOCALHOST "127.0.0.1"
#define DEFAULT_SERVER_IP IP_LOCALHOST
#define DEFAULT_SERVER_PORT 9200

#define DEFAULT_LOG_FILE "clienteUDP.log"


/**
 * Estructura de datos para pasar a la función process_args.
 * Contiene una cantidad variable de variables que se quieran inicializar
 * a partir de la entrada del programa.
 */
struct Arguments {
    char *input_file_name;
    uint16_t local_port;
    char *server_ip;
    uint16_t server_port;
    char *logfile;
};

/**
 * Estrucutra enumerada para manejar de forma más limpia las distintas opciones del programa.
 */
enum Option {
    OPT_NO_OPTION = '~',
    OPT_OPTION_FLAG = '-',
    OPT_FILE_NAME_TO_READ = 'f',
    OPT_SOURCE_PORT = 'o',
    OPT_SERVER_IP = 'i',
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
 * @brief   Maneja el intercambio de datos con el servidor.
 *
 * Abre el archivo input_file_name, lo lee línea a línea, envía cada línea
 * al servidor para que este las pase a mayúsculas, y las escribe en un fichero
 * con el mismo nombre que input_file_name pero todo también en mayúsculas.
 *
 * @param local_client      Cliente que intercambia datos.
 * @param remote_server     Servidor con el que intercambiar datos.
 * @param input_file_name   Nombre del archivo de texto a transformar a mayúsculas.
 */
void handle_data(Host *local_client, Host *remote_server, char *input_file_name);


int main(int argc, char** argv) {
    Host client, server;
    struct Arguments args = {
            .input_file_name= DEFAULT_INPUT_FILE_NAME,
            .local_port=DEFAULT_LOCAL_PORT,
            .server_ip= DEFAULT_SERVER_IP,
            .server_port= DEFAULT_SERVER_PORT,
            .logfile= DEFAULT_LOG_FILE,
    };

    set_colors();
    process_args(&args, argc, argv);

    client = create_own_host(AF_INET, SOCK_DGRAM, 0, args.local_port, args.logfile);

    server = create_remote_host(AF_INET, SOCK_DGRAM, 0, args.server_ip, args.server_port);

    handle_data(&client, &server, args.input_file_name);

    printf("\nCerrando el cliente y saliendo...\n");
    close_host(&client);
    close_host(&server);

    exit(EXIT_SUCCESS);
}



void handle_data(Host *local_client, Host *remote_server, char *input_file_name) {
    ssize_t sent_bytes = 0, recv_bytes = 0;
    FILE* fp_input;
    FILE* fp_output;
    char recv_buffer[DEFAULT_MAX_BYTES_RECV];   /* Buffer de recepción */
    char* send_buffer = NULL;  /* Buffer de envío */
    size_t buffer_size = 0; /* Necesitamos una variable con el tamaño del buffer para getline */
    socklen_t socket_addr_len = sizeof(struct sockaddr_in);
    bool received_flag;

    /* Apertura de los archivos */
    if (!(fp_input = fopen(input_file_name, "r"))) {
        fail("ERROR: Error en la apertura del archivo de lectura");
    }


    /* Enviamos el nombre del archivo */
    printf("Se procede a enviar el archivo: %s al servidor con IP: %s y puerto: %d\n", input_file_name, remote_server->ip, remote_server->port);

    printf("\nEnviando el nombre del archivo (\"%s\")\n", input_file_name);

    sent_bytes = sendto(local_client->socket, input_file_name, strlen(input_file_name) + 1, /*flags*/ 0, (struct sockaddr *) &(remote_server->address), socket_addr_len);
    if (sent_bytes < 0) {
        fail("ERROR: No se pudo enviar el mensaje");
    }

    printf("Esperando respuesta del servidor...\n");

    /* Esperamos a recibir la línea */
    received_flag = false;
    while (!received_flag) {
        if (!socket_io_pending) pause();    /* Pausamos la ejecución hasta recibir una señal de I/O o de terminación */
        if (terminate) {
            if (fclose(fp_input)) fail("ERROR: No se pudo cerrar el archivo de lectura");
            return;
        }
        recv_bytes = recvfrom(local_client->socket, recv_buffer, DEFAULT_MAX_BYTES_RECV, /*flags*/ 0, (struct sockaddr *) &(remote_server->address), &socket_addr_len);
        if (recv_bytes == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {  /* Hemos marcado al socket con O_NONBLOCK; no hay conexiones pendientes, así que lo registramos y continuamos */
                socket_io_pending = 0;
                continue;
            }
            if (fclose(fp_input)) fail("ERROR: No se pudo cerrar el archivo de lectura");
            fail("ERROR: No se pudo recibir el mensaje");
        }
        socket_io_pending--;
        received_flag = true;
    }

    printf("Recibido: \"%s\"\n", recv_buffer);

    /* Recibido el nombre del archivo en mayúsculas */
    /* Abrimos en modo escritura el archivo */
    if (!(fp_output = fopen(recv_buffer, "w"))) {
        fail("ERROR: Error en la apertura del archivo de escritura");
    }


    /* Procesamiento y envÍo del archivo */
    while (!feof(fp_input)) {
        /* Leemos hasta que lo que devuelve getline es EOF */
        if (getline(&send_buffer, &buffer_size, fp_input) == EOF) {
            // Salimos del bucle. No hace falta avisar al servidor, porque no había ninguna conexión establecida.
            break;
        }

        /* Enviamos la línea */
        printf("\nEnviando: %s\n", input_file_name);

        sent_bytes = sendto(local_client->socket, send_buffer, strlen(send_buffer) + 1, 0, (struct sockaddr *) &(remote_server->address), socket_addr_len);
        if (sent_bytes < 0) {
            if (fclose(fp_input)) fail("ERROR: No se pudo cerrar el archivo de lectura");
            if (fclose(fp_output)) fail("ERROR: No se pudo cerrar el archivo de escritura");
            if (send_buffer) free(send_buffer);

            fail("ERROR: No se pudo enviar el mensaje");
        }

        /* Esperamos a recibir la línea */
        received_flag = false;
        while (!received_flag) {
            if (!socket_io_pending) pause();    /* Pausamos la ejecución hasta recibir una señal de I/O o de terminación */
            if (terminate) {
                if (fclose(fp_input)) fail("ERROR: No se pudo cerrar el archivo de lectura");
                if (fclose(fp_output)) fail("ERROR: No se pudo cerrar el archivo de escritura");
                if (send_buffer) free(send_buffer);
                return;
            }
            recv_bytes = recvfrom(local_client->socket, recv_buffer, DEFAULT_MAX_BYTES_RECV, /*flags*/ 0, (struct sockaddr *) &(remote_server->address), &socket_addr_len);
            if (recv_bytes == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {  /* Hemos marcado al socket con O_NONBLOCK; no hay conexiones pendientes, así que lo registramos y continuamos */
                    socket_io_pending = 0;
                    continue;
                }
                if (fclose(fp_input)) fail("ERROR: No se pudo cerrar el archivo de lectura");
                if (fclose(fp_output)) fail("ERROR: No se pudo cerrar el archivo de escritura");
                if (send_buffer) free(send_buffer);

                fail("ERROR: No se pudo recibir el mensaje");
            }
            socket_io_pending--;
            received_flag = true;
        }

        printf("Recibido: %s\n", recv_buffer);
        fprintf(fp_output, "%s", recv_buffer);
    }

    /* Cerramos los archivos al salir */
    if (fclose(fp_input)) fail("ERROR: No se pudo cerrar el archivo de lectura");
    if (fclose(fp_output)) fail("ERROR: No se pudo cerrar el archivo de escritura");
    if (send_buffer) free(send_buffer);

    return;
}



static void print_help(char *exe_name) {
    /** Cabecera y modo de ejecución **/
    printf("Uso: %s [-f] <file> [-o] <puerto_origen> [-i] <ip> [-p] <puerto_remoto> [-l <log> | --no-log] [-h]\n\n", exe_name);

    /** Lista de opciones de uso **/
    printf(" Opción\t\tOpción larga\t\tSignificado\n");

    printf(" -f <file>\t--file <file>\t\tNombre del fichero que convertir a mayúsculas.\n");
    printf(" -o <puerto_origen>\t--origen <puerto_origen>\t\tPuerto local desde el que se conectará con el servidor.\n");
    printf(" -i <ip>\t--ip <ip>\t\tDirección IP del servidor al que conectarse, o \"localhost\" si el servidor se ejecuta en el mismo host que el cliente.\n");
    printf(" -p <puerto_remoto>\t--puerto <puerto_remoto>\t\tPuerto en el que escucha el servidor al que conectarse.\n");

    printf(" -l <log>\t--log <log>\t\tNombre del archivo en el que guardar el registro de actividad del servidor.\n");
    printf(" -n\t\t--no-log\t\tNo crear archivo de registro de actividad.\n");
    printf(" -h\t\t--help\t\t\tMostrar este texto de ayuda y salir.\n");

    /** Consideraciones adicionales **/
    printf("\nPueden especificarse los parámetros <file>, <puerto_origen>, <ip> y <puerto_remoto> sin escribir las opciones '-f', '-o' '-i' ni '-p', siempre y cuando estos sean los cuatro parámetros que se pasan a la función, respectivamente.\n");
    printf("\nSi se especifica varias veces un argumento, el comportamiento está indefinido.\n");
}


static uint16_t getPortOrFail(char *argv[], int pos) {
    uint16_t read_number = atoi(argv[pos]);

    if (read_number <= 0 || read_number > 65535) {
        fprintf(stderr, "ERROR: El valor de puerto especificado (%s) no es válido\n", argv[pos]);
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    return read_number;
}


static void process_args(struct Arguments *args, int argc, char** argv) {
    char *current_arg_str;

    /* Flags para saber si se setearon el fichero a convertir, el puerto local y la IP y puerto remotos */
    bool set_file = false;
    bool set_local_port = false;
    bool set_ip = false;
    bool set_server_port = false;

    /* Procesamos los argumentos (sin contar el nombre del ejecutable) */
    for (int pos = 1; pos < argc; pos++) {
        current_arg_str = argv[pos];

        if (current_arg_str[0] == OPT_OPTION_FLAG) { /* Flag de opción */
            /* Manejar las opciones largas */
            if (current_arg_str[1] == OPT_OPTION_FLAG) {
                if (strcmp(current_arg_str, "--file"))
                {
                    current_arg_str = "-f";
                } else if (!strcmp(current_arg_str, "--origen"))
                {
                    current_arg_str = "-o";
                } else if (strcmp(current_arg_str, "--ip"))
                {
                    current_arg_str = "-i";
                } else if (!strcmp(current_arg_str, "--puerto"))
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
                case OPT_FILE_NAME_TO_READ: // 'f' /* Fichero */
                    if (++pos < argc) {
                        args->input_file_name = argv[pos];
                        set_file = true;
                    } else {
                        fprintf(stderr, "ERROR: Fichero no especificado tras la opción '-f'\n\n");
                        print_help(argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    break;

                case OPT_SOURCE_PORT: // 'o' /* Puerto Origen */
                    if (++pos < argc) {
                        args->local_port = getPortOrFail(argv, pos);
                        set_local_port = true;
                    } else {
                        fprintf(stderr, "ERROR: Puerto no especificado tras la opción '-o'\n");
                        print_help(argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    break;

                case OPT_SERVER_IP: // 'i' /* IP Servidor */
                    if (++pos < argc) {
                        if (!strcmp(argv[pos], "localhost")) {
                            // Permitir al cliente indicar localhost como IP
                            args->server_ip = IP_LOCALHOST;
                        } else {
                            args->server_ip = argv[pos];
                        }
                        set_ip = true;
                    } else {
                        fprintf(stderr, "ERROR: IP no especificada tras la opción '-i'\n\n");
                        print_help(argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    break;

                case OPT_SERVER_PORT: // 'p' /* Puerto Remoto */
                    if (++pos < argc) {
                        args->server_port = getPortOrFail(argv, pos);
                        set_server_port = true;
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
        } else if (pos == 1) {    /* Se especificó el fichero como primer argumento */
            args->input_file_name = argv[pos];
            set_file = true;
        } else if (pos == 2) {    /* Se especificó el puerto del cliente como cuartsegundoo argumento */
            args->local_port = getPortOrFail(argv, pos);
            set_local_port = true;
        } else if (pos == 3) {    /* Se especificó la IP del servidor como tercer argumento */
            if (!strcmp(argv[pos], "localhost")) {
                /* Permitir al cliente indicar localhost como IP */
                args->server_ip = IP_LOCALHOST;
            } else {
                args->server_ip = argv[pos];
            }
            set_ip = true;
        } else if (pos == 4) {    /* Se especificó el puerto del servidor como cuarto argumento */
            args->server_port = getPortOrFail(argv, pos);
            set_server_port = true;
        }
    }

    if (!set_file || !set_local_port || !set_ip || !set_server_port) {
        fprintf(stderr, "ERROR:\n%s%s%s%s\n",
                (set_file ? "" : "No se especificó fichero para convertir a mayúsculas.\n"),
                (set_local_port ? "" : "  No se especificó el puerto local que usar.\n"),
                (set_ip ? "" : "  No se especificó la IP del servidor al que conectarse.\n"),
                (set_server_port ? "" : "  No se especificó el puerto del servidor al que conectarse.\n"));

        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }
}
