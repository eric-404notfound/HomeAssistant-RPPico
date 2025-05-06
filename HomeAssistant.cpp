
#include "HomeAssistant.h"



namespace HomeAssistant_Wrapper {

    int64_t reconect_callback(alarm_id_t id, void* arg) {
        HomeAssistant_MQTT* home = (HomeAssistant_MQTT*)arg;
        MQTT_CLIENT_DATA_T* mqtt_client = home->get_mqtt_client();
        home->connect();
        return 0;
    }
    
    void publish_callback(void* arg, err_t err) {

        if (err == ERR_OK) {
            #ifdef NET_MQTT_DEBUG
            printf("Nachricht veröffentlicht: %.100s\n", (char*)arg);
            #endif
        } else {
            printf("Fehler:%d beim Veröffentlichen der Nachricht: %.100s\n", err, (char*)arg);
        }
        
        delete (char*)arg;
        
    }

    void incoming_publish_callback(void* arg, const char* topic, u32_t tot_len) {
        if(tot_len == 0) {
            return;
        }
        if (topic == NULL) {
            return;
        }
        if (tot_len >= MQTT_TOPIC_LEN) {
            printf("Topic length exceeds maximum limit\n");
            return;
        }
        printf("Incoming data on topic: %.100s\n", topic);
        HomeAssistant_MQTT* home = (HomeAssistant_MQTT*)arg;
        home->set_incoming_topic(topic);
    }

    void incoming_data_callback(void* arg, const u8_t* data, u16_t len, u8_t flags) {
        HomeAssistant_MQTT* home = (HomeAssistant_MQTT*)arg;

        mqtt_handler_t handlerstrukt = home->getHandler();
        if (handlerstrukt.handler == NULL) {
            printf("No handler registered for topic");
            printf("payload: %.*s\n", len, data);
            return;
        }
        mqtt_handler_fn_t handler = handlerstrukt.handler;
        handler(handlerstrukt.arg, (char*)data, len);
    }

    static int reconnect_try = 0;
    void connect_callback(mqtt_client_t* client, void* arg, mqtt_connection_status_t status) {
        HomeAssistant_MQTT* home = (HomeAssistant_MQTT*)arg;
        MQTT_CLIENT_DATA_T* mqtt_client = home->get_mqtt_client();
        if (status == 0) {
            printf("MQTT client connected\n");
            mqtt_client->connected = true;
            reconnect_try = 0;
            home->registerHandlers();
        }else if (status == 1) {
            printf("MQTT_CONNECT_REFUSED_PROTOCOL_VERSION\n");
            mqtt_client->connected = false;
        } else if (status == 2) {
            printf("MQTT_CONNECT_REFUSED_IDENTIFIER\n");
            mqtt_client->connected = false;
        } else if (status == 3) {
            printf("MQTT_CONNECT_REFUSED_SERVER\n");
            mqtt_client->connected = false;
        } else if (status == 4) {
            printf("MQTT_CONNECT_REFUSED_USERNAME_PASS\n");
            mqtt_client->connected = false;
        } else if (status == 5) {
            printf("MQTT_CONNECT_REFUSED_NOT_AUTHORIZED\n");
            mqtt_client->connected = false;
        } else if (status == 256) {
            printf("MQTT_CONNECT_DISCONNECTED\n");
            if (reconnect_try < 5) {
                add_alarm_in_ms(2000, reconect_callback, arg, true);
            }
            reconnect_try++;
            mqtt_client->connected = false;
        } else if (status == 257) {
            printf("MQTT_CONNECT_TIMEOUT\n");
            mqtt_client->connected = false;
        }
    }


    void subscribe_callback(void* arg, err_t err) {

        if (err == ERR_OK) {
            printf("Subscribed to topic: %.100s\n", (const char*)arg);
        } else {
            printf("Error subscribing to topic: %.100s\n", (const char*)arg);
        }
        free(arg);
    }

    void unsubscribe_callback(void* arg, err_t err) {
        if (err == ERR_OK) {
            printf("Unsubscribed from topic: %.100s\n", (const char*)arg);
        } else {
            printf("Error unsubscribing from topic: %.100s\n", (const char*)arg);
        }
        free(arg);
    }
}

HomeAssistant_MQTT::HomeAssistant_MQTT(const char *_host, uint16_t _port, const char *_client_id):hostname(_host), mqtt_client(NULL) {
    

    mqtt_client = new_mqtt_client(hostname, _port, _client_id);
    if (!mqtt_client) {
        panic("MQTT client instance creation error");
    };

    for (int i = 0; i < MAX_MQTT_HANDLERS; i++) {
        handlers[i].topic = NULL;
        handlers[i].handler = NULL;
        handlers[i].arg = NULL;
    }
    

};

void HomeAssistant_MQTT::registerHandler(const char *_topic, mqtt_handler_fn_t _handler, void* _arg) {
    if (strlen(_topic) >= MQTT_TOPIC_LEN) {
        printf("Topic length exceeds maximum limit\n");
        return;
    }
    
    for (int i = 0; i < MAX_MQTT_HANDLERS; i++) {
        if (handlers[i].topic == NULL || strcmp(handlers[i].topic, _topic) == 0) {

            handlers[i].topic = new char[strlen(_topic) + 1];
            strcpy(handlers[i].topic, _topic);

            handlers[i].handler = _handler;
            handlers[i].arg = _arg;

            if(mqtt_client->connected == false) {
                return;
            }

            char *callback_info = (char *)calloc(strlen(_topic) + 1, sizeof(char));
            strcpy(callback_info, _topic);

            if (mqtt_sub_unsub(mqtt_client->mqtt_client_inst, _topic, 0, HomeAssistant_Wrapper::subscribe_callback, (void*)callback_info, 1) != ERR_OK){
                printf("Error subscribing to topic: %.100s\n", _topic);
                free(callback_info);
            }
            return;
        }
    }
    printf("No free handler slot available\n");
};

void HomeAssistant_MQTT::registerHandlers() {
    for (int i = 0; i < MAX_MQTT_HANDLERS; i++) {
        if (handlers[i].topic != NULL) {

            char *callback_info = (char *)calloc(strlen(handlers[i].topic) + 1, sizeof(char));
            strcpy(callback_info, handlers[i].topic);

            if (mqtt_sub_unsub(mqtt_client->mqtt_client_inst, handlers[i].topic, 0, HomeAssistant_Wrapper::subscribe_callback, (void*)callback_info, 1) != ERR_OK){
                printf("Error subscribing to topic: %.100s\n", handlers[i].topic);
                free(callback_info);
            }
        }
    }
};

void HomeAssistant_MQTT::unregisterHandler(const char *_topic) {
    for (int i = 0; i < MAX_MQTT_HANDLERS; i++) {
        if (handlers[i].topic != NULL && strcmp(handlers[i].topic, _topic) == 0) {
            if (handlers[i].topic != NULL)
                delete[] handlers[i].topic;
            handlers[i].handler = NULL;
            char *callback_info = (char *)calloc(strlen(_topic) + 1, sizeof(char));
            strcpy(callback_info, _topic);

            if (mqtt_sub_unsub(mqtt_client->mqtt_client_inst, _topic, 0, HomeAssistant_Wrapper::unsubscribe_callback, (void*)callback_info, 0) != ERR_OK)
                printf("Error unsubscribing from topic: %.100s\n", _topic);
            break;
        }
    }
};

mqtt_handler_t HomeAssistant_MQTT::getHandler_byTopic(const char* _topic){
    for (int i = 0; i < MAX_MQTT_HANDLERS; i++) {
        if (handlers[i].topic != NULL) {
            if(strcmp(handlers[i].topic, _topic) == 0)    
                return handlers[i];
        }
    }
    return {NULL, NULL, NULL};
}

mqtt_handler_t HomeAssistant_MQTT::getHandler() {
    return getHandler_byTopic(incoming_topic);
};

mqtt_handler_fn_t HomeAssistant_MQTT::getHandlerfn_byTopic(const char *_topic) {
    for (int i = 0; i < MAX_MQTT_HANDLERS; i++) {
        if (handlers[i].topic != NULL && strcmp(handlers[i].topic, _topic) == 0) {
            return handlers[i].handler;
        }
    }
    return NULL;
};
mqtt_handler_fn_t HomeAssistant_MQTT::getHandlerfn() {
    return getHandlerfn_byTopic(incoming_topic);
};


void HomeAssistant_MQTT::publish(const char *_topic, const char *_payload) {
    if (mqtt_client == NULL) {
        printf("MQTT client not initialized\n");
        return;
    }
    if (mqtt_client->mqtt_client_inst == NULL) {
        printf("MQTT client instance not created\n");
        return;
    }
    char *callback_info = new char[strlen(_topic) + 1];
    strcpy(callback_info, _topic);
    cyw43_arch_lwip_begin();
    err_t err = mqtt_publish(this->mqtt_client->mqtt_client_inst, _topic, _payload, strlen(_payload), 0, 1, HomeAssistant_Wrapper::publish_callback, (void*)callback_info);
    cyw43_arch_lwip_end();
    if (err != ERR_OK) {
        printf("Error publishing message: %d\n", err);
        printf("Topic: %.100s\n", _topic);
        printf("Payload: %s\n", _payload);
        printf("Length: %d\n", strlen(_payload));
    }
};  


void HomeAssistant_MQTT::setUsernamePassword(const char *_username, const char *_password) {

    mqtt_set_username_password(mqtt_client, _username, _password);
    mqtt_client->mqtt_client_info.keep_alive = 60;

};
void HomeAssistant_MQTT::connect(){

    start_client(mqtt_client, HomeAssistant_Wrapper::connect_callback, HomeAssistant_Wrapper::incoming_publish_callback, HomeAssistant_Wrapper::incoming_data_callback, (void*)this);

}

void HomeAssistant_MQTT::set_incoming_topic(const char* topic) {
    if (topic == NULL) {
        return;
    }
    if (strlen(topic) >= MQTT_TOPIC_LEN) {
        printf("Topic length exceeds maximum limit\n");
        return;
    }

    strncpy((char*)incoming_topic, topic, sizeof(incoming_topic));
    incoming_topic[sizeof(incoming_topic) - 1] = '\0'; // Null-terminate
}
void HomeAssistant_MQTT::set_tls_config(const char* cert){
    mqtt_set_tls_config(mqtt_client, cert);
}

HomeAssistant_MQTT::~HomeAssistant_MQTT(){
    if (mqtt_client) {
        free_mqtt_client(mqtt_client);
        mqtt_client = NULL;
    }
}

