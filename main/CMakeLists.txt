idf_component_register(SRCS "main.c"
                            "wifi.c"
                            "http.c"
                    INCLUDE_DIRS ".")

add_subdirectory(lib/dht22)
target_link_libraries(${COMPONENT_LIB} PUBLIC dht22-esp-idf)