#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#include "getip.h"

#define BUFFER_LEN 1024
#define SERVICE "http"
#define HTTP_REQUEST    "GET / HTTP/1.1\n"\
                        "Host: " NODE_NAME "\n"\
                        "Connection: close\n"\
                        "\n"

/**
 * @brief   Obtiene la IP externa
 *
 * Envía una petición HTTP a la página web api.ipify.org, <url>https://ipify.org</url>, para obtener la dirección
 * IP externa con la que el equipo se conecta a internet. 
 * 
 * @param ip    String en la que guardar la dirección IP obtenida.
 * @param len   Longitud de la string ip. Para asegurarse de que la IP cabe entera, debería ser por lo menos INET_ADDRSTRLEN.
 *
 * @return  Puntero a la string de destino ip, o NULL en caso de error.
 */
char* getip(char* ip, size_t len) {
    struct addrinfo hints;  
    struct addrinfo* result, *rp;
    int status;
    int sockfd;
    char http_request[] = HTTP_REQUEST;
    char input_buffer[BUFFER_LEN] = {0};

    memset(&hints, 0, sizeof(struct addrinfo)); /* Inicializar a 0 */
    /* Especifica criterios para seleccionar las estructuras de direcciones de socket en la lista que se obtiene con getaddrinfo() */
    hints = (struct addrinfo) {
        .ai_family   = AF_INET,         /* Familia de direcciones para las direcciones devueltas (IPv4) */
        .ai_socktype = SOCK_STREAM,     /* Tipo de socket preferido */
        .ai_protocol  = 0,              /* Protocolo para las direcciones devueltas */
        .ai_flags    = AI_ADDRCONFIG    /* Nos aseguramos de que no devuelve direcciones de tipo IPv6 */
    };

    /* Obtenemos las estructuras con direcciones de internet a las que preguntar por nuestra IP */
    if ( (status = getaddrinfo(NODE_NAME, SERVICE, &hints, &result)) ) {
        fprintf(stderr, "Error al obtener la información del servidor: %s\n", gai_strerror(status));
        return NULL;
    }

    /* getaddrinfo() devuelve en result una lista de struct addrinfo;
     * probamos con cada una hasta conseguir conectarse */
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        if ( (sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) < 0) continue;   /* Esta dirección no permite crear el socket */

        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) break; /* Conectado con éxito */

        close(sockfd);  /* Cerrar el socket si no se pudo conectar */
    }

    if (!rp) {
        fprintf(stderr, "No se pudo conectar a %s\n", NODE_NAME);
        return NULL;
    }

    /* Ya estamos conectados a ipify.org. Ahora tenemos que enviarle la petición
     * HTTP y procesar la respuesta */
    if (send(sockfd, http_request, strlen(http_request) + 1, 0) == 0 ) {    /* No se envió ningún byte */
        perror("No se pudo enviar la petición de http");
        return NULL;
    }

    if (recv(sockfd, input_buffer, BUFFER_LEN, 0) == 0 ) {      /* No se recibió ningún byte */
        perror("No se pudo recibir la respuesta de http");
        return NULL;
    }

    /* Parsear el input_buffer hasta encontrar dos saltos de línea seguidos */
    /* Buscamos la primera aparición de dos saltos de línea, que indica que inicia el cuerpo del mensaje, que solo
     * contiene nuestra IP externa */
    strncpy(ip, strstr(input_buffer, "\r\n\r\n") + 4, len);

    freeaddrinfo(result);    

    return ip;
}
