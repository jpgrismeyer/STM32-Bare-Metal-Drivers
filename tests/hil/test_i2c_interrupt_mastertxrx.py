import re
import time

import pytest


pytestmark = pytest.mark.hil

EXPECTED_APP_ID = "I2C_INTERRUPT_MASTERTXRX"


@pytest.fixture(autouse=True)
def _require_matching_firmware(flashed_app_id):
    if flashed_app_id != EXPECTED_APP_ID:
        pytest.skip(
            f"Board is not running '{EXPECTED_APP_ID}' "
            f"(detected: {flashed_app_id or 'no response / unrecognized firmware'}). "
            f"Flash the matching App/ demo and retry."
        )


def test_i2c_it_ping(send_command):
    assert send_command("PING") == "PONG"


def test_i2c_it_app_id(send_command):
    assert send_command("APP_ID?") == EXPECTED_APP_ID


def test_i2c_it_status_before_transfer(send_command):
    response = send_command("I2C STATUS")
    assert re.match(
        r"^STATE=(READY|BUSY_TX|BUSY_RX) LAST_EVENT=0x[0-9A-F]{2} "
        r"WHO_AM_I=0x[0-9A-F]{2} MATCH=[01]$",
        response,
    )


def test_i2c_it_whoami_roundtrip(send_command):
    # The transfer runs entirely in interrupt context (I2C_EV_IRQHandling /
    # I2C_ER_IRQHandling), so the response to the start command is just an
    # ack -- poll I2C STATUS afterwards until it settles back to READY.
    start_response = send_command("I2C WHOAMI IT")
    assert start_response in ("I2C_WHOAMI_IT STARTED", "ERR:I2C_BUSY")

    status = None
    for _ in range(20):
        status = send_command("I2C STATUS")
        if status.startswith("STATE=READY"):
            break
        time.sleep(0.05)

    assert status is not None
    match = re.match(
        r"^STATE=READY LAST_EVENT=0x[0-9A-F]{2} WHO_AM_I=(0x[0-9A-F]{2}) MATCH=([01])$",
        status,
    )
    assert match, f"I2C transfer did not settle back to READY in time: {status}"
    assert match.group(1) == "0xB1"
    assert match.group(2) == "1"
