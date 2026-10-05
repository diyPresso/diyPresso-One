#include "dp_mqtt.h"
#include <WiFiNINA.h>
#include "dp_boiler.h"
#include "dp_brew.h"
#include "dp_reservoir.h"
#include "dp_heater.h"
#include "dp_settings.h"


MqttDevice mqttDevice;


WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);


const char broker[] = "test.mosquitto.org";
int port = 1883;
char topic[64]  = "";


void mac_to_hex(char *hex, byte *mac)
{
    static const char *hexchar = "0123456789ABCDEF";
    for(int i=0; i<6; i++)
    {
        hex[2*i+0] = hexchar[ (mac[i] >> 4) ];
        hex[2*i+1] = hexchar[ (mac[i] & 15) ];
    }
    hex[12] = 0;
}

void MqttDevice::init()
{
  byte mac[6]; // Wifi MAC address

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);
  WiFi.macAddress(mac);
  strcpy(topic, "diyPressoOne/");
  mac_to_hex(topic+strlen(topic), mac);

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    return;
  }
  on();
  
  Serial.println("You're connected to the MQTT broker! Subscribe using mosquitto with this command:");
  
  Serial.print("mosquitto_sub -h ");
  Serial.print(broker);
  Serial.print(" -t ");
  Serial.println(topic);
}


void MqttDevice::run() 
{
    if ( !is_on() )
        return;

    // call poll() regularly to allow the library to send MQTT keep alive which avoids being disconnected by the broker
    mqttClient.poll();
}

void MqttDevice::prepare(const char *measurement)
{
    if (_state == MSG_START)
    {
        mqttClient.beginMessage(topic);
        mqttClient.print("measurement ");
    }
    if (_state == MSG_NEXT)
        mqttClient.print(",");
    mqttClient.print(measurement);
    mqttClient.print("=");
    _state = MSG_NEXT;
}

void MqttDevice::write(const char *measurement, double value)
{
    if ( !is_on() )  return;
    prepare(measurement);
    mqttClient.print(value);
}

void MqttDevice::write(const char *measurement, const char *value)
{
    if ( !is_on() ) return;
    prepare(measurement);
    mqttClient.print("\"");
    mqttClient.print(value);
    mqttClient.print("\"");
}

void MqttDevice::write(const char *measurement, long value)
{
    if ( !is_on() ) return;
    prepare(measurement);
    mqttClient.print(value);
}

void MqttDevice::send()
{
    mqttClient.endMessage();
    _state = MSG_START;
}

void MqttDevice::send_state()
{
  static unsigned long prev_time = millis();
  if (millis() - prev_time > 5000)
  {
    write("t_set", boilerController.set_temp());
    write("t_act", boilerController.act_temp());
    write("h_pwr", heaterDevice.power());
    write("h_avg", heaterDevice.average());
    write("r_lvl", reservoir.level());
    write("r_wgt", reservoir.weight());
    write("w_cur", brewProcess.weight());
    write("w_end", brewProcess.end_weight());
    write("shots", (long)settings.shotCounter());

    write("boil", boilerController.get_state_name());
    if (boilerController.is_error())
      write("boil_err", boilerController.get_error_text());

    write("brew", brewProcess.get_state_name());
    if (brewProcess.is_error())
      write("brew_err", brewProcess.get_error_text());

    if (reservoir.is_error())
      write("res_err", reservoir.get_error_text());

    write("msec", (long)millis());
    send();

    prev_time = millis();
  }
}