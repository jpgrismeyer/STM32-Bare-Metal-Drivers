import pytest


pytestmark = pytest.mark.hil

EXPECTED_APP_ID = "USART_CMD_CONSOLE_POLLING"


@pytest.fixture(autouse=True)
def _require_matching_firmware(flashed_app_id):
    if flashed_app_id != EXPECTED_APP_ID:
        pytest.skip(
            f"Board is not running '{EXPECTED_APP_ID}' "
            f"(detected: {flashed_app_id or 'no response / unrecognized firmware'}). "
            f"Flash the matching App/ demo and retry."
        )


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
