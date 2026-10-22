idf_component_register(SRCS "wifi.c" INCLUDE_DIRS "include"
    REQUIRES esp_wifi_remote configuration PRIV_REQUIRES ui esp_hosted esp_wifi esp_event esp_netif)
