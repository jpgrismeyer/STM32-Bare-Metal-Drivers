import os
import time

import pytest
import serial


def _query_app_id(port, baud):
    """
    Open a fresh connection, ask the board what firmware it's running,
    and return whatever it answers (or None if it didn't answer with
    something recognizable). Runs once per test session.
    """
    try:
        with serial.Serial(port, baud, timeout=2) as ser:
            time.sleep(0.2)
            ser.reset_input_buffer()
            ser.reset_output_buffer()
            ser.write(b"APP_ID?\r\n")
            response = ser.readline().decode("ascii", errors="replace").strip()
            return response or None
    except serial.SerialException:
        return None


@pytest.fixture(scope="session")
def serial_port_name():
    port = os.getenv("STM32_PORT")
    if not port:
        pytest.skip("Set STM32_PORT to the ST-LINK Virtual COM Port, for example STM32_PORT=COM4")
    return port


@pytest.fixture(scope="session")
def baudrate():
    return int(os.getenv("STM32_BAUD", "9600"))


@pytest.fixture(scope="session")
def flashed_app_id(serial_port_name, baudrate):
    """
    Detect which firmware is actually flashed on the board right now by
    asking it directly (APP_ID?), instead of trusting an env var or a
    hardcoded note that a human has to remember to set/update.

    Returns the APP_ID string reported by the firmware, or None if the
    board didn't answer with a recognized id (older firmware flashed
    before this command existed, a different/unknown app, or no board
    connected at all).
    """
    app_id = _query_app_id(serial_port_name, baudrate)
    if not app_id or app_id in ("ERR", "READY"):
        return None
    return app_id


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
