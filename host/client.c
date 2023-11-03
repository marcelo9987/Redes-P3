#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

#include "client.h"
#include "getip.h"
#include "loging.h"

#define BUFFER_LEN 64


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
Client create_client(int domain, int type, int protocol, char* server_ip, uint16_t server_port) {
    Client client;
    char buffer[BUFFER_LEN] = {0};

    memset(&client, 0, sizeof(Client));     /* Inicializar los campos a 0 */

    client = (Client) {
        .domain = domain,
        .type = type,
        .protocol = protocol,
        .server_port = server_port,
        .server_address.sin_family = domain,
        .server_address.sin_port = htons(server_port),
    };

    if (inet_pton(client.domain, server_ip, &(client.server_address.sin_addr)) != 1) {  /* La string no se pudo traducir a una IP válida */
        fprintf(stderr, "La IP especificada no es válida\n\n");
        exit(EXIT_FAILURE);
    }

    /* Guardar la IP del servidor en formato textual */
    client.server_ip = (char *) calloc(strlen(server_ip) + 1, sizeof(char));
    strcpy(client.server_ip, server_ip);

    /* Guardar el nombre del equipo en el que se ejecuta el cliente.
     * No produce error crítico, por lo que no hay que salir */
    if (gethostname(buffer, BUFFER_LEN)) {
        perror("No se pudo obtener el nombre de host del cliente");
    } else {
        client.hostname = (char *) calloc(strlen(buffer) + 1, sizeof(char));
        strcpy(client.hostname, buffer);
    }

    /* Guardar la IP externa del cliente.
     * Tampoco supone un error crítico. */
    if (!getip(buffer, BUFFER_LEN)) {
        perror("No se pudo obtener la IP externa del cliente");
    } else {
        client.ip = (char *) calloc(strlen(buffer) + 1, sizeof(char));
        strcpy(client.ip, buffer);
    }

    /* Crear el socket del cliente */
    if ( (client.socket = socket(domain, type, protocol)) < 0) fail("No se pudo crear el socket");

    return client;
}


/**
 * @brief   Conecta el cliente con el servidor.
 *
 * Crea una conexión con el servidor especificado durante la creación del cliente
 * a través de su socket.
 *
 * @param client    Cliente a conectar.
 */
void connect_to_server(Client client) {
    if (connect(client.socket, (struct sockaddr *) &(client.server_address), sizeof(struct sockaddr_in)) < 0) fail("No se pudo conetar con el servidor");

    printf("Conectado con éxito al servidor %s por el puerto %d\n", client.server_ip, client.server_port);

    return;
}


/**
 * @brief   Cierra el cliente.
 *
 * Cierra el socket asociado al cliente y libera toda la memoria
 * reservada para el cliente.
 *
 * @param client    Cliente a cerrar.
 */
void close_client(Client* client) {
    /* Cerrar el socket del cliente */
    if (client->socket != -1) {
        if (close(client->socket)) fail("No se pudo cerrar el socket del cliente");
    }

    if (client->hostname) free(client->hostname);
    if (client->ip) free(client->ip);
    if (client->server_ip) free(client->server_ip);

    /* Limpiar la estructura poniendo todos los campos a 0 */
    memset(client, 0, sizeof(Client));
    client->socket = -1;    /* Poner un socket no válido para que se sepa que no se puede usar ni volver a cerrar */

    return;
}
