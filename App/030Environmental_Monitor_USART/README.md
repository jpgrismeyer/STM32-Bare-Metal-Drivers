# 030 - Environmental Monitor USART

This example connects the STM32L475 bare-metal driver stack with a planqc-style monitoring use case.

The firmware exposes a small USART command console over the ST-LINK Virtual COM Port. Commands trigger I2C reads from the onboard environmental sensors and return raw monitoring values plus status.

## Hardware

- Board: B-L475E-IOT01A2 Discovery IoT Node
- USART: USART1 through ST-LINK Virtual COM Port
- I2C bus: I2C2 (onboard sensors), I2C1 (external module, exposed on the ARDUINO Uno V3 header)
- Pressure sensor: LPS22HB
- Temperature/humidity sensor: HTS221

## Pin Mapping

| Function | Pin | Mode |
|---|---|---|
| USART1_TX | PB6 | AF7 push-pull |
| USART1_RX | PB7 | AF7 push-pull |
| I2C2_SCL | PB10 | AF4 open-drain |
| I2C2_SDA | PB11 | AF4 open-drain |
| I2C1_SCL | PB8 | AF4 open-drain |
| I2C1_SDA | PB9 | AF4 open-drain |
| User LED | PB14 | Output |

I2C2 (onboard sensors) is wired internally on the PCB only — confirmed against ST's UM2153 (Table 4 "ARDUINO® connector pinout" + the I/O assignment table), where the PB10/PB11 net is literally named `INTERNAL-I2C2_SCL`/`INTERNAL-I2C2_SDA` and doesn't appear on any external connector. I2C1 (PB8/PB9) IS exposed on the ARDUINO Uno V3 header (CN1, pins D15=SCL/D14=SDA), so it's the bus to use for wiring up an external module and probing it with a logic analyzer.

## Sensor Addresses

| Sensor | Address | WHO_AM_I register | Expected value |
|---|---:|---:|---:|
| LPS22HB pressure sensor | `0x5D` | `0x0F` | `0xB1` |
| HTS221 temperature/humidity sensor | `0x5F` | `0x0F` | `0xBC` |

## Commands

Use a serial terminal configured as `9600 8N1`.

| Command | Response |
|---|---|
| `PING` | `PONG` |
| `HELP` | command list |
| `SENSOR IDS` | `LPS22HB=0xB1 HTS221=0xBC STATUS=OK` |
| `INIT SENSORS` | `INIT STATUS=OK` |
| `PRESSURE RAW` | `PRESS_RAW=<value> STATUS=OK` |
| `TEMP RAW` | `TEMP_RAW=<value> STATUS=OK` |
| `HUM RAW` | `HUM_RAW=<value> STATUS=OK` |
| `MONITOR` | `PRESS_RAW=<value> TEMP_RAW=<value> HUM_RAW=<value> STATUS=OK` |
| `PRESSURE HPA` | `PRESS_HPA_X10=<value> STATUS=OK` (hPa * 10) |
| `TEMP C` | `TEMP_C_X10=<value> STATUS=OK` (degC * 10) |
| `HUM RH` | `HUM_RH_X10=<value> STATUS=OK` (%RH * 10) |
| `MONITOR FULL` | `PRESS_HPA_X10=<value> TEMP_C_X10=<value> HUM_RH_X10=<value> STATUS=OK` |
| `THRESHOLD <x10>` | sets the fan humidity threshold (%RH * 10), e.g. `THRESHOLD 400` for 40.0%RH -> `THRESHOLD_X10=400 STATUS=OK` |
| `THRESHOLD?` | reads back the current threshold -> `THRESHOLD_X10=<value> STATUS=OK` |
| `FAN STATUS` | `FAN=ON`\|`OFF THRESHOLD_X10=<value> STATUS=OK` |
| `SD INIT` | runs the SD-over-SPI init sequence -> `SD_INIT STATUS=<code> TYPE=<SDSC\|SDHC_SDXC\|UNKNOWN>` |
| `SD STATUS` | reports the last `SD INIT` result -> `SD_READY=0`\|`1 TYPE=<...> STATUS=OK` |
| `SD WRITE TEST` | writes a known 256-byte-repeating pattern to a raw test block -> `SD_WRITE_TEST STATUS=<code>` (requires `SD INIT` first) |
| `SD READ TEST` | reads the same test block back and checks it matches -> `SD_READ_TEST STATUS=<code> MATCH=0`\|`1` |
| `USART STATUS` | selected USART diagnostic values |
| `I2C1 SCAN` | scans addresses `0x08`-`0x77` on the external I2C1 bus, reports every address that ACKs -> `I2C1_SCAN_DONE FOUND=0`\|`1` |
| `I2C1 READX <addr> <reg>` | generic single-byte register read (write-reg, repeated START, read-data, STOP) against any 7-bit address on I2C1, hex arguments, e.g. `I2C1 READX 3C 00` -> `I2C1_READX ADDR=0x3C REG=0x00 VAL=<value> STATUS=OK` |

## Current Scope

1. USART command interface
2. I2C sensor identification
3. sensor power/configuration writes
4. raw sensor register reads
5. physical-unit conversion (pressure, temperature, humidity)
6. humidity-threshold fan actuator (GPIO)
7. status reporting

LPS22HB pressure converts directly using the fixed 4096 LSB/hPa sensitivity from the datasheet (no calibration needed). HTS221 temperature and humidity use the sensor's factory two-point calibration, read once in `Sensors_Init()` via `HTS221_ReadCalibration()` and cached for every later conversion. Converted values are reported as integers scaled by 10 (one implied decimal digit) to avoid float formatting in `SendInt32()`.

The original `PRESSURE RAW` / `TEMP RAW` / `HUM RAW` / `MONITOR` commands are kept unchanged so the existing HIL test suite keeps passing as-is; the physical-unit commands were added alongside them, not as a replacement.

## Fan Actuator (Plant Guardian)

`GPIO_FanInit()` configures `PA8` as a push-pull output driving a relay module, which switches a 5V fan. Whenever a fresh humidity reading is taken (`HUM RH` or `MONITOR FULL`), `Fan_UpdateState()` compares it against a configurable threshold (`humidity_threshold_x10`, default 40.0%RH) and drives the pin: fan ON when humidity is below the threshold, OFF otherwise. The threshold is adjustable at runtime with `THRESHOLD <x10>` and readable with `THRESHOLD?` or `FAN STATUS`.

This app has no periodic timer tick, so the fan only updates when a humidity-reading command actually runs — it reacts to host polling, it doesn't sample on its own yet. A SysTick-driven periodic loop (read sensors + update fan without needing a host command) is the natural next step for fully unattended operation, and is not implemented here.

## SD Card Logger (Plant Guardian)

`SD_SPI1_GPIOInits()`/`SD_SPI1_Inits()` bring up SPI1 on `PA5`/`PA6`/`PA7` (SCK/MISO/MOSI, AF5) plus `PA4` as a manually-driven chip-select, at a slow prescaler suitable for the SD-over-SPI init sequence. The `SD INIT` command runs the full protocol init (`sd_spi_driver.c`: CMD0, CMD8, CMD55+ACMD41, CMD58, optional CMD16) and reports the result plus detected card type (SDSC vs SDHC/SDXC); on success the driver also switches SPI1 to a faster prescaler for the block I/O that comes next. `SD STATUS` reports the last `SD INIT` result without re-running it.

`SD_ReadBlock`/`SD_WriteBlock` (`Src/sd_spi_driver.c`) implement single 512-byte block I/O over CMD17/CMD24 with the standard SPI-mode data tokens. `SD WRITE TEST` / `SD READ TEST` exercise them end-to-end against a fixed raw block (`SD_TEST_BLOCK_ADDR`, far from block 0 so it doesn't touch the MBR/partition table) — **only run these on a card you don't mind overwriting raw data on**, there's no filesystem layer yet, so this writes directly to the block. Only SD ver2.0+ cards are supported (`SD_ERROR_UNSUPPORTED` if CMD8 is rejected); this covers essentially every microSD card sold today.

Not implemented yet: FatFs integration (`diskio.c` glue) and the periodic logging loop — next milestones.

## External I2C Module (I2C1, logic-analyzer captures)

`I2C1_GPIOInits()`/`I2C1_Inits()` bring up a second, independent I2C peripheral on PB8/PB9 (I2C1), separate from the onboard-sensor I2C2 bus. This exists purely so an external module (any breakout with `VCC`/`GND`/`SCL`/`SDA`, wired with jumpers to the ARDUINO header's `3.3V`/`GND`/`D15`/`D14` pins) can be probed with a logic analyzer — I2C2 can't be, since it never leaves the board.

`I2C1 SCAN` performs a standard bus scan (1-byte write to every 7-bit address 0x08-0x77, reports which ACK) to find an unknown module's address without needing its datasheet. `I2C1 READX <addr> <reg>` then runs the same write-register/repeated-START/read-data/STOP pattern used everywhere else in this project (`I2C_ReadReg8` in App 030, `I2C_MasterSendDataIT`+`I2C_MasterReceiveDataIT` in App 031), against any address/register pair — the returned value doesn't need to be meaningful; the point is to demonstrate the full protocol sequence on the wire.

`I2C1_ReadReg8()` reuses the blocking `I2C_MasterSendData`/`I2C_MasterReceiveData` functions already validated by every other I2C app in this repo, just against a different `I2C_Handle_t` instance — no new driver code, only new GPIO/peripheral init and console glue.

## Interview Relevance

This example is useful for explaining firmware work at the hardware-firmware boundary:

- initialize MCU peripherals
- communicate with environmental sensors
- validate sensor identity
- read raw monitoring data
- report values and status through a command interface
- prepare the firmware for Python HIL tests

The structure maps directly to monitoring and verification tasks in laboratory electronics.

