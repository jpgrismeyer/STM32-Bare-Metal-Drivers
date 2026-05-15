import pytest


pytestmark = pytest.mark.hil


def test_interrupt_console_ping(send_command):
    assert send_command("PING") == "PONG"


def test_interrupt_console_echo(send_command):
    assert send_command("ECHO interrupt console") == "interrupt console"


def test_interrupt_console_led_on(send_command):
    assert send_command("LED ON") == "OK"


def test_interrupt_console_led_off(send_command):
    assert send_command("LED OFF") == "OK"


def test_interrupt_console_button_query(send_command):
    assert send_command("BUTTON?") in ("PRESSED", "RELEASED")


def test_interrupt_console_i2c_whoami(send_command):
    assert send_command("I2C WHOAMI") == "0xB1"
