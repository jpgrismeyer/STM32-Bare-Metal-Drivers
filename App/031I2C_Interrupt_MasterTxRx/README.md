# 031 - I2C Interrupt-Driven Master Read

Same register-read pattern used everywhere else in this repo (write register address, repeated START, read data) — except every wait is driven by `I2C_EV_IRQHandling()`/`I2C_ER_IRQHandling()` instead of a blocking `while` loop on ISR flags. Compare against `013I2C_MasterRx_Sensor` (polling) or `I2C_ReadReg8()` in `030Environmental_Monitor_USART` (also polling): same protocol, different execution model.

## Hardware

- Board: B-L475E-IOT01A2 Discovery IoT Node
- USART: USART1 through ST-LINK Virtual COM Port
- I2C bus: I2C2
- Target: onboard LPS22HB pressure sensor (address `0x5D`, `WHO_AM_I` register `0x0F`, expected `0xB1`)

## What's new here (vs. the rest of the I2C driver)

`Inc/Src/stm32l47xx_i2c_driver.{h,c}` gained interrupt-driven master transfers this session:

- `I2C_MasterSendDataIT()` / `I2C_MasterReceiveDataIT()` — kick off a transfer and return immediately.
- `I2C_EV_IRQHandling()` / `I2C_ER_IRQHandling()` — call these from `I2Cx_EV_IRQHandler()`/`I2Cx_ER_IRQHandler()`. They drive TXIS/RXNE/NACKF/TC/STOPF (event line) and BERR/ARLO/OVR (error line).
- `I2C_ApplicationEventCallBack()` — fires with `I2C_EVENT_TX_CMPLT`, `I2C_EVENT_RX_CMPLT`, or one of the `I2C_ERROR_EVENT_*` codes.
- `I2C_CloseSendData()` / `I2C_CloseReceiveData()` — disable the interrupt-enable bits and reset transfer state; called internally on completion, exposed for an app-level abort/watchdog too.

Before this, `I2C_IRQInterruptConfig`/`I2C_IRQPriorityConfig`/`I2C_ApplicationEventCallBack` were declared in the header but never implemented in the `.c` file — confirmed by grep, not just by memory, before writing any of this.

This app chains a full register read across two IT calls: `I2C WHOAMI IT` starts a 1-byte write of the register address with `Sr=I2C_SR_ENABLE` (repeated START, bus stays held). When `I2C_EVENT_TX_CMPLT` fires, the callback in this file automatically starts the 1-byte read (`Sr=I2C_SR_DISABLE`, real STOP this time). No code in `main()` or `ProcessCommand()` ever waits on an I2C flag.

## Commands

Use a serial terminal configured as `9600 8N1`.

| Command | Response |
|---|---|
| `PING` | `PONG` |
| `HELP` | command list |
| `APP_ID?` | `I2C_INTERRUPT_MASTERTXRX` (used by the HIL suite to auto-detect flashed firmware) |
| `I2C WHOAMI IT` | `I2C_WHOAMI_IT STARTED` (starts the async transfer) |
| `I2C STATUS` | `STATE=<READY\|BUSY_TX\|BUSY_RX> LAST_EVENT=<hex> WHO_AM_I=<hex> MATCH=0\|1` |

Typical sequence: send `I2C WHOAMI IT`, then poll `I2C STATUS` a couple of times until `STATE=READY` and `WHO_AM_I=0xB1 MATCH=1`.

## Design notes

- `AUTOEND` is always left disabled for IT transfers — completion is driven entirely off `TC`/`STOPF`, the same software-managed-end logic the blocking `I2C_MasterSendData`/`ReceiveData` already use in their non-autoend branch. One completion path instead of two.
- `NACKF` is handled in `I2C_EV_IRQHandling` (matches how ST's own HAL treats it — part of the normal transfer sequence, not a bus fault) but is checked defensively in `I2C_ER_IRQHandling` too, in case it actually lands on the error line on this specific silicon. Idempotent either way.
- This also closes a latent gap the blocking functions had: their `while` loops had no timeout on some paths. The IT versions don't loop at all — a stuck transfer just means `I2C STATUS` keeps reporting `BUSY_TX`/`BUSY_RX`, which is at least observable instead of hanging the whole firmware.
