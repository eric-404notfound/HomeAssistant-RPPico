
# Bibliothek erstellen
add_library(Pico_HomeAssistant STATIC
    HomeAssistant.cpp
    Format.cpp
)

target_link_libraries(Pico_HomeAssistant
    Pico_Networking
)