# SPI Driver Milestone

This document summarizes the current SPI validation direction in the STM32 bare-metal driver stack.

## Goal

Build and validate a basic SPI master transmit path using register-level GPIO and SPI configuration.

## Current Driver Features

- SPI peripheral clock enable/disable
- master/slave mode configuration
- full-duplex, half-duplex and simplex configuration fields
- SPI clock prescaler configuration
- CPOL/CPHA configuration
- software slave management
- STM32L4 data-size configuration through `CR2.DS`
- blocking transmit using `SPI_SendData()`
- blocking receive using `SPI_ReceiveData()`
- timeout-capable transmit using `SPI_SendDataWithStatus()`
- timeout-capable receive using `SPI_ReceiveDataWithStatus()`
- SPI status polling through `SPI_GetFlagStatus()`

## Application Validation Path

Current SPI examples:

- `006SPI_TxOnly`: basic SPI1 transmit-only example
- `007SPI_TxSTM32_RxEsp32`: STM32-to-ESP32 SPI transfer experiment
- `008SPIcmd_Handling`: SPI command handling experiment
- `029SPI1_Arduino_LogicAnalyzer`: SPI1 signal generation for external logic analyzer validation

## Logic Analyzer Validation

`029SPI1_Arduino_LogicAnalyzer` sends repeated SPI frames through Arduino-accessible pins:

- Arduino D13 / PA5: SPI1_SCK
- Arduino D11 / PA7: SPI1_MOSI
- Arduino D10 / PA2: software chip-select marker

Expected frames:

- ASCII: `SPI1 ARD D13 SCK D11 MOSI`
- Pattern: `0x55 0xA5 0xF0 0x0F`

PulseView decoder settings:

- protocol: SPI
- CPOL: 0
- CPHA: 0
- word size: 8 bits
- bit order: MSB first
- CS: active low

Status: pending logic analyzer validation.

## Next Improvements

- verify SCK/MOSI/CS timing with PulseView
- document captured waveforms after hardware validation
- add error/status handling for `MODF`, `OVR` and `BSY`
