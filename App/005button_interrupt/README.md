STM32 Interrupt-Driven LED Control (EXTI Driver)
This project demonstrates how to control a User LED using External Interrupts (EXTI) on an STM32 microcontroller (ARM Cortex-M4). It features a custom-built GPIO driver and follows best practices for interrupt handling in embedded systems.

🚀 Features
Custom GPIO Driver: Low-level register manipulation for pin configuration, speed, and pull-up/pull-down resistors.

EXTI Management: Edge-detection configuration (Falling Edge) for the onboard User Button (PC13).

Efficient Interrupt Architecture: Uses a "Top-Half / Bottom-Half" approach where the ISR is kept minimal, offloading the main logic to the application loop via a volatile flag.

Software Debouncing: Implemented a non-blocking delay to filter mechanical switch noise.

🛠️ Hardware Requirements
Microcontroller: STM32L475VG 

Input: User Button (PC13 - connected to EXTI line 13).

Output: User LED (PB14).