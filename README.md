# arduino

Arduino controller code

# Requirements
To be able to use the mega2560_256 board, you need to create a new board in your boards.txt (in the Arduino library base) which is a copy of mega2560 except the name (now mega2560_256, duh) and `mega2560.build.core=arduino` becomes `mega2560_256.build.core=arduino_bigserial`. Then you need to copy `..../hardware/arduino/cores/arduino/` to `..../hardware/arduino/cores/arduino_bigserial/` and in the new directory edit `HardwareSerial.cpp` to change `#define SERIAL_BUFFER_SIZE 64` to `#define SERIAL_BUFFER_SIZE 256`.

# Uploading the code to an Arduino
Run `./upload.sh <board>` where board is any implemented board (such as "mega2560").

# Running the emulator
The emulator requires [shammam](https://github.com/Verbozeteam/shammam) to be installed. To launch the emulator go to the `emulation` directory and run `./run_emulator.sh <board>` where board is any implemented board (such as "mega2560").

PLEASE FIX ME: The script currently requires the shammam repository to be cloned in the same directory as this repository.

# Communication Protocol
All communication happens by sending messages in the following format: `[code (1 byte)][message length N (1 byte)][message (N bytes)]`. Message codes are the following:

## Messages Arduino sends
### Code 0: Pin value update `[0][3][pin_type][pin_index][reading]`
- pin_type: 0 for digital pin, 1 for analog pin, 2 for virtual pin
- pin_index: Index of the pin
- reading: Current value at the pin

## Messages accepted by Arduino
### Code 1: Set pin mode (NON VIRTUAL PINS) `[1][3][pin_type][pin_index][mode]`
- pin_type: 0 for digital pin, 1 for analog pin, 2 for virtual pin
- pin_index: Index of the pin
- mode: If this pin is NOT virtual: 0 (input), 1 (output), 2 (PWM output), 3 (input with pullup resistor) or 4 (PWM output with smoothing)

### Code 2: Set VIRTUAL pin mode `[2][length][2][pin_index][virtual_pin_type][<pin-specific>]`
- length: Length of the following bytes (pin-specific)
- pin_index: Index of the pin
- virtual_pin_type:
    - 0: Digital temperature sensor. pin-specific info: `[temp_index]`
        - temp_index: Index of the temperature sensor on the OneWire bus
    - 1: ISR Light controller (only makes a pulse during the AC wave). pin-specific info: `[frequency][sync][pin]`
        - frequency: frequency of the ISR light (50 or 60). All virtual pins of this type MUST have the same value for their frequency.
        - sync: the pin on which SYNC happens (zero-crossing is measured). All virtual pins of this type MUST have the same value for their sync.
        - pin: pin to output the waves for this light.
    - 2: Same as ISR light (1) but this doesn't make a pulse at the right time, but rather keeps the port HIGH until next zero cross
    - 3:
    - 4: NTC temperature sensor. pin-specific info: `[analog_pin_index, k_resistance]`
        - analog_pin_index: Index of the analog pin (i.e. 0, 1, 2, ...15 on the mega) to read the sensor
        - k_resistance: resistance value used (in thousands). pass 100 for 100kohm, 20 for 20kohm, etc...
    - 5: EmonLin current sensor: `[analog_pin_index]`
        - analog_pin_index: Index of the analog pin (i.e. 0 for A0, 1 for A1, 2, ...15 for A15 on the mega) to read the sensor

### Code 3: Set pin output `[3][3][pin_type][pin_index][output]`
- pin_type: 0 for digital pin, 1 for analog pin, 2 for virtual pin
- pin_index: Index of the pin
- output: output value (0/1 for digital, or 0-255 for PWM)

### Code 4: Read pin input `[4][2][pin_type][pin_index]`
The reading will be returned via a message (code 0) sent by the arduino
- pin_type: 0 for digital pin, 1 for analog pin, 2 for virtual pin
- pin_index: Index of the pin

### Code 5: Register pin listener `[5][6][pin_type][pin_index][interval (4 bytes UNSIGNED LITTLE ENDIAN)]`
Makes the Arduino report the pin's reading every interval
- pin_type: 0 for digital pin, 1 for analog pin, 2 for virtual pin
- pin_index: Index of the pin
- interval: interval (in ms) at which the Arduino reads the pin (-1 means don't read, 0 means no interval but report on-change)

### Code 6: Reset board state `[6][0]`
Resets all pin states
