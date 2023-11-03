#ifndef GETIP_H
#define GETIP_H

#include <stdlib.h>

/* Nombre de la pagína web que proporciona la IP pública */
#define NODE_NAME "api.ipify.org"

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
char* getip(char* ip, size_t len);


#endif /* GETIP_H */

