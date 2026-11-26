idf_component_register(
    SRCS "main.cpp" "web_server.cpp" "mixer_state.cpp"
    INCLUDE_DIRS "."
    REQUIRES arduino yummydsp arduinojson nvs_flash esp_wifi esp_event esp_netif esp_http_server
)
