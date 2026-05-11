USART Command Console With GPIO
This application extends the USART Echo demo by adding GPIO output control and GPIO input reading.

Commands are sent from the PC through the ST-LINK Virtual COM Port. The STM32 receives each command, parses it, and uses custom bare-metal drivers to control the user LED or read the user button.

Hardware
Board: B-L475E-IOT01A2
MCU: STM32L475
USART: USART1
USART pins: PB6 / PB7
Interface: ST-LINK Virtual COM Port
Baud rate: 115200
Serial format: 8 data bits, no parity, 1 stop bit
Flow control: None
User LED: PB14
User button: PC13
Purpose
The goal of this demo is to use USART as a simple command interface for validating other bare-metal drivers.

This demo verifies that the firmware can:

receive commands from the PC through USART
parse line-based commands
transmit command responses back to the PC
control a GPIO output pin
read a GPIO input pin
combine USART and GPIO drivers in the same application
Supported Commands
PING -> PONG
ECHO hello -> hello
LED ON -> OK
LED OFF -> OK
BUTTON? -> PRESSED or RELEASED
What This Adds
Compared to the USART Echo demo, this version adds GPIO validation through the USART command console.

The PC can now trigger hardware actions on the board and read simple hardware state using text commands.

Validation Path
PC terminal -> ST-LINK VCP -> USART1 RX -> command parser -> GPIO driver -> LED/button hardware -> USART1 TX -> PC terminal

How to Test
Open a serial terminal such as PuTTY, Tera Term, or a Python serial script using the serial settings above.

Example session:

Send: PING

Expected response: PONG

Send: ECHO hello

Expected response: hello

Send: LED ON

Expected response: OK

Send: LED OFF

Expected response: OK

Send: BUTTON?

Expected response: PRESSED or RELEASED

Debugging Note
During development, the LED did not turn on even though the GPIO output data register changed correctly.

The issue was caused by the GPIO output type configuration. PB14 was left configured as open-drain instead of push-pull.

For the user LED, PB14 must be configured as a push-pull output. The fix was to clear the corresponding OTYPER bit when configuring the pin:

OTYPER bit = 0 -> push-pull
OTYPER bit = 1 -> open-drain
This reinforced an important rule when configuring peripheral registers:

clear the target bits first, then write the new configuration.

Notes
This demo uses polling-based USART transmit and receive functions.
Commands are processed after a line ending is received.
Both carriage return and newline can be used as command terminators.
GPIO output control is validated through the user LED.
GPIO input reading is validated through the user button.
This command interface will be reused to validate I2C and SPI drivers in later demos.



