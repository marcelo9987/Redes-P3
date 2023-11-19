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
#include <stdbool.h>

#include "host.h"
#include "getpublicip.h"
#include "loging.h"
#include "getlocalips.h"

#define BUFFER_LEN 2048

/** Variables globales que exportar en el fichero de cabecera para el manejo de señales */
uint8_t socket_io_pending;
bool terminate;


/**
 * @brief   Maneja las señales que recibe el host
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
            terminate = true;          /* Marca que el programa debe terminar */
            break;
        default:
            break;
    }
}


/**
 * @brief   Crea un host del propio programa.
 *
 * Crea un host nuevo con un nuevo socket, y le asigna un puerto.
 * Si el argumento logfile no es NULL, crea también un archivo de log para guardar un registro de actividad.
 *
 * @param domain    Dominio de comunicación. 
 * @param type      Tipo de protocolo usado para el socket.
 * @param protocol  Protocolo particular a usar en el socket. Normalmente solo existe
 *                  un protocolo para la combinación dominio-tipo dada, en cuyo caso se
 *                  puede especificar con un 0.
 * @param port      Número de puerto en el que escuchar (en orden de host).
 * @param logfile   Nombre del archivo en el que guardar el registro de actividad.
 *
 * @return  Host que guarda toda la información relevante sobre sí mismo con la que
 *          fue creado, y con un socket abierto y conectado por el puerto  especificado.
 */
Host create_own_host(int domain, int type, int protocol, uint16_t port, char* logfile){
    Host host;
    char buffer[BUFFER_LEN] = {0};

    memset(&host, 0, sizeof(Host));     /* Inicializamos los campos a 0 */

    host = (Host) {
        .domain = domain,
        .type = type,
        .protocol = protocol,
        .port = port,
        .address.sin_family = domain,
        .address.sin_port = htons(port),
        .address.sin_addr.s_addr = htonl(INADDR_ANY)    /* Aceptar conexiones desde cualquier IP */
    };

    /* Abrimos el log para escritura.
     * Si no se puede abrir, avisamos y seguimos, ya que no es un error crítico. */
    if (logfile) {
        if ( (host.log = fopen(logfile, "w")) == NULL)
            perror("No se pudo crear el log del host");
    }        
    log_printf(host.log, "Inicializando host...\n");

    /* Guardar el nombre del equipo en el que se ejecuta el host.
     * No produce error crítico, por lo que no hay que salir */
    if (gethostname(buffer, BUFFER_LEN)) {
        perror("No se pudo obtener el nombre de host del servidor");
        log_printf_err(host.log, "Error al obtener el nombre de host.\n");
    } else {
        host.hostname = (char *) calloc(strlen(buffer) + 1, sizeof(char));
        strcpy(host.hostname, buffer);
        log_printf(host.log, "Nombre de host configurado con éxito: %s.\n", host.hostname);
    }

    /* Guardar la IP externa del host.
     * Tampoco supone un error crítico. */
    if (!getpublicip(buffer, BUFFER_LEN)) {
        perror("No se pudo obtener la IP externa del host");
        log_printf_err(host.log, "Error al obtener la IP externa del host.\n");
    } else {
        host.public_ip = (char *) calloc(strlen(buffer) + 1, sizeof(char));
        strcpy(host.public_ip, buffer);
        log_printf(host.log, "IP externa del host configurada con éxito: %s.\n", host.public_ip);
    }

    /* Guardar las IPs v4 locales del host.
     * Tampoco supone un error crítico. */
    if (!get_local_ip_addresses(buffer, BUFFER_LEN, AF_INET)) {
        perror("No se pudieron obtener las IPs v4 locales del host");
        log_printf_err(host.log, "Error al obtener las IPs v4 locales del host.\n");
    } else {
        host.local_ips_v4 = (char *) calloc(strlen(buffer) + 1, sizeof(char));
        strcpy(host.local_ips_v4, buffer);
        log_printf(host.log, "IPs v4 locales del host configuradas con éxito: %s.\n", host.local_ips_v4);
    }

    /* Guardar las IPs v6 locales del host.
     * Tampoco supone un error crítico. */
    if (!get_local_ip_addresses(buffer, BUFFER_LEN, AF_INET6)) {
        perror("No se pudieron obtener las IPs v6 locales del host");
        log_printf_err(host.log, "Error al obtener las IPs v6 locales del host.\n");
    } else {
        host.local_ips_v6 = (char *) calloc(strlen(buffer) + 1, sizeof(char));
        strcpy(host.local_ips_v6, buffer);
        log_printf(host.log, "IPs v6 locales del host configuradas con éxito: %s.\n", host.local_ips_v6);
    }

    /* Crear el socket del host */
    if ( (host.socket = socket(domain, type, protocol)) < 0) {
        log_printf_err(host.log, "Error al crear el socket del host.\n");
        fail("No se pudo crear el socket");
    }

    /* Asignar IPs a las que escuchar y número de puerto por el que escuchar (bind) */
    if (bind(host.socket, (struct sockaddr *) &host.address, sizeof(struct sockaddr_in)) < 0) {
        log_printf_err(host.log, "Error al asignar la dirección (bind) del socket del host.\n");
        fail("No se pudo asignar dirección IP");
    }

    /* Configurar el host para enviarse a sí mismo un SIGIO cuando se produzca actividad en el socket, para evitar bloqueos esperando por conexiones */
    if (fcntl(host.socket, F_SETFL, O_ASYNC | O_NONBLOCK) < 0) {
        log_printf_err(host.log, "Error al configurar el envío de SIGIO en el socket.\n");
        fail("No se pudo configurar el envío de SIGIO en el socket");
    }
    if (fcntl(host.socket, F_SETOWN, getpid()) < 0) {
        log_printf_err(host.log, "Error al configurar el autoenvío de señales en el socket.\n");
        fail("No se pudo configurar el autoenvío de señales en el socket");
    }

    if (signal(SIGIO, signal_handler) == SIG_ERR) {
        log_printf_err(host.log, "Error al establecer el manejo de la señal SIGIO.\n");
        fail("No se pudo establecer el manejo de la señal SIGIO");
    }
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        log_printf_err(host.log, "Error al establecer el manejo de la señal SIGINT.\n");
        fail("No se pudo establecer el manejo de la señal SIGINT");
    }
    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
        log_printf_err(host.log, "Error al establecer el manejo de la señal SIGTERM.\n");
        fail("No se pudo establecer el manejo de la señal SIGTERM");
    }
    

    printf( "Host creado con éxito.\n"
            "Hostname: %s; IPs v4 locales:%s; IPs v6 locales: %s; Puerto: %d; IP pública: %s\n\n", host.hostname, host.local_ips_v4, host.local_ips_v6, host.port, host.public_ip);
    log_printf(host.log, "Host creado con éxito.\tHostname: %s; IPs v4 locales:%s; IPs v6 locales:%s; Puerto: %d; IP pública: %s\n", host.hostname, host.local_ips_v4, host.local_ips_v6, host.port, host.public_ip);

    return host;
}


/**
 * @brief   Crea un host remoto.
 *
 * Crea un host nuevo con la información del host remoto al que se envían
 * o del que se reciben datos.
 *
 * @param domain    Dominio de comunicación. 
 * @param type      Tipo de protocolo usado para el socket.
 * @param protocol  Protocolo particular a usar en el socket. Normalmente solo existe
 *                  un protocolo para la combinación dominio-tipo dada, en cuyo caso se
 *                  puede especificar con un 0.
 * @param ip        IP del host remoto (en formato textual).
 * @param port      Número de puerto en el que escucha el host remoto (en orden de host).
 *
 * @return  Host con toda la información relevante y disponible sobre el host remoto.
 */
Host create_remote_host(int domain, int type, int protocol, char* ip, uint16_t port) {
    Host remote;

    memset(&remote, 0, sizeof(Host));   /* Inicializamos los campos a 0 */

    remote = (Host) {
        .domain = domain,
        .type = type,
        .protocol = protocol,
        .port = port,
        .address.sin_family = domain,
        .address.sin_port = htons(port)
    };

    if (inet_pton(remote.domain, ip, &(remote.address.sin_addr)) != 1) { /* La string no se pudo traducir a una IP válida */
        fprintf(stderr, "La IP especificada (%s) no es válida\n\n", ip);
        exit(EXIT_FAILURE);
    }

    /* Guardar la IP del host remoto en formato textual */
    remote.public_ip = (char *) calloc(strlen(ip) + 1, sizeof(char));
    strcpy(remote.public_ip, ip);

    remote.socket = -1; /* Ponemos el socket a un valor no permitido para saber que no se puede usar y que no hay que cerrarlo */

    return remote;
}


/**
 * @brief   Cierra el host.
 *
 * Cierra el socket asociado al host y libera toda la memoria
 * reservada para el host.
 *
 * @param host  Host a cerrar.
 */
void close_host(Host* host) {
    log_printf(host->log, "Cerrando host...\n");
    /* Cerrar el socket del host */
    if (host->socket >= 0) {
        if (close(host->socket)) {
            log_printf_err(host->log, "Error al cerrar el socket del host.\n");
            fail("No se pudo cerrar el socket del host");
        }
    }

    if (host->hostname) free(host->hostname);
    if (host->public_ip) free(host->public_ip);
    if (host->local_ips_v4) free(host->local_ips_v4);
    if (host->local_ips_v6) free(host->local_ips_v6);
    if (host->log) fclose(host->log);

    /* Limpiar la estructura poniendo todos los campos a 0 */
    memset(host, 0, sizeof(Host));
    host->socket = -1;  /* Poner un socket no válido para que se sepa que no se puede usar ni volver a cerrar */

    return;
}
