# USART TxRx Ping-Pong Demo

This application validates the basic USART TX/RX path on the STM32L475 Discovery IoT board using a simple command-response interface.

The PC sends a command through the ST-LINK Virtual COM Port, and the STM32 responds using a custom bare-metal USART driver.

## Hardware

| Item | Configuration |
|---|---|
| Board | B-L475E-IOT01A2 |
| MCU | STM32L475 |
| USART | USART1 |
| USART pins | PB6 / PB7 |
| Interface | ST-LINK Virtual COM Port |
| Baud rate | 9600 |
| Serial format | 8 data bits, no parity, 1 stop bit |
| Flow control | None |

## Purpose

The goal of this demo is to verify that the USART driver can:

- initialize USART1
- configure the GPIO alternate function pins
- configure the USART baud rate
- receive data from the PC
- transmit data back to the PC

## Supported Commands

| Command | Expected Response |
|---|---|
| `PING` | `PONG` |

## Validation Path

This demo validates the complete communication path:

```text
PC terminal -> ST-LINK VCP -> USART1 RX -> command parser -> USART1 TX -> PC terminal

How to Test
Open a serial terminal such as PuTTY, Tera Term, or a Python serial script using the serial settings above.

Send: PING
Expected response:PONG


Notes
This demo uses polling-based USART transmit and receive functions.
USART receive uses timeout handling to avoid blocking forever.
The USART clock source is currently configured as HSI16.
The baud rate register is calculated by the driver from a 16 MHz USART clock.