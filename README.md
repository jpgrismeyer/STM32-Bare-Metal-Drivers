# STM32-Bare-Metal-Drivers
Bare-metal peripheral drivers (GPIO, EXTI, SPI, I2C, UART) for STM32L47xx MCU from scratch. Developed using register-level programming without HAL/LL libraries.

STM32L47xx Custom Drivers Stack
This repository contains a professional-grade peripheral driver library for the STM32L476xx (ARM Cortex-M4) microcontroller, developed from the ground up using bare-metal register programming.

📂 Project Structure
/Inc: Header files for GPIO, SPI, and MCU-specific register definitions (stm32l47xx.h).

/Src: Source code implementation for the driver logic.

/App: Application layer containing practical examples:
GPIO:
001led_toggle.c
002led_button.c
003led_button_ext.c

EXTI:
005button_interrupt.c

SPI:
006SPI_TxOnly.c
007SPI_TxSTM32_RxEsp32.c
008SPIcmd_Handling.c 

## 📫 Contact & Connect
I am an aspiring Embedded Systems Engineer passionate about low-level programming and RTOS. Feel free to reach out if you want to discuss bare-metal development or potential collaborations!

* **Name:** Juan Pablo Grismeyer
* **LinkedIn:** https://www.linkedin.com/in/juan-pablo-grismeyer-a392a0187/
* **Email:** juampagrismeyer@gmail.com


