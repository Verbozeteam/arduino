# arduino

Arduino controller code

# Communication Protocol
All communication happens by sending messages in the following format: `[code (1 byte)][message length N (1 byte)][message (N bytes)]`. Message codes are the following:

## Messages Arduino sends
### Code 0: Pin value update `[0][3][pin_type][pin_index][reading]`
- pin_type: 0 for digital pin, 1 for analog pin, 2 for virtual pin
- pin_index: Index of the pin
- reading: Current value at the pin

## Messages accepted by Arduino
### Code 1: Set pin mode `[1][3][pin_type][pin_index][mode]`
- pin_type: 0 for digital pin, 1 for analog pin, 2 for virtual pin
- pin_index: Index of the pin
- mode: 0 (input) or 1 (output)

### Code 2: Set pin output `[2][3][pin_type][pin_index][output]`
- pin_type: 0 for digital pin, 1 for analog pin, 2 for virtual pin
- pin_index: Index of the pin
- output: output value (0/1 for digital, or 0-255 for PWM)

### Code 3: Read pin input `[3][2][pin_type][pin_index]`
The reading will be returned via a message (code 0) sent by the arduino
- pin_type: 0 for digital pin, 1 for analog pin, 2 for virtual pin
- pin_index: Index of the pin

### Code 4: Register pin listener `[4][6][pin_type][pin_index][interval (4 bytes UNSIGNED LITTLE ENDIAN)]`
Makes the Arduino report the pin's reading every interval
- pin_type: 0 for digital pin, 1 for analog pin, 2 for virtual pin
- pin_index: Index of the pin
- interval: interval (in ms) at which the Arduino reads the pin (-1 means don't read)
