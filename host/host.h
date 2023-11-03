#ifndef HOST_H
#define HOST_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

/**
 * Estructura que contiene toda la información relevante 
 * del servidor y el socket en el que escucha peticiones.
 */
typedef struct {
    int socket;     /* Socket asociado al servidor en el que escuchar conexiones o atender peticiones */
    int domain;     /* Dominio de comunicación. Especifica la familia de protocolos que se usan para la comunicación */
    int type;       /* Tipo de protocolo usado para el socket */
    int protocol;   /* Protocolo particular usado en el socket */
    uint16_t port;  /* Puerto en el que el servidor escucha peticiones (en orden de host) */
    char* hostname; /* Nombre del equipo en el que está ejecutándose el servidor */
    char* ip;       /* IP externa del servidor (en formato textual) */
    struct sockaddr_in address;  /* Estructura con el dominio de comunicación, IPs a las que atender
                                           y puerto al que está asociado el socket */
    FILE* log;      /* Archivo en el que guardar el registro de actividad del servidor */
} Host;


Host create_self_host();

Host create_peer_host();


/**
 * @brief   Cierra el host.
 *
 * Cierra el socket asociado al host y libera toda la memoria
 * reservada para el host.
 *
 * @param host  Host a cerrar.
 */
void close_host(Host* host);

#endif  /* HOST_H */

