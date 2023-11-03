#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

#include "client.h"
#include "loging.h"

#define MAX_BYTES_RECV 2056
#define FILENAME_LEN 128


/**
 * Estructura de datos para pasar a la función process_args.
 * Debe contener siempre los campos int argc, char** argv, provenientes de main,
 * y luego una cantidad variable de punteros a las variables que se quieran inicializar
 * a partir de la entrada del programa.
 */
struct arguments {
    int argc;
    char** argv;
    char* server_ip;
    uint16_t* server_port;
    char* input_file_name;
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
static void print_help(char* exe_name);

/**
 * @brief   Maneja el intercambio de datos con el servidor.
 *
 * Abre el archivo input_file_name, lo lee línea a línea, envía cada línea
 * al servidor para que este las pase a mayúsculas, y las escribe en un fichero
 * con el mismo nombre que input_file_name pero todo también en mayúsculas.
 *
 * @param client    Cliente que intercambia datos, previamente conectado al servidor.
 * @param input_file_name   Nombre del archivo de texto a transformar a mayúsculas.
 */
void handle_data(Client client, char* input_file_name);



int main(int argc,char **argv){
    Client client;
    char server_ip[INET_ADDRSTRLEN];
    char input_file_name[FILENAME_LEN];
    uint16_t server_port;
    struct arguments args = {
        .argc = argc,
        .argv = argv,
        .server_ip = server_ip,
        .server_port = &server_port,
        .input_file_name = input_file_name
    };
	
    set_colors();

    process_args(args);

    client = create_client(AF_INET, SOCK_STREAM, 0, server_ip, server_port);

    connect_to_server(client); 

    handle_data(client, input_file_name);

    close_client(&client);

    exit(EXIT_SUCCESS);
}


void handle_data(Client client, char* input_file_name){
    ssize_t sent_bytes = 0, recv_bytes = 0;
    FILE *fp_input, *fp_output;
    char recv_buffer[MAX_BYTES_RECV];
    size_t buffer_size; /* Necesitamos una variable con el tamaño del buffer para getline */
    char* send_buffer;  /* Buffer para guardar las líneas del archivo a enviar. Como se usa getline, tiene que asignarse dinamicamente */

    /* Apertura de los archivos */
    if ( !(fp_input = fopen(input_file_name, "r")) ) fail("Error en la apertura del archivo de lectura");
	
    /* Enviamos el nombre del archivo */
    printf("Se procede a enviar el archivo: %s\n", input_file_name);

    if ( (sent_bytes = send(client.socket, input_file_name, strlen(input_file_name) + 1, 0)) < 0) fail("No se pudo enviar el mensaje");

    /* Esperamos a recibir la linea */
    if( (recv_bytes = recv(client.socket, recv_buffer, MAX_BYTES_RECV, 0)) < 0) fail("No se pudo recibir el mensaje");

    /* Recibido el nombre del archivo en mayúsculas */
    /* Abrimos en modo escritura el archivo */    
    if ( !(fp_output = fopen(recv_buffer, "w")) ) fail("Error en la apertura del archivo de escritura");

    /* Procesamiento y envio del archivo */
    /* Inicializamos el buffer de envío, en el que leeremos del archivo con getline */
    buffer_size = MAX_BYTES_RECV;
    send_buffer = (char *) calloc(buffer_size, sizeof(char));
    while (!feof(fp_input)) {
        /* Leemos hasta que lo que devuelve getline es EOF, cerramos la conexión en ese caso */
        if(getline(&send_buffer, &buffer_size, fp_input) == EOF){ /* Escaneamos la linea hasta el final del archivo */
            shutdown(client.socket, SHUT_RD);   /* Le decimos al servidor que pare de recibir */
            continue;
        }
        if ( (sent_bytes = send(client.socket, send_buffer, strlen(send_buffer) + 1, 0)) < 0) fail("No se pudo enviar el mensaje");

        /*Esperamos a recibir la linea*/
        if( (recv_bytes = recv(client.socket, recv_buffer, MAX_BYTES_RECV,0)) < 0) fail("No se pudo recibir el mensaje");

        fprintf(fp_output, "%s", recv_buffer);
    }
    
    /* Cerramos los archivos al salir */
    if (fclose(fp_input)) fail("No se pudo cerrar el archivo de lectura");
    if (fclose(fp_output)) fail("No se pudo cerrar el archivo de escritura");

    if (send_buffer) free(send_buffer);

    return;
}

static void print_help(char* exe_name){
    /** Cabecera y modo de ejecución **/
    printf("Uso: %s [-f] <file> [-i] <IP> [-p] <port> [-h]\n\n", exe_name);

    /** Lista de opciones de uso **/
    printf(" Opción\t\tOpción larga\t\tSignificado\n");
    printf(" -f <file>\t--file <file>\t\tNombre del fichero que convertir a mayúsculas.\n");
    printf(" -i/-I <IP>\t--ip/--IP <IP>\t\tIP del servidor al que conectarse, o \"localhost\" si el servidor se ejecuta en el mismo host que el cliente.\n");
    printf(" -p <port>\t--port <port>\t\tPuerto en el que escucha el servidor al que conectarse.\n");
    printf(" -h\t\t--help\t\t\tMostrar este texto de ayuda y salir.\n");

    /** Consideraciones adicionales **/
    printf("\nPueden especificarse los parámetros <file>, <IP> y <port> para el fichero a pasar a mayúsculas e IP y puerto en los que escucha el servidor sin escribir las opciones '-f', '-I' ni '-p', siempre y cuando estos sean el primer, segundo y tercer parámetros que se pasan a la función, respectivamente.\n");
    printf("\nSi se especifica varias veces un argumento, el comportamiento está indefinido.\n");
}

static void process_args(struct arguments args) {
    int i;
    char* current_arg;
    uint8_t set_file = 0, set_ip = 0, set_port = 0;   /* Flags para saber si se setearon el fichero a convertir, la IP y puerto */

    for (i = 1; i < args.argc; i++) { /* Procesamos los argumentos (sin contar el nombre del ejecutable) */
        current_arg = args.argv[i];
        if (current_arg[0] == '-') { /* Flag de opción */
            /* Manejar las opciones largas */
            if (current_arg[1] == '-') { /* Opción larga */
                if (!strcmp(current_arg, "--IP") || !strcmp(current_arg, "--ip")) current_arg = "-i";
                else if (!strcmp(current_arg, "--port")) current_arg = "-p";
                else if (!strcmp(current_arg, "--file")) current_arg = "-f";
                else if (!strcmp(current_arg, "--help")) current_arg = "-h";
            } 
            switch(current_arg[1]) {
                case 'I':   /* IP */
                case 'i':
                    if (++i < args.argc) {
                        if (!strcmp(args.argv[i], "localhost")) args.argv[i] = "127.0.0.1"; /* Permitir al cliente indicar localhost como IP */
                        strncpy(args.server_ip, args.argv[i], INET_ADDRSTRLEN);
                        set_ip = 1;
                    } else {
                        fprintf(stderr, "IP no especificada tras la opción '-i'\n\n");
                        print_help(args.argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    break;
                case 'p':   /* Puerto */
                    if (++i < args.argc) {
                        *args.server_port = atoi(args.argv[i]);
                        if (*args.server_port < 0) {
                            fprintf(stderr, "El valor de puerto especificado (%s) no es válido.\n\n", args.argv[i]);
                            print_help(args.argv[0]);
                            exit(EXIT_FAILURE);
                        }
                        set_port = 1;
                    } else {
                        fprintf(stderr, "Puerto no especificado tras la opción '-p'.\n\n");
                        print_help(args.argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    break;
                case 'f':   /* Fichero */
                    if (++i < args.argc) {
                        strncpy(args.input_file_name, args.argv[i], FILENAME_LEN);
                        set_file = 1;
                    } else {
                        fprintf(stderr, "Fichero no especificado tras la opción '-f'\n\n");
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
        } else if (i == 1) {    /* Se especificó el fichero como primer argumento */
            strncpy(args.input_file_name, args.argv[i], FILENAME_LEN);
            set_file = 1;
        } else if (i == 2) {    /* Se especificó la IP como segundo argumento */
            if (!strcmp(args.argv[i], "localhost")) args.argv[i] = "127.0.0.1"; /* Permitir al cliente indicar localhost como IP */
            strncpy(args.server_ip, args.argv[i], INET_ADDRSTRLEN);
            set_ip = 1;
        } else if (i == 3) {    /* Se especificó el puerto como tercer argumento */
            *args.server_port = atoi(args.argv[i]);
            if (*args.server_port < 0) {
                fprintf(stderr, "El valor de puerto especificado (%s) no es válido.\n\n", args.argv[i]);
                print_help(args.argv[0]);
                exit(EXIT_FAILURE);
            }
            set_port = 1;
        }
    }

    if (!set_file || !set_ip || !set_port) {
        fprintf(stderr, "%s%s%s\n", (set_file ? "" : "No se especificó fichero para convertir a mayúsculas.\n"),
                                    (set_ip ? "" : "No se especificó la IP del servidor al que conectarse.\n"), 
                                    (set_port ? "" : "No se especificó el puerto del servidor al que conectarse.\n"));
        print_help(args.argv[0]);
        exit(EXIT_FAILURE);
    }
}
