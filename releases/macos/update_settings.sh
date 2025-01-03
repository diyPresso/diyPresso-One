#!/bin/bash
set -e

SERIAL_LOG_FILE="/tmp/diyPresso_serial_log.txt"


RESPONSE_OK="PUT SETTINGS OK"
RESPONSE_NOK="PUT SETTINGS NOK"

SETTINGS_FILE="settingsPre162Commissioned.txt"

if grep -q "version" $SETTINGS_FILE; then
    if ! grep -q "version=1" $SETTINGS_FILE; then
        echo "Error: Incompatible settings version. Your machine is already updated or this is an old version of the update script."
        echo "press any key to exit"
        read -n 1 -s
        exit 1
    fi
fi


# Read the settings file, skip lines which start with 'version=' or 'crc='. remove carrage returns and replace newlines with commas. 
SETTINGS_COMMAND="PUT settings $(grep -v -e '^version=' -e '^crc=' $SETTINGS_FILE | tr -d '\r' | tr '\n' ',' | sed 's/,$//')"

echo "SETTINGS_COMMAND: $SETTINGS_COMMAND"

echo "start"
#TODO no static port

PORT=$(arduino-cli board list | grep "MKR WiFi 1010" | awk '{print $1}')
echo "PORT: $PORT"

#Replace cu. with tty. to get the serial port for communication
PORT_SERIAL=$(echo $PORT | sed 's/cu./tty./')
echo "PORT_SERIAL: $PORT_SERIAL"

#open serial port and send command using screen
screen -S diyPresso -d -m $PORT_SERIAL 115200
 #wait for the serial port to open and the board to initialize

rm -f $SERIAL_LOG_FILE # remove any previous log file
screen -S diyPresso -p 0 -X logfile $SERIAL_LOG_FILE
screen -S diyPresso -p 0 -X log on

sleep 1

echo "sending settings"
screen -S diyPresso -p 0 -X stuff "$SETTINGS_COMMAND"

sleep 0.1

# Check the response
ERROR=0
TIMEOUT=15
FLUSH_BUFFER_TIMEOUT=3

START_TIME=$(date +%s)
while true; do
    if grep -q "$RESPONSE_OK" $SERIAL_LOG_FILE || grep -q "$RESPONSE_NOK" $SERIAL_LOG_FILE; then
        break
    fi

    CURRENT_TIME=$(date +%s)
    ELAPSED_TIME=$((CURRENT_TIME - START_TIME))

    #flush buffer if no response after X seconds
    if [ $ELAPSED_TIME -gt $FLUSH_BUFFER_TIMEOUT ]; then
        screen -S diyPresso -p 0 -X log off
        screen -S diyPresso -p 0 -X log on
        break
    fi

    if [ $ELAPSED_TIME -gt $TIMEOUT ]; then
        echo "Error: Timeout while waiting for response"
        break
    fi

    sleep 0.5
done

if grep -q "$RESPONSE_OK" $SERIAL_LOG_FILE; then
    echo "Settings restored successfully"
    ERROR=0
elif grep -q "$RESPONSE_NOK" $SERIAL_LOG_FILE; then
    echo "Error: Settings not restored successfully"
    grep "$RESPONSE_NOK" $SERIAL_LOG_FILE
    echo "A backup of the device settings has been saved to $SETTINGS_FILE"

else
    echo "Error: Settings not restored successfully, no response."
    echo "A backup of the device settings has been saved to $SETTINGS_FILE"
fi

# Clean up
screen -S diyPresso -X log off
screen -S diyPresso -X quit

if [ $ERROR -eq 1 ]; then
    exit 1
fi

#rm -f $SERIAL_LOG_FILE #Only remove the log file if there is no error

echo "done"
