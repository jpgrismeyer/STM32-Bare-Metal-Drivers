# USART Driver Milestone

This document summarizes the current USART driver milestone in the STM32 bare-metal driver stack.

## Goal

Build and validate a USART driver from register-level configuration up to interrupt-driven command handling on real STM32L475 hardware.

## Implemented Driver Features

- USART peripheral clock enable/disable
- TX/RX mode configuration
- word length configuration
- stop bit configuration
- parity configuration
- hardware flow control configuration
- baud rate setup through `BRR`
- blocking transmit using `USART_SendData()`
- blocking receive using `USART_ReceiveData()`
- RX interrupt enable
- driver-level IRQ handler through `USART_IRQHandling()`
- weak application callback through `USART_ApplicationEventCallback()`
- RX byte forwarding through `USART_Handle_t.RxByte`
- parity, framing, noise and overrun error detection
- driver error code storage through `USART_Handle_t.ErrorCode`
- error flag clearing through the USART `ICR` register

## Application Validation Path

The USART examples evolve incrementally:

- `020USART_TxRxecho`: basic transmit/receive experiment
- `021USART_TxRx_PingPong`: command-response using polling
- `022USART_TxRx_Echo`: echo command validation
- `023USART_TxRx_GPIO`: USART command control for GPIO
- `024USART_TxRx_I2C_WHOAMI`: USART command console validating I2C sensor access
- `025USART_RxInterrupt_RingBuffer`: RX interrupt with local ring buffer
- `026USART_RxInterrupt_Callback`: driver-level IRQ handling with weak callback
- `027USART_Interrupt_CommandConsole`: interrupt-driven command console with GPIO, I2C and USART diagnostics
- `028UART4_Arduino_LogicAnalyzer`: UART4 signal generation on Arduino D1 for external logic analyzer validation

## Interrupt-Driven Console Flow

The `027USART_Interrupt_CommandConsole` application uses this flow:

1. A byte arrives through USART RX.
2. The USART peripheral sets `RXNE`.
3. NVIC calls `USART1_IRQHandler()`.
4. The ISR calls `USART_IRQHandling(&USART1Handle)`.
5. The driver reads `RDR` into `USART_Handle_t.RxByte`.
6. The driver calls `USART_ApplicationEventCallback()`.
7. The application stores the byte in a reusable ring buffer.
8. The main loop reads the ring buffer and parses complete commands.

The ISR and callback stay short. Blocking operations, command parsing and response generation are handled in the main loop.

## Supported Console Commands

- `PING` -> `PONG`
- `ECHO <text>` -> `<text>`
- `LED ON` -> `OK`
- `LED OFF` -> `OK`
- `BUTTON?` -> `PRESSED` or `RELEASED`
- `I2C WHOAMI` -> `0xB1`
- `USART STATUS` -> `ISR=0x... BRR=0x... ERR=0x...`

## Error Handling

The driver detects these USART errors:

- `PE`: parity error
- `FE`: framing error
- `NE`: noise error
- `ORE`: overrun error

When an error is detected:

1. The driver reads the hardware status from `ISR`.
2. The corresponding bits are stored in `USART_Handle_t.ErrorCode`.
3. The hardware flags are cleared through `ICR`.
4. The application is notified through the weak callback.
5. The main loop reports the error over the serial console.

The application intentionally does not transmit from inside the callback, because `USART_SendData()` is blocking and should not run inside the interrupt path.

## Hardware-In-The-Loop Validation

Python HIL tests use `pytest` and `pyserial` over the ST-LINK Virtual COM Port.

Current validation includes:

- command-response behavior
- echo behavior
- GPIO command responses
- button query response
- I2C `WHO_AM_I` sensor read
- USART diagnostic response format

Run:

```powershell
$env:STM32_PORT='COM4'
$env:STM32_BAUD='9600'
python -m pytest -m hil tests\hil\test_usart_interrupt_command_console.py -v
```

## Pending External Validation

`028UART4_Arduino_LogicAnalyzer` is prepared for external signal validation with PulseView or another logic analyzer.

Planned validation:

- capture UART4 TX on Arduino D1 / PA0
- decode `9600 8N1`
- verify ASCII output
- verify `0x55` and `0xA5` byte patterns
- compare measured bit time against the expected 9600 baud timing

Status: pending logic analyzer validation.
