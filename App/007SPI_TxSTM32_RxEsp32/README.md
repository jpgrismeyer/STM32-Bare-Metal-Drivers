STM32-to-ESP32 Low-Level SPI Driver
This repository contains a bare-metal SPI master driver written in C for STM32 microcontrollers. The project demonstrates high-speed data synchronization with an ESP32 slave by bypassing standard HAL libraries and interacting directly with hardware registers.



🚀 Technical Highlights
Zero-HAL Approach: Complete peripheral control via RCC, GPIO, and SPI register manipulation.

True 8-bit Data Steering: Solved the STM32 16-bit "ghost byte" issue by implementing volatile 8-bit pointer casting for DR register access.

Custom Frame Protocol: Implemented a robust \[Length] + \[Payload] + \[Zero-Padding] scheme to ensure slave-side synchronization.

Manual CS Management: Software-controlled Chip Select (NSS) for precise transaction framing.



🛠️Tech Stack
Master: STM32 (L4 Series) – Core C.

Slave: ESP32 (WROOM) – Arduino/C++ Framework.

Communication: SPI Mode 0, MSB First.

Toolchain: STM32CubeIDE (Bare-metal project setup).



🧠 Engineering Challenges \& Solutions

1. The 16-bit Store Bug
Problem: Writing SPI1->DR = data caused the CPU to perform a 16/32-bit store, leading the SPI hardware to generate 16 clock pulses and inserting a 0x00 byte after every character.
Solution: Forced an 8-bit bus transaction using a volatile byte-pointer cast:



&#x09;\*((\_\_vo uint8\_t \*)\&pSPIx->DR) = \*pTxBuffer;



2. Slave Buffer Alignment
Problem: The ESP32 SPI slave hardware requires a fixed number of clock pulses (32 bytes / 256 bits) to release the transaction buffer.
Solution: Developed a padding algorithm that fills the remaining SPI frame with null bytes, allowing the ESP32 to process strings of variable lengths up to 32 bytes.



🔌 Hardware Configuration

Signal		STM32 (Master)		ESP32 (Slave)

MOSI		PA7			GPIO 23

SCK		PA5			GPIO 18

CS / NSS	PA4			GPIO 5

GNDCommon 	GNDCommon 		GND

VCC		5V (USB VBUS)		Vin (5V)

