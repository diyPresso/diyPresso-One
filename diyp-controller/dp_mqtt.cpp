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

const long interval = 4000;
unsigned long previousMillis = 0;
int count = 0;


void mac_to_hex(char *hex, byte *mac)
{
    static const char *hexchar = "0123456789ABCDEF";
    for(int i=0; i<6; i++)
    {
        hex[2*i+0] = hexchar[ (mac[i] >> 4) ];
        hex[2*i+1] = hexchar[ (mac[1] & 15) ];
    }
    hex[13] = 0;
}

void MqttDevice::init()
{
  byte mac[6]; // Wifi MAC address

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);
  WiFi.macAddress(mac);
  char topic_buf[64];
  strcpy(topic, "diyPressoOne/");
  mac_to_hex(topic+strlen(topic), mac);

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
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
    static int count = 0;
    unsigned long currentMillis = millis();
    if ( !is_on() )
        return;

    // call poll() regularly to allow the library to send MQTT keep alive which avoids being disconnected by the broker
    mqttClient.poll();

    /* if (currentMillis - previousMillis >= interval) 
    {
        previousMillis = currentMillis;
        Serial.print("Sending message to topic: ");
        Serial.println(topic);
        Serial.println(count++);
        mqttClient.beginMessage(topic);
        mqttClient.print("counter count=");
        mqttClient.print(count);
        mqttClient.endMessage();
    } */
}

void MqttDevice::prepare(char *measurement)
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

void MqttDevice::write(char *measurement, double value)
{
    if ( !is_on() )  return;
    prepare(measurement);
    mqttClient.print(value);
}

void MqttDevice::write(char *measurement, char *value)
{
    if ( !is_on() ) return;
    prepare(measurement);
    mqttClient.print("\"");
    mqttClient.print(value);
    mqttClient.print("\"");
}

void MqttDevice::write(char *measurement, long value)
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

    write("boil", (char *)boilerController.get_state_name());
    if (boilerController.is_error())
      write("boil_err", (char *)boilerController.get_error_text());

    write("brew", (char *)brewProcess.get_state_name());
    if (brewProcess.is_error())
      write("brew_err", (char *)brewProcess.get_error_text());

    if (reservoir.is_error())
      write("res_err", (char *)reservoir.get_error_text());

    write("msec", (long)millis());
    send();

    prev_time = millis();
  }
}