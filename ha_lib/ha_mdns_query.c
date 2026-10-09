#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <lwip/api.h>
#include <lwip/sockets.h>
#include <lwip/ip_addr.h>
#include <lwip/netdb.h>
#include "ha_mdns_query.h"

#define MDNS_MULTICAST_IP   "224.0.0.251"
#define MDNS_PORT           5353
#define MDNS_BUF_SIZE       512
#define MDNS_MAX_RETRIES    2
#define MDNS_RETRY_MS       1000

/* Simple cache for resolved hostnames */
#define CACHE_SIZE 4
static struct {
    char host[64];
    ip_addr_t ip;
    uint32_t expires;  /* ticks when entry expires */
} s_cache[CACHE_SIZE];
static int s_cache_idx = 0;

/* Check if string is a dotted-decimal IPv4 address */
static int is_ip_address(const char *s)
{
    int dots = 0;
    while (*s) {
        if (*s == '.') {
            dots++;
        } else if (*s < '0' || *s > '9') {
            return 0;
        }
        s++;
    }
    return dots == 3;
}

/* Encode a domain name into DNS wire format.
 * "homeassistant.local" -> \x0chomeassistant\x05local\x00
 * Returns number of bytes written. */
static int dns_encode_name(const char *name, uint8_t *out, int out_len)
{
    int pos = 0;
    const char *p = name;

    /* Strip trailing dot if present */
    int name_len = strlen(name);
    if (name_len > 0 && name[name_len - 1] == '.') {
        name_len--;
    }

    while (*p && (p - name) < name_len) {
        const char *dot = p;
        while (*dot && *dot != '.' && (dot - name) < name_len) {
            dot++;
        }
        int label_len = dot - p;
        if (label_len <= 0 || label_len > 63 || pos + 1 + label_len >= out_len) {
            return -1;
        }
        out[pos++] = (uint8_t)label_len;
        memcpy(out + pos, p, label_len);
        pos += label_len;
        p = dot;
        if (*p == '.') {
            p++;
        }
    }
    out[pos++] = 0; /* root label */
    return pos;
}

/* Build mDNS query packet for A record */
static int build_mdns_query(const char *hostname, uint8_t *buf, int buf_len)
{
    int pos = 0;

    if (buf_len < 12) return -1;

    /* Header: ID=0, Flags=0, QDCOUNT=1, ANCOUNT=0, NSCOUNT=0, ARCOUNT=0 */
    memset(buf, 0, 12);
    buf[0] = 0x00; buf[1] = 0x00; /* ID */
    buf[2] = 0x00; buf[3] = 0x00; /* Flags */
    buf[4] = 0x00; buf[5] = 0x01; /* QDCOUNT = 1 */
    buf[6] = 0x00; buf[7] = 0x00; /* ANCOUNT */
    buf[8] = 0x00; buf[9] = 0x00; /* NSCOUNT */
    buf[10] = 0x00; buf[11] = 0x00; /* ARCOUNT */
    pos = 12;

    /* Question: encode hostname */
    int name_len = dns_encode_name(hostname, buf + pos, buf_len - pos - 4);
    if (name_len < 0) return -1;
    pos += name_len;

    /* TYPE = A (1), CLASS = IN (1) with unicast-response bit */
    buf[pos++] = 0x00;
    buf[pos++] = 0x01; /* TYPE A */
    buf[pos++] = 0x80; /* CLASS IN with cache-flush bit */
    buf[pos++] = 0x01;

    return pos;
}

/* Parse mDNS response and extract A record IP */
static int parse_mdns_response(const uint8_t *buf, int len, ip_addr_t *result)
{
    if (len < 12) return -1;

    int ancount = (buf[6] << 8) | buf[7];
    if (ancount == 0) return -1;

    int pos = 12;

    /* Skip question section */
    int qdcount = (buf[4] << 8) | buf[5];
    for (int i = 0; i < qdcount; i++) {
        /* Skip name (compressed or normal) */
        while (pos < len) {
            uint8_t b = buf[pos];
            if (b == 0) { pos++; break; }
            if ((b & 0xC0) == 0xC0) { pos += 2; break; }
            pos += 1 + b;
        }
        pos += 4; /* TYPE + CLASS */
    }

    /* Parse answer section */
    for (int i = 0; i < ancount && pos < len; i++) {
        /* Skip name (may be compressed) */
        if ((buf[pos] & 0xC0) == 0xC0) {
            pos += 2;
        } else {
            while (pos < len && buf[pos] != 0) {
                pos += 1 + buf[pos];
            }
            pos++; /* skip null terminator */
        }

        if (pos + 10 > len) break;

        int type = (buf[pos] << 8) | buf[pos + 1];
        int rdlength = (buf[pos + 8] << 8) | buf[pos + 9];
        pos += 10;

        /* Looking for A record (type 1) */
        if (type == 1 && rdlength == 4 && pos + 4 <= len) {
            ip_addr_t ip;
            IP4_ADDR(&ip, buf[pos], buf[pos + 1], buf[pos + 2], buf[pos + 3]);
            *result = ip;
            return 0;
        }

        pos += rdlength;
    }

    return -1;
}

/* Convert hostname to DNS wire format label for cache lookup.
 * Strips .local suffix for comparison. */
static void normalize_host(char *out, int out_len, const char *host)
{
    strncpy(out, host, out_len - 1);
    out[out_len - 1] = '\0';
    int len = strlen(out);
    if (len > 6 && strcmp(out + len - 6, ".local") == 0) {
        out[len - 6] = '\0';
    }
}

/* Check cache for a hostname */
static int cache_lookup(const char *host, ip_addr_t *result)
{
    char norm[64];
    normalize_host(norm, sizeof(norm), host);

    for (int i = 0; i < CACHE_SIZE; i++) {
        if (s_cache[i].host[0] != '\0' &&
            strcmp(s_cache[i].host, norm) == 0 &&
            xTaskGetTickCount() < s_cache[i].expires) {
            *result = s_cache[i].ip;
            return 0;
        }
    }
    return -1;
}

/* Add entry to cache */
static void cache_add(const char *host, const ip_addr_t *ip)
{
    char norm[64];
    normalize_host(norm, sizeof(norm), host);

    strncpy(s_cache[s_cache_idx].host, norm, sizeof(s_cache[s_cache_idx].host) - 1);
    s_cache[s_cache_idx].host[sizeof(s_cache[s_cache_idx].host) - 1] = '\0';
    s_cache[s_cache_idx].ip = *ip;
    s_cache[s_cache_idx].expires = xTaskGetTickCount() + pdMS_TO_TICKS(300000); /* 5 min */
    s_cache_idx = (s_cache_idx + 1) % CACHE_SIZE;
}

/* Send mDNS query and wait for response */
static int mdns_query_a(const char *hostname, ip_addr_t *result, uint32_t timeout_ms)
{
    int sock;
    struct sockaddr_in dest_addr;
    uint8_t query[MDNS_BUF_SIZE];
    uint8_t response[MDNS_BUF_SIZE];
    int query_len;
    int ret = -1;

    query_len = build_mdns_query(hostname, query, sizeof(query));
    if (query_len < 0) {
        printf("[MDNS_Q] failed to build query for %s\n", hostname);
        return -1;
    }

    sock = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        printf("[MDNS_Q] socket create failed\n");
        return -1;
    }

    /* Set receive timeout */
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    lwip_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    /* Enable broadcast */
    int broadcast = 1;
    lwip_setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(MDNS_PORT);
    inet_aton(MDNS_MULTICAST_IP, &dest_addr.sin_addr);

    for (int retry = 0; retry <= MDNS_MAX_RETRIES; retry++) {
        ssize_t sent = lwip_sendto(sock, query, query_len, 0,
                                   (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (sent < 0) {
            printf("[MDNS_Q] sendto failed (retry %d)\n", retry);
            continue;
        }

        printf("[MDNS_Q] sent query for %s (attempt %d)\n", hostname, retry + 1);

        /* Wait for response */
        while (1) {
            ssize_t recvd = lwip_recvfrom(sock, response, sizeof(response), 0, NULL, NULL);
            if (recvd < 0) {
                break; /* timeout or error */
            }
            if (parse_mdns_response(response, recvd, result) == 0) {
                printf("[MDNS_Q] resolved %s -> %s\n",
                       hostname, ipaddr_ntoa(result));
                ret = 0;
                goto done;
            }
        }
    }

    printf("[MDNS_Q] failed to resolve %s after %d attempts\n",
           hostname, MDNS_MAX_RETRIES + 1);

done:
    lwip_close(sock);
    return ret;
}

/* Main API: resolve hostname to IP */
int ha_resolve_host(const char *host, ip_addr_t *result, uint32_t timeout_ms)
{
    if (!host || host[0] == '\0' || !result) {
        return -1;
    }

    /* Direct IP address — no resolution needed */
    if (is_ip_address(host)) {
        if (ipaddr_aton(host, result)) {
            return 0;
        }
        return -1;
    }

    /* Check cache first */
    if (cache_lookup(host, result) == 0) {
        printf("[RESOLVE] cache hit for %s -> %s\n", host, ipaddr_ntoa(result));
        return 0;
    }

    int ret;

    /* .local hostname — use mDNS query */
    int host_len = strlen(host);
    if (host_len > 6 && strcmp(host + host_len - 6, ".local") == 0) {
        ret = mdns_query_a(host, result, timeout_ms);
    } else {
        /* Regular hostname — use DNS resolution */
        ret = netconn_gethostbyname(host, result);
        if (ret != ERR_OK) {
            printf("[RESOLVE] DNS failed for %s: %d\n", host, ret);
            ret = -1;
        } else {
            ret = 0;
        }
    }

    /* Cache successful result */
    if (ret == 0) {
        cache_add(host, result);
    }

    return ret;
}
