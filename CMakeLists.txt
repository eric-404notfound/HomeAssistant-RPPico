
# Bibliothek erstellen
add_library(Pico_HomeAssistant STATIC
    HomeAssistant.cpp
)

target_link_libraries(Pico_HomeAssistant
    Pico_Networking
)