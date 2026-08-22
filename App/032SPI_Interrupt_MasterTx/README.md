# 032 - SPI Interrupt Master Tx

Same evolution the I2C driver went through in App 031: SPI transmit was polling-only
(`SPI_SendData`, used by the SD-over-SPI driver and the earlier ESP32 apps), now
`SPI_SendDataIT` kicks off a transfer and returns immediately, and `SPI_IRQHandling`
(fed from `SPI1_IRQHandler`) sends every remaining byte from the ISR.

## Hardware

- Board: B-L475E-IOT01A2 Discovery IoT Node
- USART: USART1 through ST-LINK Virtual COM Port
- SPI bus: SPI1, external module (MFRC522 RFID reader), manual CS

## Pin Mapping

| Function | Pin | Mode |
|---|---|---|
| USART1_TX | PB6 | AF7 push-pull |
| USART1_RX | PB7 | AF7 push-pull |
| SPI1_SCK | PA5 | AF5 push-pull |
| SPI1_MISO | PA6 | AF5 push-pull (wired for completeness, unused by these commands) |
| SPI1_MOSI | PA7 | AF5 push-pull |
| SPI1 CS (manual GPIO) | PA4 | Output, active low |

CS is a plain GPIO, driven manually, not hardware NSS/SSOE -- same pattern the SD-over-SPI
driver already uses in App 030. SPI runs at a deliberately slow prescaler
(`SPI_SCLK_SPEED_DIV32`) so the transaction is easy to read on a logic analyzer.

## Commands

Use a serial terminal configured as `9600 8N1`.

| Command | Response |
|---|---|
| `PING` | `PONG` |
| `HELP` | command list |
| `APP_ID?` | `SPI_INTERRUPT_MASTERTX` |
| `SPI RESET` | sends the MFRC522 SoftReset frame (`0x02 0x0F` -- write `CommandReg`=`SoftReset`) via `SPI_SendDataIT` -> `SPI_RESET STARTED` |
| `SPI SEND <hex bytes>` | sends an arbitrary space-separated hex byte list (up to 8 bytes) via `SPI_SendDataIT`, e.g. `SPI SEND 02 0F` -> `SPI_SEND STARTED LEN=0x02` |
| `SPI STATUS` | `STATE=READY`\|`BUSY_TX LAST_EVENT=<hex> LAST_LEN=<hex> DONE=0`\|`1` |

`SPI RESET` and `SPI SEND 02 0F` are equivalent -- `SPI RESET` is just the safe, idempotent
convenience command that doesn't require remembering MFRC522 register addressing.

## Design Notes

- `SPI_SendDataIT`/`SPI_ReceiveDataIT` (new in `stm32l475xx_spi_driver.c`) mirror the
  existing blocking `SPI_SendDataWithStatus`/`SPI_ReceiveDataWithStatus` in behavior --
  same 8/16-bit `DS`-register branching -- but only touch `CR2.TXEIE`/`RXNEIE` and return
  immediately. `SPI_IRQHandling` does the byte-by-byte work from the ISR, checking each
  source flag (`TXE`, `RXNE`, `OVR`) against its enable bit in turn, since SPI1/2/3 share
  one IRQ line for every event (unlike I2C's separate EV/ER lines).
- `SPI_ApplicationEventCallback` (weak default, same pattern as `I2C_ApplicationEventCallBack`)
  is where this app raises CS back up once the frame completes (`SPI_EVENT_TX_CMPLT`) --
  no code in `main()` ever waits on a SPI flag or manages CS timing directly.
- `SPI_CR2_ERRIE` is enabled alongside `TXEIE`/`RXNEIE` so an overrun (`OVR`, a byte
  arriving in the RX shift register while a previous one hadn't been read) gets reported
  and the transfer closed cleanly instead of silently losing data.
