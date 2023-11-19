
#include "getlocalips.h"

#define _GNU_SOURCE     /* To get defns of NI_MAXSERV and NI_MAXHOST */

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_link.h>

char *get_local_ip_addresses(char *local_ip_addresses_string, size_t len, int family_to_request)
{

    local_ip_addresses_string[0] = '\0';

    struct ifaddrs *ifaddr;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        return NULL;
    }

    /* Walk through linked list, maintaining head pointer so we
       can free list later. */

    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
        {
            continue;
        }

        int family = ifa->ifa_addr->sa_family;

        /* Display interface name and family (including symbolic form of the latter for the common families). */
        /*
        printf("%-8s %s (%d)\n",
               ifa->ifa_name,
               (family == AF_PACKET) ? "AF_PACKET" :
               (family == AF_INET) ? "AF_INET" :
               (family == AF_INET6) ? "AF_INET6" : "???",
               family);
        */

        /* For an AF_INET* interface address, display the address. */
        if (family == AF_INET || family == AF_INET6)
        {
            int s = getnameinfo(ifa->ifa_addr,
                                (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                sizeof(struct sockaddr_in6),
                                host, NI_MAXHOST,
                                NULL, 0, NI_NUMERICHOST);
            if (s != 0)
            {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                return NULL;
            }

//            printf("\t\taddress: <%s>\n", host);

            if (family == family_to_request)
            {
                if (local_ip_addresses_string[0] != '\0')
                {
                    strcat(local_ip_addresses_string, ", ");
                }

                strcat(local_ip_addresses_string, host);
            }
/*
        } else if (family == AF_PACKET && ifa->ifa_data != NULL)
        {
            struct rtnl_link_stats *stats = ifa->ifa_data;
            printf("\t\ttx_packets = %10u; rx_packets = %10u\n"
                   "\t\ttx_bytes   = %10u; rx_bytes   = %10u\n",
                   stats->tx_packets, stats->rx_packets,
                   stats->tx_bytes, stats->rx_bytes);
*/
        }
    }

    freeifaddrs(ifaddr);

    return local_ip_addresses_string;

}
