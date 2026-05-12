# USART Command Console With I2C WHOAMI

This application extends the USART GPIO command console by adding I2C sensor validation.

Commands are sent from the PC through the ST-LINK Virtual COM Port. The STM32 receives each command, parses it, and uses custom bare-metal drivers to control GPIO or read the `WHO_AM_I` register from the onboard LPS22HB pressure sensor.

## Hardware

- Board: B-L475E-IOT01A2
- MCU: STM32L475
- USART: USART1
- USART pins: PB6 / PB7
- Interface: ST-LINK Virtual COM Port
- Baud rate: 9600
- Serial format: 8 data bits, no parity, 1 stop bit
- Flow control: None
- User LED: PB14
- User button: PC13
- I2C peripheral: I2C2
- I2C pins: PB10 / PB11
- Sensor: LPS22HB
- LPS22HB address: 0x5D
- WHO_AM_I register: 0x0F
- Expected WHO_AM_I value: 0xB1

## Purpose

The goal of this demo is to use USART as a command interface for validating both GPIO and I2C bare-metal drivers on real hardware.

This demo verifies that the firmware can:

- receive commands from the PC through USART
- parse line-based commands
- transmit command responses back to the PC
- control a GPIO output pin
- read a GPIO input pin
- perform an I2C register write followed by an I2C register read
- validate a real onboard sensor through its identity register

## Supported Commands

- PING -> PONG
- ECHO hello -> hello
- LED ON -> OK
- LED OFF -> OK
- BUTTON? -> PRESSED or RELEASED
- I2C WHOAMI -> 0xB1

## What This Adds

Compared to the USART GPIO command console, this version adds I2C sensor validation through the same USART command interface.

The command `I2C WHOAMI` writes the register address `0x0F` to the LPS22HB sensor and then reads one byte back using a repeated-start I2C transaction.

## Validation Path

PC terminal -> ST-LINK VCP -> USART1 RX -> command parser -> I2C driver -> LPS22HB sensor -> USART1 TX -> PC terminal

## How to Test

Open a serial terminal such as PuTTY, Tera Term, or a Python serial script using the serial settings above.

Example session:

- Send: PING
- Expected response: PONG

- Send: ECHO hello
- Expected response: hello

- Send: LED ON
- Expected response: OK

- Send: LED OFF
- Expected response: OK

- Send: BUTTON?
- Expected response: PRESSED or RELEASED

- Send: I2C WHOAMI
- Expected response: 0xB1

## Notes

- This demo uses polling-based USART transmit and receive functions.
- This demo uses polling-based I2C master transmit and receive functions.
- Commands are processed after a line ending is received.
- Both carriage return and newline can be used as command terminators.
- I2C2 uses HSI16 as its kernel clock source.
- USART1 uses HSI16 as its kernel clock source.
- This command interface is a foundation for future hardware-in-the-loop tests using Python and pytest.
