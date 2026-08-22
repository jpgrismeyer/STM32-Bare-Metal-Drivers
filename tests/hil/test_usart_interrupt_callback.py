import pytest


pytestmark = pytest.mark.hil

EXPECTED_APP_ID = "USART_INTERRUPT_CALLBACK"


@pytest.fixture(autouse=True)
def _require_matching_firmware(flashed_app_id):
    if flashed_app_id != EXPECTED_APP_ID:
        pytest.skip(
            f"Board is not running '{EXPECTED_APP_ID}' "
            f"(detected: {flashed_app_id or 'no response / unrecognized firmware'}). "
            f"Flash the matching App/ demo and retry."
        )


def test_interrupt_ping(send_command):
    assert send_command("PING") == "PONG"


def test_interrupt_echo(send_command):
    assert send_command("ECHO interrupt") == "interrupt"
