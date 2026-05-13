# USART RX Interrupt Ring Buffer Demo

This application validates interrupt-driven USART reception using a simple ring buffer.

The PC sends commands through the ST-LINK Virtual COM Port. Each received byte triggers the USART1 interrupt, the interrupt handler stores the byte in a ring buffer, and the main loop processes complete command lines.

## Hardware

- Board: B-L475E-IOT01A2
- MCU: STM32L475
- USART: USART1
- USART pins: PB6 / PB7
- Interface: ST-LINK Virtual COM Port
- Baud rate: 9600
- Serial format: 8 data bits, no parity, 1 stop bit
- Flow control: None

## Purpose

The goal of this demo is to move USART reception from polling to interrupt-driven operation.

This demo verifies that the firmware can:

- enable the USART1 RXNE interrupt
- enable the USART1 interrupt line in NVIC
- receive bytes in `USART1_IRQHandler`
- store incoming bytes in a ring buffer
- process complete commands in the main loop
- transmit responses back to the PC

## Supported Commands

- PING -> PONG
- ECHO hello -> hello

## Validation Path

PC terminal -> ST-LINK VCP -> USART1 RX interrupt -> ring buffer -> command parser -> USART1 TX -> PC terminal

## Notes

- Reading `RDR` inside the interrupt handler clears the RXNE flag.
- The interrupt handler only stores bytes and returns quickly.
- Command parsing remains in the main loop.
- This pattern is more scalable than blocking on `USART_ReceiveData()` in the main loop.
