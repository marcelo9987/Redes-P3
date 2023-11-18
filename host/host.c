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

#include "host.h"
#include "getip.h"
#include "loging.h"

#define BUFFER_LEN 128


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

    memset(&host, 0, sizeof(Host));     /* Inicializar los campos a 0 */

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
    if (!getip(buffer, BUFFER_LEN)) {
        perror("No se pudo obtener la IP externa del host");
        log_printf_err(host.log, "Error al obtener la IP externa del host.\n");
    } else {
        host.ip = (char *) calloc(strlen(buffer) + 1, sizeof(char));
        strcpy(host.ip, buffer);
        log_printf(host.log, "IP externa del host configurada con éxito: %s.\n", host.ip);
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
    
    printf( "Host creado con éxito.\n"
            "Hostname: %s; IP: %s; Puerto: %d\n\n", host.hostname, host.ip, host.port);
    log_printf(host.log, "Host creado con éxito.\tHostname: %s; IP: %s; Puerto: %d\n", host.hostname, host.ip, host.port);

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

    memset(&remote, 0, sizeof(Host));   /* Inicializar los campos a 0 */

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
    remote.ip = (char *) calloc(strlen(ip) + 1, sizeof(char));
    strcpy(remote.ip, ip);

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
    if (host->ip) free(host->ip);
    if (host->log) fclose(host->log);

    /* Limpiar la estructura poniendo todos los campos a 0 */
    memset(host, 0, sizeof(Host));
    host->socket = -1;  /* Poner un socket no válido para que se sepa que no se puede usar ni volver a cerrar */

    return;
}
