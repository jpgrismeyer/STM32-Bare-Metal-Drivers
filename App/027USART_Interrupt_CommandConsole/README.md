# 027 - USART Interrupt Command Console

This example combines the USART RX interrupt callback design with a small command console.

Incoming bytes are received by the USART interrupt handler, forwarded to the application callback, stored in a ring buffer, and processed in the main loop as complete commands.

## Hardware

- Board: STM32L475 Discovery IoT Node
- USART1 TX/RX: PB6/PB7 through ST-LINK Virtual COM Port
- LED: PB14
- User button: PC13
- I2C2: PB10/PB11
- Sensor: LPS22HB pressure sensor

## Serial Configuration

- Baud rate: 9600
- Data bits: 8
- Parity: none
- Stop bits: 1
- Line ending: CR, LF, or CRLF

## Supported Commands

- `PING` -> `PONG`
- `ECHO <text>` -> `<text>`
- `LED ON` -> `OK`
- `LED OFF` -> `OK`
- `BUTTON?` -> `PRESSED` or `RELEASED`
- `I2C WHOAMI` -> `0xB1`

## Firmware Flow

1. USART RX receives a byte and sets RXNE.
2. NVIC calls `USART1_IRQHandler()`.
3. The ISR calls `USART_IRQHandling()`.
4. The driver reads `RDR` and calls `USART_ApplicationEventCallback()`.
5. The application stores the received byte in a ring buffer.
6. The main loop reads the ring buffer and processes complete commands.

This keeps the interrupt path short while allowing the application to handle higher-level command parsing outside the ISR.
