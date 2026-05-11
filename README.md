# STM32 Bare-Metal Drivers

Bare-metal peripheral drivers for the STM32L47xx family, developed from scratch using register-level programming without STM32 HAL or LL libraries.

The project is focused on understanding how embedded drivers work underneath higher-level abstractions: memory-mapped registers, clock configuration, GPIO alternate functions, peripheral flags, bus protocols, and hardware validation.

## Target Hardware

- Board: B-L475E-IOT01A2 Discovery IoT Node
- MCU: STM32L475VG
- Core: ARM Cortex-M4
- Debug/programming interface: ST-LINK
- Serial interface: ST-LINK Virtual COM Port

## Implemented Drivers

- GPIO
- EXTI
- SPI
- I2C
- USART
- RCC helper functions

## Project Structure

- `Inc/`  
  Header files for MCU register definitions and peripheral drivers.

- `Src/`  
  Source files implementing the driver logic.

- `App/`  
  Example applications used to validate each driver on real hardware.

## Example Applications

### GPIO

- `001Led_Toggle`
- `002led_button`
- `003led_button_ext`

### EXTI

- `005button_interrupt`

### SPI

- `006SPI_TxOnly`
- `007SPI_TxSTM32_RxEsp32`
- `008SPIcmd_Handling`

### I2C

- `010I2C_LedToggle`
- `011I2C_MasterTx_testing`
- `012I2C_MasterRx_testing`
- `013I2C_MasterRx_Sensor`
- `014I2C_MasterTxRx_OLED`
- `015I2C_MasterRx_Debug`

### USART

- `020USART_TxRxecho`
- `021USART_TxRx_PingPong`
- `022USART_TxRx_Echo`
- `023USART_TxRx_GPIO`

## Current Validation

The drivers are validated through small bare-metal applications running on the STM32L475 board.

Current validation includes:

- GPIO output control
- GPIO input reading
- external interrupt handling
- SPI transmit examples
- I2C master transmit and receive
- I2C sensor register reading
- USART transmit and receive
- USART command console for GPIO validation

## USART Command Console

The USART examples use the ST-LINK Virtual COM Port as a simple command interface between the PC and the board.

Example commands:

- `PING` -> `PONG`
- `ECHO hello` -> `hello`
- `LED ON` -> `OK`
- `LED OFF` -> `OK`
- `BUTTON?` -> `PRESSED` or `RELEASED`

This command interface will be reused to validate other drivers and sensors.

## Development Approach

This project is built incrementally:

1. Define MCU memory map and peripheral register structures.
2. Implement low-level peripheral drivers.
3. Validate each driver with small hardware examples.
4. Combine drivers through simple application-level demos.
5. Add automated hardware-in-the-loop tests using Python and pytest.

## Roadmap

Planned next steps:

- add USART command for I2C sensor validation
- add Python/pytest hardware-in-the-loop tests
- improve USART receive using interrupts and ring buffers
- expand SPI validation
- add DMA-based examples
- explore FreeRTOS integration with custom drivers

## Notes

This project is intended as a learning and portfolio project focused on low-level firmware development.

The goal is not to replace STM32 HAL in production projects, but to understand what happens underneath it and build a strong foundation in embedded driver design.

## Contact

I am developing this project as part of my path toward embedded systems and firmware engineering roles.

Feel free to connect or reach out if you are interested in bare-metal development, STM32, RTOS, or embedded software engineering.


* **Name:** Juan Pablo Grismeyer
* **LinkedIn:** https://www.linkedin.com/in/juan-pablo-grismeyer-a392a0187/
* **Email:** juampagrismeyer@gmail.com


