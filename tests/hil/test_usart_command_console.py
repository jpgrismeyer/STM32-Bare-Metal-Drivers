import pytest


pytestmark = pytest.mark.hil


def test_ping(send_command):
    assert send_command("PING") == "PONG"


def test_echo(send_command):
    assert send_command("ECHO hello") == "hello"


def test_led_on(send_command):
    assert send_command("LED ON") == "OK"


def test_led_off(send_command):
    assert send_command("LED OFF") == "OK"


def test_button_query(send_command):
    assert send_command("BUTTON?") in ("PRESSED", "RELEASED")


def test_i2c_whoami(send_command):
    assert send_command("I2C WHOAMI") == "0xB1"
