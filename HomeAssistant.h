
#include "../network/NetworkManager.h"

#define MAX_MQTT_HANDLERS 10



typedef void (*mqtt_handler_fn_t)(void* arg, char* payload, size_t len);
typedef struct mqtt_handler_t {
    char* topic;
    mqtt_handler_fn_t handler;
    void* arg;
} mqtt_handler_t;


class HomeAssistant_MQTT{

private:
    // MQTT server details
    const char* hostname;

    // MQTT client instance
    MQTT_CLIENT_DATA_T* mqtt_client;

    const char* discovery_topic;
    const char* discovery_payload;

    // Callback Table
    mqtt_handler_t handlers[MAX_MQTT_HANDLERS];
    char incoming_topic[MQTT_TOPIC_LEN];


public:
    HomeAssistant_MQTT(const char *host, uint16_t port, const char *client_id);
    ~HomeAssistant_MQTT();

    void registerHandler(const char* topic, mqtt_handler_fn_t handler, void* arg);
    void unregisterHandler(const char* topic);
    void registerHandlers();

    mqtt_handler_t getHandler_byTopic(const char* topic);
    mqtt_handler_t getHandler();

    mqtt_handler_fn_t getHandlerfn();
    mqtt_handler_fn_t getHandlerfn_byTopic(const char* topic);

    void set_incoming_topic(const char* topic);
    
    void publish(const char* topic, const char* payload);
    void registerEntity(const char* topic, const char* payload);

    void setup();
    void setUsernamePassword(const char* username, const char* password);
    void connect();

    MQTT_CLIENT_DATA_T* get_mqtt_client() {return mqtt_client;}

    void set_tls_config(const char* cert);
};