#ifndef CLIENT_H
#define CLIENT_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

/**
 * Estructura que contiene toda la información relevante del
 * cliente, el socket por el que se comunica con el servidor
 * y la dirección del servidor con el que se está comunicando.
 */
typedef struct {
    int socket;         /* Socket asociado al cliente por el que conectarse a un servidor y comunicarse con él */
    int domain;         /* Dominio de comunicación. Especifica la familia de protocolos que se usan para la comunicación */
    int type;           /* Tipo de protocolo usado para el socket */
    int protocol;       /* Protocolo particular usado en el socket */
    char* hostname;     /* Nombre del equipo en el que está ejecutándose el cliente */
    char* ip;           /* IP externa del cliente (en formato textual) */
    char* server_ip;    /* IP del servidor al que conectarse (en formato textual) */
    uint16_t port;      /* Puerto por el que envía conexiones el cliente (en orden de host, pensado para uso del servidor) */
    uint16_t server_port;   /* Puerto en el que el servidor escucha peticiones (en orden de host) */
    struct sockaddr_in address;         /* Estructura con el dominio de comunicación e IP y puerto por los que se comunica el cliente (pensada para uso del servidor) */
    struct sockaddr_in server_address;  /* Estructura con el dominio de comunicación e IP y puerto del servidor al que conectarse */
} Client;


/**
 * @brief   Crea un cliente.
 *
 * Crea un cliente nuevo con un nuevo socket, y guarda en él la información necesaria
 * sobre el servidor para posteriormente poder conectarse con él.
 *
 * @param domain        Dominio de comunicación. 
 * @param type          Tipo de protocolo usado para el socket.
 * @param protocol      Protocolo particular a usar en el socket. Normalmente solo existe
 *                      un protocolo para la combinación dominio-tipo dada, en cuyo caso se
 *                      puede especificar con un 0.
 * @param server_ip     Dirección IP del servidor al que conectarse (en formato textual).
 * @param server_port   Número de puerto en el que escucha el servidor (en orden de host).
 *
 * @return  Cliente que guarda toda la información relevante sobre sí mismo con la que
 *          fue creado, y con un socket abierto en el cual está listo para conectarse 
 *          al servidor con la IP y puerto especificados.
 */
Client create_client(int domain, int type, int protocol, char* server_ip, uint16_t server_port);


/**
 * @brief   Conecta el cliente con el servidor.
 *
 * Crea una conexión con el servidor especificado durante la creación del cliente
 * a través de su socket.
 *
 * @param client    Cliente a conectar.
 */
void connect_to_server(Client client);


/**
 * @brief   Cierra el cliente.
 *
 * Cierra el socket asociado al cliente y libera toda la memoria
 * reservada para el cliente.
 *
 * @param client    Cliente a cerrar.
 */
void close_client(Client* client); 


#endif  /* CLIENT_H */
