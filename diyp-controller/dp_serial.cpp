/*
    Handles serial communication with the controller
    - send data and receive commands

    (c) 2025 - diyEspresso - rjvh - CC-BY-NC


    To connect to the serial bus on MacOS:
    screen /dev/tty.usbmodem11301 115200
    (exit with ctrl + a, ctrl + \)

    supported commands:
    - GET info
    - GET settings
    - PUT settings temperature=98.50,P=7.00,I=0.30,D=80.00,ff_heat=3.00,ff_ready=10.00,ff_brew=80.00,tareWeight=0.00,trimWeight=0.00,preInfusionTime=3.00,infuseTime=1.00,extractTime=25.00,extractionWeight=0.00,commissioningDone=1,shotCounter=5,wifiMode=0
    or e.g. PUT settings temperature=98.00,commissioningDone=1

*/

#include "dp_serial.h"
#include "dp.h"
#include "dp_hardware.h"
#include "dp_brew.h"
#include "dp_boiler.h"
#include "dp_reservoir.h"

//initialize the class
DpSerial dpSerial(115200);

DpSerial::DpSerial(unsigned long baudRate) : _baudRate(baudRate) {}

void DpSerial::begin() {
    Serial.begin(_baudRate);
    while (!Serial) {
        ; // Wait for serial port to connect. Needed for native USB port only
    }
}

/* Cuts up a string into lines and sends them to the serial bus
*/
void DpSerial::send(const String &data) {
    int start = 0;
    int end = data.indexOf('\n');
    while (end != -1) {
        Serial.println(data.substring(start, end));
        start = end + 1;
        end = data.indexOf('\n', start);
    }
    // Send the last part if there is no newline at the end
    if (start < data.length()) {
        Serial.println(data.substring(start));
    }
}

void DpSerial::send(double data) {
    Serial.println(data);
}

void DpSerial::send(int data) {
    Serial.println(data);
}

void DpSerial::send(char data) {
    Serial.println(data);
}

void DpSerial::send(const char* data) {
    Serial.println(data);
}

/* receives commands from serial bus and prasses them

*/
void DpSerial::receive() {
    String receivedData = "";
    if (Serial.available() <= 0) {
        return;
    }

    receivedData = Serial.readStringUntil('\n');

    //GET info
    if (receivedData.startsWith("GET info")) {
        send_info();
    } else if (receivedData.startsWith("GET settings")) {
        send_settings();
    } else if (receivedData.startsWith("PUT settings "))
    {
        put_settings(receivedData.substring(String("SET settings ").length()));
    }
    

    send("echo: " + receivedData);
}

void DpSerial::send_info() {
    send("diyPresso");
    send("firmwareVersion=" + String(SOFTWARE_VERSION));
    send("hardwareVersion=" + String(HARDWARE_REVISION));
    send("buildDate=" + String(BUILD_DATE));
    send("brewProcessState=" + String(brewProcess.get_state_name()));
    send("brewProcessError=" + String(brewProcess.get_error_text()));
    send("boilerControllerState=" + String(boilerController.get_state_name()));
    send("boilerControllerError=" + String(boilerController.get_error_text()));
    send("reservoirError=" + String(reservoir.get_error_text()));
    send("GET info OK");
}

void DpSerial::send_settings() {
    send(settings.serialize());
    send("GET settings OK");
}

void DpSerial::put_settings(String value) {

    int res_deserialize = settings.deserialize(value);

    if (res_deserialize == 0) {

        send(settings.temperature());
        int res_save = settings.save(); // all good, save the settings

        if (res_save = 1) {
            send("PUT settings OK, settings saved.");
            settings.apply();
        } else if (res_save = 0) {
            send("PUT settings OK, no changes.");
        } else {
            send("PUT settings NOK, unknown return code when saving settings: " + String(res_save));
        }
    } else if (res_deserialize == -1) {
        send("PUT settings NOK, Invalid input string format: settings not saved");
    } else if (res_deserialize == -2) {
        send("PUT settings NOK, unknown key: settings not saved");
    } else {
        send("PUT settings NOK, settings not saved, unknown error code when deserializing settings: " + String(res_deserialize));
    }

}