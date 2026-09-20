#ifndef __HA_MDNS_QUERY_H__
#define __HA_MDNS_QUERY_H__

#include <lwip/ip_addr.h>

/*
 * Resolve a hostname to an IP address.
 *
 * - If host is a dotted-decimal IP (e.g. "192.168.1.50"), parse directly.
 * - If host ends with ".local", use mDNS query (UDP 224.0.0.251:5353).
 * - Otherwise, use standard DNS resolution.
 *
 * Returns 0 on success, -1 on failure.
 */
int ha_resolve_host(const char *host, ip_addr_t *result, uint32_t timeout_ms);

#endif /* __HA_MDNS_QUERY_H__ */
