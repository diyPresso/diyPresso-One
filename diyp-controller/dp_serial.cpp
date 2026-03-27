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
    - PUT settings temperature=98.50,P=7.00,I=0.30,D=80.00,dffFactorPct=90.00,ffHeat=3.00,ffReady=10.00,tareWeight=0.00,trimWeight=0.00,preInfusionTime=3.00,infusionTime=1.00,extractionTime=25.00,extractionWeight=0.00,commissioningDone=1,shotCounter=5,wifiMode=0
    or e.g. PUT settings temperature=98.00,commissioningDone=1
    - GET serialOutputConfig
    - PUT serialOutputConfig DP_PID_STATE=1
    - GET boiler
    - PUT boiler powerControlMode=STATIC,staticPower=5.0
    - PUT boiler powerControlMode=PID
    - GET boilerPidAutotune
    - PUT boilerPidAutotune start
    - PUT boilerPidAutotune cancel
    - PUT boilerPidAutotune apply


*/

#include "dp_serial.h"
#include "dp.h"
#include "dp_hardware.h"
#include "dp_brew.h"
#include "dp_boiler.h"
#include "dp_reservoir.h"
#include "dp_pid.h"
#include "dp_heater.h"
#include "dp_machine.h"
#include "dp_commission.h"

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

    if (receivedData.startsWith("GET info")) {
        send_info();
    } else if (receivedData.startsWith("GET settings")) {
        send_settings();
    } else if (receivedData.startsWith("PUT settings ")) {
        put_settings(receivedData.substring(String("PUT settings ").length()));
    } else if (receivedData.startsWith("GET serialOutputConfig")) {
        get_serial_output_config();
    } else if (receivedData.startsWith("PUT serialOutputConfig ")) {
        put_serial_output_config(receivedData.substring(String("PUT serialOutputConfig ").length()));
    } else if (receivedData.startsWith("GET boilerPidAutotune")) {
        get_boiler_pid_autotune();
    } else if (receivedData.startsWith("PUT boilerPidAutotune ")) {
        put_boiler_pid_autotune(receivedData.substring(String("PUT boilerPidAutotune ").length()));
    } else if (receivedData.startsWith("GET boiler")) {
        get_boiler();
    } else if (receivedData.startsWith("PUT boiler ")) {
        put_boiler(receivedData.substring(String("PUT boiler ").length()));
    } else {
        send("unknown command: " + receivedData);
    }
}

void DpSerial::send_info() {
    send("diyPresso " + String(model_name()));
    send("model=" + String(model_name()));
    send("firmwareVersion=" + String(SOFTWARE_VERSION));
    send("hardwareVersion=" + String(HARDWARE_REVISION));
    send("buildDate=" + String(BUILD_DATE));
    send("machineState=" + String(machineController.get_state_name()));
    send("commissioningState=" + String(commissioningProcess.get_state_name()));
    send("commissioningError=" + String(commissioningProcess.get_error_text()));
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

void DpSerial::get_serial_output_config() {
    send("DP_PID_STATE=" + String(boilerController.get_serial_output() ? "1" : "0"));
    send("GET serialOutputConfig OK");
}

void DpSerial::put_serial_output_config(String value) {
    // Expecting a string like "DP_PID_STATE=1" or "DP_PID_STATE=0"
    if (value.startsWith("DP_PID_STATE=")) {
        String stateStr = value.substring(String("DP_PID_STATE=").length());
        if (stateStr == "1") {
            boilerController.set_serial_output(true);
            send("PUT serialOutputConfig OK");
        } else if (stateStr == "0") {
            boilerController.set_serial_output(false);
            send("PUT serialOutputConfig OK");
        } else {
            send("PUT serialOutputConfig NOK, invalid value for DP_PID_STATE: " + stateStr);
        }
    } else {
        send("PUT serialOutputConfig NOK, unknown key: " + value);
    }
}

void DpSerial::get_boiler() {
    send("powerControlMode=" + boilerController.get_power_control_mode_str());
    send("staticPower=" + String(boilerController.get_power_static(), 2));
    send("GET boiler OK");
}

void DpSerial::put_boiler(String value) {
    // Parse key-value pairs separated by commas
    // Example: "powerControlMode=STATIC,staticPower=5.0"
    
    value.trim();
    bool success = true;
    String errorMsg = "";
    
    int startPos = 0;
    while (startPos < value.length()) {
        int commaPos = value.indexOf(',', startPos);
        if (commaPos == -1) {
            commaPos = value.length();
        }
        
        String pair = value.substring(startPos, commaPos);
        pair.trim();
        
        int equalsPos = pair.indexOf('=');
        if (equalsPos == -1) {
            success = false;
            errorMsg = "Invalid format, missing '=' in: " + pair;
            break;
        }
        
        String key = pair.substring(0, equalsPos);
        String val = pair.substring(equalsPos + 1);
        key.trim();
        val.trim();
        key.toLowerCase();
        
        if (key == "powercontrolmode") {
            if (!boilerController.set_power_control_mode_str(val)) {
                success = false;
                errorMsg = "Invalid powerControlMode value: " + val;
                break;
            }
        } else if (key == "staticpower") {
            double power = val.toDouble();
            if (power < 0.0 || power > 100.0) {
                success = false;
                errorMsg = "staticPower must be between 0.0 and 100.0, got: " + val;
                break;
            }
            boilerController.set_power_static(power);
        } else {
            success = false;
            errorMsg = "Unknown key: " + key;
            break;
        }
        
        startPos = commaPos + 1;
    }
    
    if (success) {
        send("PUT boiler OK");
    } else {
        send("PUT boiler NOK, " + errorMsg);
    }
}

void DpSerial::get_boiler_pid_autotune() {
    autotune_state_t state = boilerController.getAutoTuneState();
    String stateStr;
    
    switch(state) {
        case AUTOTUNE_IDLE: stateStr = "IDLE"; break;
        case AUTOTUNE_RUNNING: stateStr = "RUNNING"; break;
        case AUTOTUNE_SUCCESS: stateStr = "SUCCESS"; break;
        case AUTOTUNE_FAILED_TIMEOUT: stateStr = "FAILED_TIMEOUT"; break;
        case AUTOTUNE_FAILED_NO_OSCILLATION: stateStr = "FAILED_NO_OSCILLATION"; break;
        default: stateStr = "UNKNOWN"; break;
    }
    
    send("state=" + stateStr);
    
    // If complete, also send the results
    if (state == AUTOTUNE_SUCCESS) {
        AutoTuneResults results = boilerController.getAutoTuneResults();
        if (results.isValid) {
            send("Kp=" + String(results.Kp, 3));
            send("Ki=" + String(results.Ki, 3));
            send("Kd=" + String(results.Kd, 3));
            send("Ku=" + String(results.Ku, 3));
            send("Pu=" + String(results.Pu, 3));
        } else {
            send("isValid=false");
        }
    }
    
    send("GET boilerPidAutotune OK");
}

void DpSerial::put_boiler_pid_autotune(String value) {
    value.trim();
    value.toLowerCase();
    
    if (value == "start") {
        // Start auto-tune with default relay amplitude
        boilerController.startAutoTune();
        send("PUT boilerPidAutotune OK, Auto-tune started");
    } else if (value == "cancel") {
        boilerController.cancelAutoTune();
        send("PUT boilerPidAutotune OK, Auto-tune cancelled");
    } else if (value == "apply") {
        AutoTuneResults results = boilerController.getAutoTuneResults();
        if (results.isValid) {
            boilerController.set_pid(results.Kp, results.Ki, results.Kd); //TODO: store in settings.
            send("PUT boilerPidAutotune OK, Applied Kp=" + String(results.Kp, 3) + 
                 " Ki=" + String(results.Ki, 3) + " Kd=" + String(results.Kd, 3));
        } else {
            send("PUT boilerPidAutotune NOK, No valid results to apply");
        }
    } else {
        send("PUT boilerPidAutotune NOK, Unknown command (use: start/cancel/apply)");
    }
}

void DpSerial::print_state()
{
  static unsigned long prev_time = millis();
  if (millis() - prev_time > 500)
  {
    Serial.print("setpoint:");
    Serial.print(boilerController.set_temp());
    Serial.print(", power:");
    Serial.print(heaterDevice.power());
    Serial.print(", average:");
    Serial.print(heaterDevice.average());
    Serial.print(", act_temp:");
    Serial.print(boilerController.act_temp());
    Serial.print(", boiler-state:");
    Serial.print(boilerController.get_state_name());
    Serial.print(", boiler-error:");
    Serial.print(boilerController.get_error_text());
    Serial.print(", brew-state:");
    Serial.print(brewProcess.get_state_name());
    Serial.print(", weight:");
    Serial.print(brewProcess.weight());
    Serial.print(", end_weight:");
    Serial.print(brewProcess.end_weight());
    Serial.print(", reservoir_level:");
    Serial.print(reservoir.level());
    Serial.print(", reservoir_weight:");
    Serial.print(reservoir.weight());
    Serial.println("");
    prev_time = millis();
  }
}