#ifndef HOST_H
#define HOST_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>

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
Host create_own_host(int domain, int type, int protocol, uint16_t port, char* logfile);


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
Host create_remote_host(int domain, int type, int protocol, char* ip, uint16_t port);


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

