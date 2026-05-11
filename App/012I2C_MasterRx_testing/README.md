# App: 012I2C_MasterRx_testing.c

## 📝 Purpose
This application demonstrates a simple 10-byte reception from an ESP32 Slave (Address 0x68). It is the basic "Hello World" of our I2C implementation.

## 🚀 How to Run
1. Connect PB8 (SCL) and PB9 (SDA) to the ESP32.
2. Ensure common Ground and 4.7k pull-up resistors are present.
3. Flash the STM32. 
4. The data will be stored in `rxBuffer`. Put a breakpoint in the `if (status == I2C_ERROR_NONE)` line to inspect the message.

## 🛠 App-Specific Logic
* **Data Length:** 10 bytes (defined by `RX_BUFFER_LEN`).
* **Slave Address:** `0x68`.
* **Flow:** Initialization -> 1-second delay loop -> Master Receive.

---
*For deep technical details on the I2C driver and register mapping, refer to the specific I2C_Driver README.