#pragma once

#if defined( __linux__) || defined(__APPLE__)
#include <netinet/in.h>
#else
#include <WinSock2.h>

#endif

/*
 * Definitions for internet protocol version 4.
 * Per RFC 791, September 1981.
 */
#define	IPVERSION	4

/*
 * Structure of an internet header, naked of options.
 */
struct iphdr {
    u_int	ip_hl:4,		/* header length */
            ip_v:4;			/* version */

    u_char	ip_tos;			/* type of service */
    u_short	ip_len;			/* total length */
    u_short	ip_id;			/* identification */
    u_short	ip_off;			/* fragment offset field */
    u_char	ip_ttl;			/* time to live */
    u_char	ip_p;			/* protocol */
    u_short	ip_sum;			/* checksum */
    struct	in_addr ip_src,ip_dst;	/* source and dest address */
};

