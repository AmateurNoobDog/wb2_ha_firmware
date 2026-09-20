#include <stdio.h>
#include <string.h>
#include <lwip/apps/mdns.h>
#include <mdns_server.h>
#include <wifi_mgmr_ext.h>

#include "ha_mdns.h"
#include "app_config.h"

#define AND_SVC_TYPE "_and"
#define AND_SVC_PORT TCP_SERVER_PORT

static void ha_mdns_txt_cb(struct mdns_service *service, void *txt_userdata)
{
    mdns_resp_add_service_txtitem(service, "type=" DEVICE_TYPE,
                                  sizeof("type=" DEVICE_TYPE) - 1);
    mdns_resp_add_service_txtitem(service, "name=" DEVICE_NAME,
                                  sizeof("name=" DEVICE_NAME) - 1);
}

void ha_mdns_start(void)
{
    struct netif *netif = wifi_mgmr_sta_netif_get();
    if (!netif) {
        printf("[MDNS] netif not ready\n");
        return;
    }

    uint8_t mac[6];
    char hostname[32];
    if (wifi_mgmr_sta_mac_get(mac) == 0) {
        snprintf(hostname, sizeof(hostname), "and-%s-%02X%02X%02X",
                 DEVICE_TYPE, mac[3], mac[4], mac[5]);
    } else {
        snprintf(hostname, sizeof(hostname), "and-%s-unknown", DEVICE_TYPE);
    }

    mdns_resp_init();
    if (mdns_resp_add_netif(netif, hostname, 3600) != 0) {
        printf("[MDNS] add netif failed\n");
        mdns_resp_deinit();
        return;
    }

    mdns_resp_add_service(netif, hostname, AND_SVC_TYPE,
                          DNSSD_PROTO_TCP, AND_SVC_PORT, 3600,
                          ha_mdns_txt_cb, NULL);

    printf("[MDNS] %s.local (%s port %d)\n",
           hostname, AND_SVC_TYPE, AND_SVC_PORT);
}
