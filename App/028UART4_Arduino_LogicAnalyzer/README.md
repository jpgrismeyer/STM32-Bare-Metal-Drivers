# 028 - UART4 Arduino Logic Analyzer Test

This example sends a continuous UART signal through the Arduino connector so it can be captured with a logic analyzer.

Status: pending logic analyzer validation.

## Target Pins

- UART peripheral: UART4
- Arduino D1 / TX: PA0 / UART4_TX
- Arduino D0 / RX: PA1 / UART4_RX
- Optional trigger marker: PB14 LED

For a basic capture, connect:

- Logic analyzer GND -> board GND
- Logic analyzer channel 0 -> Arduino D1

## UART Configuration

- Baud rate: 9600
- Data bits: 8
- Parity: none
- Stop bits: 1
- Logic level: 3.3 V

## Expected Signal

The firmware repeatedly sends:

- `UART4 TX PA0 ARD.D1 9600 8N1`
- four bytes of `0x55`
- four bytes of `0xA5`

`0x55` is useful on a logic analyzer because it creates an alternating bit pattern.

## Notes

The STM32L475 Discovery IoT Node maps Arduino D1/D0 to UART4:

- D1 -> PA0 -> UART4_TX
- D0 -> PA1 -> UART4_RX

This is separate from the ST-LINK Virtual COM Port, which uses USART1 on PB6/PB7.
