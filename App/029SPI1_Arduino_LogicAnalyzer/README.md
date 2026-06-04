# 029 - SPI1 Arduino Logic Analyzer Test

This example sends repeated SPI frames through the Arduino connector so the SPI driver can be inspected with a logic analyzer.

The application uses `SPI_SendDataWithStatus()` so the transmit path can fail with a timeout instead of blocking forever if the expected SPI flag is not reached.

Status: pending logic analyzer validation.

## Target Pins

- SPI peripheral: SPI1
- Arduino D13 / SCK: PA5 / SPI1_SCK
- Arduino D11 / MOSI: PA7 / SPI1_MOSI
- Arduino D10 / CS marker: PA2 / software-controlled GPIO

For a basic capture, connect:

- Logic analyzer GND -> board GND
- Logic analyzer CH0 -> Arduino D10 / CS
- Logic analyzer CH1 -> Arduino D13 / SCK
- Logic analyzer CH2 -> Arduino D11 / MOSI

MISO is not used in this transmit-only validation example.

## SPI Configuration

- Mode: master
- Direction: full-duplex configuration, transmit-only usage
- Data size: 8 bits
- Clock polarity: CPOL = 0
- Clock phase: CPHA = 0
- Software slave management: enabled
- Software chip select marker: active low on Arduino D10

## Expected Data

The firmware repeatedly sends:

- ASCII frame: `SPI1 ARD D13 SCK D11 MOSI`
- byte pattern: `0x55 0xA5 0xF0 0x0F`

The byte pattern is useful because it contains alternating, grouped-high and grouped-low bit patterns.

## PulseView Decoder Settings

- Protocol decoder: SPI
- CS: channel connected to Arduino D10
- CLK: channel connected to Arduino D13
- MOSI: channel connected to Arduino D11
- MISO: not used
- CPOL: 0
- CPHA: 0
- Bit order: MSB first
- Word size: 8 bits

## Notes

Arduino D13 / PA5 is also connected to LED1 on this board, so do not use LED1 as a marker while capturing SPI1 SCK.
