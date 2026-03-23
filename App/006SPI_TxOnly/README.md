STM32 SPI Master Driver (Bare-Metal)
This project implements a custom SPI Master driver for the STM32L47xx MCU to transmit data via polling. It bypasses HAL/LL libraries, interacting directly with registers to demonstrate low-level hardware control.

🛠️ Custom Drivers
The application runs on a proprietary driver stack:

stm32l47xx.h: Device-specific header with memory-mapped register addresses and bit definitions.

stm32l475xx_gpio_driver.h/c: APIs for peripheral clock control, Pin Init, and Alternate Function (AF) mapping.

stm32l475xx_spi_driver.h/c: Core SPI logic including SPI_Init, SendData, and StatusFlag management.

🔧 Hardware Setup (SPI1)
Pins: SCK (PA5), MOSI (PA7), NSS (PA4) — Alt Function Mode 5.

Mode: Master, Full Duplex.

Format: 8-bit Data Frame, CPOL=0, CPHA=1.

NSS: Software Slave Management (SSM) enabled.

💻 Logic Flow
Init: Configure GPIO pins for High-Speed AF5 and initialize SPI1 registers via custom APIs.

Transfer: * Enable SPI peripheral.

Push "Hello World" through the TX buffer using polling (TXE flag).

Wait for BSY (Busy) flag to clear before disabling to ensure full data delivery.

Power: Disable SPI1 after transmission to optimize consumption.

🚀 Key Takeaways
This project highlights professional skills in register-level programming, memory mapping, and hardware-software synchronization without the abstraction of third-party libraries.