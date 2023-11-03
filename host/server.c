#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#include "server.h"
#include "getip.h"
#include "loging.h"

#define BUFFER_LEN 128

/** Variables globales que exportar en el fichero de cabecera para el manejo de señales */
uint8_t socket_io_pending;
uint8_t terminate;


/**
 * @brief   Maneja las señales que recibe el servidor
 *
 * Maneja las señales de SIGIO, SIGINT y SIGTERM que puede recibir el servidor durante su ejecución.
 *  - SIGIO: tuvo lugar un evento de I/O en el socket del servidor.
 *  - SIGINT, SIGTERM: terminar la ejecución del programa segura.
 * Establece los valores de las variables globales socket_io_pending y terminate apropiadamente.
 *
 *  @param signum   Número de señal recibida.
 */
static void signal_handler(int signum) {
    switch (signum) {
        case SIGIO:
            socket_io_pending++;    /* Aumentar en 1 el número de eventos de entrada/salida pendientes */
            break;
        case SIGINT:
        case SIGTERM:
            terminate = 1;          /* Marca que el programa debe terminar */
            break;
        default:
            break;
    }

}


/**
 * @brief   Crea un servidor.
 *
 * Crea un servidor nuevo con un nuevo socket, le asigna un puerto y 
 * lo marca como pasivo para poder escuchar conexiones.
 * Si el parámetro log no es NULL, crea también un archivo de log para guardar un registro de actividad.
 *
 * @param domain    Dominio de comunicación. 
 * @param type      Tipo de protocolo usado para el socket.
 * @param protocol  Protocolo particular a usar en el socket. Normalmente solo existe
 *                  un protocolo para la combinación dominio-tipo dada, en cuyo caso se
 *                  puede especificar con un 0.
 * @param port      Número de puerto en el que escuchar (en orden de host).
 * @param backlog   Longitud máxima de la cola de conexiones pendientes.
 * @param logfile   Nombre del archivo en el que guardar el registro de actividad.
 *
 * @return  Servidor que guarda toda la información relevante sobre sí mismo con la que
 *          fue creado, y con un socket pasivo abierto en el cual está listo para escuchar 
 *          y aceptar conexiones entrantes desde cualquier IP y del dominio y por puerto 
 *          especificados.
 */
Server create_server(int domain, int type, int protocol, uint16_t port, int backlog, char* logfile) {
    Server server;
    char buffer[BUFFER_LEN] = {0};

    memset(&server, 0, sizeof(Server));     /* Inicializar los campos a 0 */

    server = (Server) {
        .domain = domain,
        .type = type,
        .protocol = protocol,
        .port = port,
        .backlog = backlog,
        .listen_address.sin_family = domain,
        .listen_address.sin_port = htons(port),
        .listen_address.sin_addr.s_addr = htonl(INADDR_ANY) /* Aceptar conexiones desde cualquier IP */
    };

    /* Abrimos el log para escritura.
     * Si no se puede abrir, avisamos y seguimos, ya que no es un error crítico. */
    if (logfile) {
        if ( (server.log = fopen(logfile, "w")) == NULL)
            perror("No se pudo crear el log del servidor");
    }        
    log_printf("Inicializando servidor...\n");

    /* Guardar el nombre del equipo en el que se ejecuta el servidor.
     * No produce error crítico, por lo que no hay que salir */
    if (gethostname(buffer, BUFFER_LEN)) {
        perror("No se pudo obtener el nombre de host del servidor");
        log_printf(ANSI_COLOR_RED "Error al obtener el nombre de host.\n" ANSI_COLOR_RESET);
    } else {
        server.hostname = (char *) calloc(strlen(buffer) + 1, sizeof(char));
        strcpy(server.hostname, buffer);
        log_printf("Nombre de host del servidor configurado con éxito: %s.\n", server.hostname);
    }

    /* Guardar la IP externa del servidor.
     * Tampoco supone un error crítico. */
    if (!getip(buffer, BUFFER_LEN)) {
        perror("No se pudo obtener la IP externa del servidor");
        log_printf(ANSI_COLOR_RED "Error al obtener la IP externa del servidor.\n" ANSI_COLOR_RESET);
    } else {
        server.ip = (char *) calloc(strlen(buffer) + 1, sizeof(char));
        strcpy(server.ip, buffer);
        log_printf("IP externa del servidor configurada con éxito: %s.\n", server.ip);
    }

    /* Crear el socket del servidor */
    if ( (server.socket = socket(domain, type, protocol)) < 0) {
        log_printf(ANSI_COLOR_RED "Error al crear el socket del servidor.\n" ANSI_COLOR_RESET);
        fail("No se pudo crear el socket");
    }

    /* Asignar IPs a las que escuchar y número de puerto por el que atender peticiones (bind) */
    if (bind(server.socket, (struct sockaddr *) &server.listen_address, sizeof(struct sockaddr_in)) < 0) {
        log_printf(ANSI_COLOR_RED "Error al asignar la dirección (bind) del socket del servidor.\n" ANSI_COLOR_RESET);
        fail("No se pudo asignar dirección IP");
    }

    /* Marcar el socket como pasivo, para que pueda escuchar conexiones de clientes */
    if (listen(server.socket, backlog) < 0) {
        log_printf(ANSI_COLOR_RED "Error al marcar el socket del servidor como pasivo (listen).\n" ANSI_COLOR_RESET);
        fail("No se pudo marcar el socket como pasivo");
    }

    /* Configurar el servidor para enviarse a sí mismo un SIGIO cuando se produzca actividad en el socket, para evitar bloqueos esperando por conexiones */
    if (fcntl(server.socket, F_SETFL, O_ASYNC | O_NONBLOCK) < 0) {
        log_printf(ANSI_COLOR_RED "Error al configurar el envío de SIGIO en el socket.\n" ANSI_COLOR_RESET);
        fail("No se pudo configurar el envío de SIGIO en el socket");
    }
    if (fcntl(server.socket, F_SETOWN, getpid()) < 0) {
        log_printf(ANSI_COLOR_RED "Error al configurar el autoenvío de señales en el socket.\n" ANSI_COLOR_RESET);
        fail("No se pudo configurar el autoenvío de señales en el socket");
    }

    if (signal(SIGIO, signal_handler) == SIG_ERR) {
        log_printf(ANSI_COLOR_RED "Error al establecer el manejo de la señal SIGIO.\n" ANSI_COLOR_RESET);
        fail("No se pudo establecer el manejo de la señal SIGIO");
    }
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        log_printf(ANSI_COLOR_RED "Error al establecer el manejo de la señal SIGINT.\n" ANSI_COLOR_RESET);
        fail("No se pudo establecer el manejo de la señal SIGINT");
    }
    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
        log_printf(ANSI_COLOR_RED "Error al establecer el manejo de la señal SIGTERM.\n" ANSI_COLOR_RESET);
        fail("No se pudo establecer el manejo de la señal SIGTERM");
    }


    printf("Servidor creado con éxito y listo para escuchar solicitudes de conexión.\n"
            "Hostname: %s; IP: %s; Puerto: %d\n\n", server.hostname, server.ip, server.port);
    log_printf("Servidor creado con éxito y listo para escuchar solicitudes de conexión.\tHostname: %s; IP: %s; Puerto: %d\n\n", server.hostname, server.ip, server.port);

    return server;
}


/**
 * @brief   Escucha conexiones de clientes.
 *
 * Pone al servidor a escuchar intentos de conexión.
 * El socket de conexión está marcado como no bloqueante.
 * Si no hay ninguna conexión pendiente, la función retorna y en client queda guardado un cliente
 * con todos sus campos a 0, excepto el socket, que tiene a -1.
 * En caso de que sí haya una conexión pendiente, se acepta, se informa de ella y se crea una nueva
 * estructura en la que guardar la información del cliente conectado, y un nuevo socket
 * conectado al cliente para atender sus peticiones.
 * Esta función no es responsable de liberar el cliente referenciado si este ya estuviese
 * iniciado, por lo que de ser así se debe llamar a close_client antes de invocar a esta función.
 * 
 * @param server    Servidor que poner a escuchar conexiones. Debe tener un socket
 *                  asociado marcado como pasivo.
 * @param client    Dirección en la que guardar la información del cliente conectado.
 *                  Guarda en el campo socket del cliente el nuevo socket conectado al cliente.
 */
void listen_for_connection(Server server, Client* client) {
    char ipname[INET_ADDRSTRLEN];
    socklen_t address_length = sizeof(struct sockaddr_in);

    memset(client, 0, sizeof(Client));  /* Inicializar los campos del cliente a 0. No se liberan campos */

    /* Aceptar la conexión del cliente en el socket pasivo del servidor */
    if ( (client->socket = accept(server.socket, (struct sockaddr *) &(client->address), &address_length)) < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {  /* Hemos marcado al socket con O_NONBLOCK; no hay conexiones pendientes, así que lo registramos y salimos */
            socket_io_pending = 0;
            return;
        }
        log_printf(ANSI_COLOR_RED "Error al aceptar una conexión.\n" ANSI_COLOR_RESET);
        fail("No se pudo aceptar la conexión");
    }


    /* Rellenar los campos del cliente con la información relevante para el servidor.
     * Todos los campos que se desconozcan se dejarán con el valor por defecto que deja 
     * la función close_client */
    client->domain = client->address.sin_family;
    client->type = server.type; /* Asumimos que usan el mismo tipo de protocolo de comunicación */
    client->protocol = server.protocol;
    /* Obtener el nombre de la dirección IP del cliente y guardarlo */
    inet_ntop(client->domain, &(client->address.sin_addr), ipname, INET_ADDRSTRLEN);
    client->ip = (char *) calloc(strlen(ipname) + 1, sizeof(char));
    strcpy(client->ip, ipname);
    client->server_ip = (char *) calloc(strlen(server.ip) + 1, sizeof(char));
    strcpy(client->server_ip, server.ip);
    client->port = ntohs(client->address.sin_port);
    client->server_port = server.port;

    /* Informar de la conexión */
    printf("Cliente conectado desde %s:%u.\n", client->ip, client->port);
    log_printf("Cliente conectado desde %s:%u.\n", client->ip, client->port);

    socket_io_pending--;    /* Una conexión ya manejada */

    return;
}


/**
 * @brief   Cierra el servidor.
 *
 * Cierra el socket asociado al servidor y libera toda la memoria
 * reservada para el servidor.
 *
 * @param server    Servidor a cerrar.
 */
void close_server(Server* server) {
    log_printfp("Cerrando el servidor...\n");
    /* Cerrar el socket del servidor */
    if (server->socket >= 0) {
        if (close(server->socket)) {
            log_printfp(ANSI_COLOR_RED "Error al cerrar el socket del servidor.\n");
            fail("No se pudo cerrar el socket del servidor");
        } 
    }

    if (server->hostname) free(server->hostname);
    if (server->ip) free(server->ip);
    if (server->log) fclose(server->log);

    /* Limpiar la estructura poniendo todos los campos a 0 */
    memset(server, 0, sizeof(Server));
    server->socket = -1;    /* Poner un socket no válido para que se sepa que no se puede usar ni volver a cerrar */
    
    return;
}
