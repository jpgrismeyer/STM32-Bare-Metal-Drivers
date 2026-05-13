# USART RX Interrupt Callback Demo

This application validates interrupt-driven USART reception using a driver-level IRQ handler, an application callback, and a reusable ring buffer module.

The PC sends commands through the ST-LINK Virtual COM Port. Each received byte triggers the USART1 interrupt, the application ISR calls the USART driver IRQ handler, and the driver notifies the application through `USART_ApplicationEventCallback()`. The callback stores the received byte in a reusable ring buffer, while the main loop processes complete command lines.

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

The goal of this demo is to separate USART interrupt flag handling from application-specific command processing.

This demo verifies that the firmware can:

- enable the USART1 RXNE interrupt
- enable the USART1 interrupt line in NVIC
- handle RXNE inside `USART_IRQHandling()`
- pass received bytes to the application through `USART_ApplicationEventCallback()`
- store incoming bytes in a reusable ring buffer module
- process complete commands in the main loop
- transmit responses back to the PC

## Supported Commands

- PING -> PONG
- ECHO hello -> hello

## Validation Path

PC terminal -> ST-LINK VCP -> USART1 RX interrupt -> USART driver IRQ handler -> application callback -> ring buffer -> command parser -> USART1 TX -> PC terminal

## Notes

- Reading `RDR` inside the driver IRQ handler clears the RXNE flag.
- The application ISR only calls `USART_IRQHandling()`.
- The application callback only stores bytes and returns quickly.
- Command parsing remains in the main loop.
- The ring buffer implementation is shared through `ring_buffer.h` and `ring_buffer.c`.
- This pattern is more scalable than blocking on `USART_ReceiveData()` in the main loop.
