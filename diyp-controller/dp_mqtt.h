#ifndef DP_MQTT_H
#define DP_MQTT_H

#include "dp.h"
#include <ArduinoMqttClient.h>

class MqttDevice
{
    private:
      bool _on=false;
      typedef enum  mqtt_state_t { MSG_START, MSG_NEXT };
      mqtt_state_t _state = MSG_START;
      void prepare(char *measurement);
    public:
      MqttDevice() { off(); }
      void on(void) { _on=true;  }
      void off() { _on = false; }
      bool is_on(void) { return _on; }
      void init();
      void run();
      void write(char *measurement, long value);
      void write(char *measurement, double value);
      void write(char *measurement, char *value);
      void send();

};

extern MqttDevice mqttDevice;

#endif // DP_MQTT_H
