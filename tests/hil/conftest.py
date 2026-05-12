import os
import time

import pytest
import serial


@pytest.fixture(scope="session")
def serial_port_name():
    port = os.getenv("STM32_PORT")
    if not port:
        pytest.skip("Set STM32_PORT to the ST-LINK Virtual COM Port, for example STM32_PORT=COM4")
    return port


@pytest.fixture(scope="session")
def baudrate():
    return int(os.getenv("STM32_BAUD", "9600"))


@pytest.fixture(scope="function")
def stm32_serial(serial_port_name, baudrate):
    with serial.Serial(serial_port_name, baudrate, timeout=2) as ser:
        time.sleep(0.2)
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        yield ser


@pytest.fixture(scope="function")
def send_command(stm32_serial):
    def _send(command):
        stm32_serial.write(command.encode("ascii") + b"\r\n")
        return stm32_serial.readline().decode("ascii", errors="replace").strip()

    return _send
