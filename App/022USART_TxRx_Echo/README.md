USART Echo Demo
This application extends the USART Ping-Pong demo by adding a simple echo command.

The STM32 receives a command line from the PC through the ST-LINK Virtual COM Port, parses it, and sends a response back using a custom bare-metal USART driver.

Hardware
Item	Configuration
Board	B-L475E-IOT01A2
MCU	STM32L475
USART	USART1
USART pins	PB6 / PB7
Interface	ST-LINK Virtual COM Port
Baud rate	115200
Serial format	8 data bits, no parity, 1 stop bit
Flow control	None
Purpose
The goal of this demo is to validate line-based serial communication over USART.

This demo verifies that the USART driver and application layer can:

receive characters from the PC
store received data in a command buffer
detect line endings
parse simple text commands
transmit fixed and variable-length responses
Supported Commands
Command	Expected Response
PING	PONG
ECHO hello	hello
What This Adds
Compared to the USART Ping-Pong demo, this version adds:

a receive buffer
command parsing by line
support for commands with arguments
variable-length USART responses
Validation Path
PC terminal -> ST-LINK VCP -> USART1 RX -> command buffer -> command parser -> USART1 TX -> PC terminal

How to Test
Open a serial terminal such as PuTTY, Tera Term, or a Python serial script using the serial settings above.

Example session:

Send: PING

Expected response: PONG

Send: ECHO hello

Expected response: hello

Notes
This demo uses polling-based USART transmit and receive functions.
Commands are processed after a line ending is received.
Both carriage return and newline can be used as command terminators.
This command interface will be reused to validate other bare-metal drivers.