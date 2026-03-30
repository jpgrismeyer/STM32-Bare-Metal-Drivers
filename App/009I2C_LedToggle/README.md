STM32L4 I2C Bare-Metal Driver & Sensor Identity Test
This project features a custom-built I2C Bare-Metal Driver for the STM32L475VG microcontroller (Discovery Kit B-L475E-IOT01A2). The application verifies the hardware communication by polling an on-board sensor's unique ID and triggering a visual LED confirmation.
🚀 Overview
1. The system initializes the I2C2 peripheral at 100kHz (Standard Mode) using the HSI16 (16MHz) internal oscillator as the clock source.
2. Upon pressing the User Button (PC13), the Master sends an I2C request to the integrated HTS221 Humidity & Temp sensor (7-bit address: 0x5F).
3. The driver reads the WHO_AM_I register (0x0F).
4. If the received value matches the factory hardcoded ID (0xBC), the LD2 Blue LED (PB14) blinks 5 times.

🛠️ Hardware Configuration (On-board)
Peripheral	Pin				Configuration				Function
I2C2 SCL	PB10		Alt Function 4, Open-Drain, Internal Pull-up		Bus Clock
I2C2 SDA	PB11		Alt Function 4, Open-Drain, Internal Pull-up		Bus Data
User Button	PC13			Input, External Pull-up				Execution Trigger
LD2 (Green)	PB14			Output, Push-Pull				Success Indicator

📂 Driver Architecture
The driver is optimized for the STM32L4 Series hardware-level automation (different from the legacy F4 series):
I2C_Init(): Handles peripheral clock gating and TIMINGR calculation based on a 16MHz kernel clock.
I2C_MasterSendData(): Utilizes the CR2 register to automate the Start condition, Slave Address (SADD), and Data Length (NBYTES).
I2C_MasterReceiveData(): Implements a blocking read mechanism, monitoring the RXNE (Receive Not Empty) flag and confirming the end of the transaction via the TC (Transfer Complete) flag.

📝 Technical NotesClock Gating: Unlike older STM32s, the L4 requires explicit clock source selection in RCC_CCIPR before initializing the I2C timing logic. 
Flag Management: The driver correctly transitions from RXNE polling to TC polling to ensure the hardware-controlled STOP condition is generated safely.
Open-Drain Requirement: GPIOs are strictly configured in Open-Drain mode to prevent bus contention and allow the pull-up resistors to define the logic high level.