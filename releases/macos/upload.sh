#!/bin/bash
set -e

# Variables
BOARD="arduino:samd:mkrwifi1010"
FIRMWARE_PATH="/Users/joop/Documents/diyPresso/git/rjvh-diyPresso-One/.pio/build/mkr_wifi1010/firmware.bin"  # Update with the binary file path
TIMEOUT_BOARD_DETECTION=30 #seconds


# Check if the Arduino CLI is installed
# if ! command -v arduino-cli &> /dev/null; then
#   echo "Error: Arduino CLI is not installed. Please install it from https://arduino.github.io/arduino-cli/latest/installation/."
#   exit 1
# fi


# Check if and Arduino board is connected
if arduino-cli board list | grep -q "MKR WiFi 1010"; then
  echo "Error: Please power off and disconnect the diyPresso machine from USB before running this script."
  exit 1
fi

# Check if the firmware file exists
if [ ! -f $FIRMWARE_PATH ]; then
  echo "Error: Firmware file not found at path: $FIRMWARE_PATH"
  exit 1
fi

# Detect the USB port
echo ""
echo "Starting script..."
echo ""
echo "Please connect the diyPresso to an USB port now."
echo ""
echo "Detecting diyPresso..."

START_TIME=$(date +%s)
while true; do
  PORT=$(arduino-cli board list | grep "MKR WiFi 1010" | awk '{print $1}')
  
  if [ -n "$PORT" ]; then
    echo "diyPresso detected on port: $PORT"
    break
  fi

  CURRENT_TIME=$(date +%s)
  ELAPSED_TIME=$((CURRENT_TIME - START_TIME))

  if [ $ELAPSED_TIME -ge $TIMEOUT_BOARD_DETECTION ]; then
    echo "Error: Could not detect an diyPresso within $TIMEOUT seconds. Ensure the diyPresso is POWERED OFF and connect it via USB after staring this script."
    exit 1
  fi
  sleep 0.1
done

echo ""
echo "Retrieving current settings..."

# Define the output file for settings with date and time appended
SETTINGS_FILE="settings_$(date +'%Y%m%d_%H%M%S').txt"

# Start monitoring
arduino-cli monitor -q -p $PORT --fqbn $BOARD | while IFS= read -r line
do
    # Store lines with "<variable_name>=<value>" to the settings file
    if [[ "$line" =~ ^[a-zA-Z_]+= ]]; then
        echo "$line" >> $SETTINGS_FILE
    fi

    # Stop monitoring at the first "setpoint" line
    if [[ "$line" == setpoint:* ]]; then
        # if the brew state is idle, then the commissioning is done
        if [[ "$line" == *"brew-state:idle"* ]]; then 
            # add to settings file, if not already present
            if ! grep -q "commissioningDone" $SETTINGS_FILE; then 
              echo "commissioningDone=1" >> $SETTINGS_FILE
            fi
        fi
        break # stop monitoring
    fi
done

#check if the settings file is created and has atleast 12 lines
if [ -f $SETTINGS_FILE ] && [ $(wc -l < $SETTINGS_FILE) -ge 12 ]; then
    echo "Settings saved to file: $SETTINGS_FILE"
else
    echo "Error: Settings not retrieved successfully. Exiting..."
    exit 1
fi



# Upload the firmware
# echo ""
# echo "Uploading firmware..."
# arduino-cli upload -p $PORT --fqbn $BOARD --input-file $FIRMWARE_PATH

# echo "Firmware upload completed!"
