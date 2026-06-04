# STM32 Bare-Metal Drivers

Bare-metal peripheral drivers for the STM32L47xx family, developed from scratch using register-level programming without STM32 HAL or LL libraries.

The project focuses on understanding how embedded drivers work underneath higher-level abstractions: memory-mapped registers, clock configuration, GPIO alternate functions, peripheral flags, bus protocols, and hardware validation on a real board.

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

- `Inc/`: MCU register definitions and peripheral driver headers
- `Src/`: peripheral driver implementations
- `App/`: bare-metal demo applications used to validate each driver
- `Docs/`: milestone notes and driver validation summaries
- `tests/hil/`: Python hardware-in-the-loop tests using pytest and pyserial

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
- `029SPI1_Arduino_LogicAnalyzer`

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
- `024USART_TxRx_I2C_WHOAMI`
- `025USART_RxInterrupt_RingBuffer`
- `026USART_RxInterrupt_Callback`
- `027USART_Interrupt_CommandConsole`
- `028UART4_Arduino_LogicAnalyzer`

## Current Validation

The drivers are validated through small bare-metal applications running on the STM32L475 board.

Current validation includes:

- GPIO output control
- GPIO input reading
- external interrupt handling
- SPI transmit examples
- SPI data-size configuration through STM32L4 `CR2.DS`
- SPI blocking transmit/receive with timeout-capable status APIs
- I2C master transmit and receive
- I2C sensor register reading
- USART transmit and receive
- USART command console for GPIO and I2C validation
- USART RX interrupt handling with ring buffer
- USART RX interrupt handling through driver callbacks
- interrupt-driven USART command console using the reusable ring buffer
- USART parity, framing, noise and overrun error detection
- UART4 signal generation on Arduino D1 for logic analyzer validation
- SPI1 signal generation on Arduino D13/D11/D10 for logic analyzer validation
- Python hardware-in-the-loop tests over the ST-LINK Virtual COM Port

## USART Command Console

The USART examples use the ST-LINK Virtual COM Port as a command interface between the PC and the board.

Example commands:

- `PING` -> `PONG`
- `ECHO hello` -> `hello`
- `LED ON` -> `OK`
- `LED OFF` -> `OK`
- `BUTTON?` -> `PRESSED` or `RELEASED`
- `I2C WHOAMI` -> `0xB1`
- `USART STATUS` -> `ISR=0x... BRR=0x... ERR=0x...`

The `I2C WHOAMI` command validates the onboard LPS22HB pressure sensor by reading its `WHO_AM_I` register through the custom I2C driver.

The `USART STATUS` command reports selected USART diagnostic values from the running firmware.

## USART Interrupt Reception

The USART examples evolve from polling-based reception to interrupt-driven reception:

- polling receive using `USART_ReceiveData()`
- direct RX interrupt handling in `USART1_IRQHandler()`
- driver-level IRQ handling through `USART_IRQHandling()`
- application notification through `USART_ApplicationEventCallback()`
- ring-buffer based command processing in the main loop
- interrupt-driven command console combining GPIO and I2C validation

This keeps the interrupt handler short while allowing the application to process complete commands outside interrupt context.

## Hardware-In-The-Loop Tests

The project includes pytest-based hardware-in-the-loop tests that communicate with the board over the ST-LINK Virtual COM Port.

Install dependencies:

```powershell
python -m pip install -r requirements.txt
```

Run the tests:

```powershell
$env:STM32_PORT='COM4'
$env:STM32_BAUD='9600'
python -m pytest -m hil tests\hil -v
```

The current HIL test suite validates:

- `PING`
- `ECHO hello`
- `LED ON`
- `LED OFF`
- `BUTTON?`
- `I2C WHOAMI`
- `USART STATUS`

## Milestones

- [USART driver milestone](Docs/USART_Milestone.md)
- [SPI driver milestone](Docs/SPI_Milestone.md)

## Development Approach

This project is built incrementally:

1. Define the MCU memory map and peripheral register structures.
2. Implement low-level peripheral drivers.
3. Validate each driver with small hardware examples.
4. Combine drivers through command-based demo applications.
5. Add automated hardware-in-the-loop tests using Python and pytest.

## Roadmap

Planned next steps:

- expand SPI validation
- add more sensor validation commands
- add DMA-based examples
- explore FreeRTOS integration with custom drivers

## Notes

This project is intended as a learning and portfolio project focused on low-level firmware development.

The goal is not to replace STM32 HAL in production projects, but to understand what happens underneath it and build a strong foundation in embedded driver design.

## Contact

I am developing this project as part of my path toward embedded systems and firmware engineering roles.

Feel free to connect or reach out if you are interested in bare-metal development, STM32, RTOS, or embedded software engineering.

- Name: Juan Pablo Grismeyer
- LinkedIn: https://www.linkedin.com/in/juan-pablo-grismeyer-a392a0187/
- Email: juampagrismeyer@gmail.com
