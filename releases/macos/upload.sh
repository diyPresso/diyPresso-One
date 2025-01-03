#!/bin/bash
set -e

#change directory to the script location
cd "$(dirname "$0")"

# Variables
BOARD="arduino:samd:mkrwifi1010"
FIRMWARE_PATH="./firmware.bin" 
#FIRMWARE_PATH="../../.pio/build/mkr_wifi1010/firmware.bin" 
TIMEOUT_BOARD_DETECTION=30 #seconds
SERIAL_LOG_FILE="/tmp/diyPresso_serial_log.txt"
ARDUINO_CLI_PATH="./arduino-cli --config-dir ."

# UPLOAD_ONLY=false
# # Parse arguments
# while [[ "$#" -gt 0 ]]; do
#     case $1 in
#         --upload-only) UPLOAD_ONLY=true ;;
#         # Add more options here as needed
#         *) echo "Unknown parameter passed: $1"; exit 1 ;;
#     esac
#     shift
# done

# Check if the Arduino CLI is installed
if ! command -v $ARDUINO_CLI_PATH &>/dev/null; then
    echo "Error: arduino-cli not found"
    echo "Press any key to exit"
    read -n 1 -s
    exit 1
fi

# Check if the firmware file exists
if [ ! -f $FIRMWARE_PATH ]; then
    echo "Error, firmware file not found at path: $FIRMWARE_PATH"
    echo "Press any key to exit"
    read -n 1 -s
    exit 1
fi

# Check if the smad platform is installed, if not install
if ! $ARDUINO_CLI_PATH core list | grep -q "arduino:samd"; then
    echo "Arduino platform files not yet installed, downloading and installing arduino:samd platform..."
    $ARDUINO_CLI_PATH core install arduino:samd
fi

# Check if and Arduino board is connected
if $ARDUINO_CLI_PATH board list | grep -q "MKR WiFi 1010"; then
    echo "Error: Please power off and disconnect the diyPresso machine from USB before starting this script."
    echo "Press any key to exit"
    read -n 1 -s
    exit 1
fi

###### Detect the USB port
echo ""
echo "Starting script..."
echo ""
echo "Looking for diyPresso..."
echo "Please connect the diyPresso to an USB port now."

START_TIME=$(date +%s)
while true; do
    PORT=$($ARDUINO_CLI_PATH board list | grep "MKR WiFi 1010" | awk '{print $1}')

    if [ -n "$PORT" ]; then
        echo "diyPresso detected on port: $PORT"
        break
    fi

    CURRENT_TIME=$(date +%s)
    ELAPSED_TIME=$((CURRENT_TIME - START_TIME))

    if [ $ELAPSED_TIME -ge $TIMEOUT_BOARD_DETECTION ]; then
        echo "Error: Could not detect an diyPresso within $TIMEOUT seconds. Ensure the diyPresso is POWERED OFF and connect it via USB after staring this script."
        echo "Press any key to exit"
        read -n 1 -s
        exit 1
    fi
    sleep 0.1
done

###### Retrieve the current settings from the device
echo ""
echo "Retrieving current settings..."

# Define the output file for settings with date and time appended
SETTINGS_FILE="settings_$(date +'%Y%m%d_%H%M%S').txt"

# Start monitoring
$ARDUINO_CLI_PATH monitor -q -p $PORT --fqbn $BOARD | while IFS= read -r line; do
    # Store lines with "<variable_name>=<value>" to the settings file
    if [[ "$line" =~ ^[a-zA-Z_]+= ]]; then
        echo "$line" >>$SETTINGS_FILE
    fi

    # Stop monitoring at the first "setpoint" line
    if [[ "$line" == setpoint:* ]]; then
        # if the brew state is idle, then the commissioning is done
        if [[ "$line" == *"brew-state:idle"* ]]; then
            # add to settings file, if not already present
            if ! grep -q "commissioningDone" $SETTINGS_FILE; then
                echo "commissioningDone=1" >>$SETTINGS_FILE
            fi
        fi
        break # stop monitoring
    fi
done

#check if the settings file is created and has at least 12 lines
if [ -f $SETTINGS_FILE ] && [ $(wc -l <$SETTINGS_FILE) -ge 12 ]; then
    echo "Settings saved to file: $SETTINGS_FILE"
else
    echo "Error: Settings not retrieved successfully. Exiting..."
    echo "Press any key to exit"
    read -n 1 -s
    exit 1
fi

##### check is settings version is compatible
# settings file should have version=1 or no line with version at all
if grep -q "version" $SETTINGS_FILE; then
    if ! grep -q "version=1" $SETTINGS_FILE; then
        echo "Error: Incompatible settings version. Your machine is already updated or this is an old version of the update script."
        echo "Press any key to exit"
        read -n 1 -s
        exit 1
    fi
fi

###### Upload the firmware
echo ""
echo "Uploading firmware..."
$ARDUINO_CLI_PATH upload -p $PORT --fqbn $BOARD --input-file $FIRMWARE_PATH

echo "Firmware upload completed!"

####
echo ""

sleep 1

###### Restore the settings to the devices
echo "Restoring settings..."

# Read the settings file; Skip lines which start with 'version=' or 'crc='; Remove carrage returns; Replace newlines with commas; Remove trailing comma.
SETTINGS_COMMAND="PUT settings $(grep -v -e '^version=' -e '^crc=' $SETTINGS_FILE | tr -d '\r' | tr '\n' ',' | sed 's/,$//')"

#SETTINGS_COMMAND="PUT settings temperature=100.00,P=7.00,I=0.30,D=80.00,ff_heat=3.00,ff_ready=10.00,ff_brew=80.00,tareWeight=0.00,trimWeight=0.00,preInfusionTime=3.00,infuseTime=1.00,extractTime=25.00,extractionWeight=0.00,commissioningDone=1,wifiMode=0"

RESPONSE_OK="PUT SETTINGS OK"
RESPONSE_NOK="PUT SETTINGS NOK"

echo "Connecting..."

#Open serial port and send command using screen
#Replace "cu." with "tty." to get the serial port for communication
PORT_SERIAL=$(echo $PORT | sed 's/cu./tty./')
screen -S diyPresso -d -m $PORT_SERIAL 115200

# remove any previous log file
rm -f $SERIAL_LOG_FILE

# Save serial out put to a log file
screen -S diyPresso -p 0 -X logfile $SERIAL_LOG_FILE
screen -S diyPresso -p 0 -X log on

sleep 5 #wait for the serial port to open and the board to initialize

echo "Sending settings"
screen -S diyPresso -p 0 -X stuff "$SETTINGS_COMMAND"
sleep 0.1

# Check for the response
ERROR=1
TIMEOUT=15 #seconds
FLUSH_BUFFER_TIMEOUT=3 #seconds
START_TIME=$(date +%s)

while true; do
    if grep -q "$RESPONSE_OK" $SERIAL_LOG_FILE || grep -q "$RESPONSE_NOK" $SERIAL_LOG_FILE; then
        break
    fi

    CURRENT_TIME=$(date +%s)
    ELAPSED_TIME=$((CURRENT_TIME - START_TIME))

    #Flush buffer if no response after X seconds
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
    echo "Press any key to exit"
    read -n 1 -s
    exit 1
fi

rm -f $SERIAL_LOG_FILE #Only remove the log file if there is no error

echo ""
echo "Your diyPresso has been successfully updated. Enjoy your coffee!"
echo "Press any key to exit"
read -n 1 -s
exit 0
