#include "HomeAssistant.h"
#include "string.h"

static const char discovery_topic[] = "homeassistant/lock/Haustuer/config";
static const char entity_type[] = 
"{"
    "\"name\": \"Haustuer\","
    "\"device\": {"
        "\"identifiers\": [\"Pico_Lock_Haustuer\"],"
        "\"name\": \"Pico2-404notfound\","
        "\"manufacturer\": \"Eric Seifer\","
        "\"model\": \"RP2350 with WiFi\","
        "\"sw_version\": \"1.0\""
    "}"
"}";


static char* entity_definitions[] = {
    "{"
        "\"name\":\"Haustuer\","
        "\"unique_id\":\"Haustuer_lock_\","
        "\"command_topic\":\"HomeAutomation/lock/Haustuer/unlock_command\","
        "\"state_topic\":\"HomeAutomation/lock/Haustuer/lock_state\","
        "\"payload_lock\":\"LOCK\","
        "\"payload_unlock\":\"UNLOCK\","
        "\"state_locked\":\"LOCKED\","
        "\"state_unlocked\":\"UNLOCKED\","
  "}"
};
static size_t entity_definitions_count = sizeof(entity_definitions) / sizeof(entity_definitions[0]);

