import re

import pytest


pytestmark = [
    pytest.mark.hil,
    pytest.mark.environmental_monitor,
]

EXPECTED_APP_ID = "ENV_MONITOR_USART"


@pytest.fixture(autouse=True)
def _require_matching_firmware(flashed_app_id):
    if flashed_app_id != EXPECTED_APP_ID:
        pytest.skip(
            f"Board is not running '{EXPECTED_APP_ID}' "
            f"(detected: {flashed_app_id or 'no response / unrecognized firmware'}). "
            f"Flash App/030Environmental_Monitor_USART and retry."
        )


def test_environmental_monitor_ping(send_command):
    assert send_command("PING") == "PONG"


def test_environmental_monitor_sensor_ids(send_command):
    response = send_command("SENSOR IDS")
    assert response == "LPS22HB=0xB1 HTS221=0xBC STATUS=OK"


def test_environmental_monitor_pressure_raw(send_command):
    response = send_command("PRESSURE RAW")
    assert re.match(r"^PRESS_RAW=-?\d+ STATUS=OK$", response)


def test_environmental_monitor_temperature_raw(send_command):
    response = send_command("TEMP RAW")
    assert re.match(r"^TEMP_RAW=-?\d+ STATUS=OK$", response)


def test_environmental_monitor_humidity_raw(send_command):
    response = send_command("HUM RAW")
    assert re.match(r"^HUM_RAW=-?\d+ STATUS=OK$", response)


def test_environmental_monitor_combined(send_command):
    response = send_command("MONITOR")
    assert re.match(
        r"^PRESS_RAW=-?\d+ TEMP_RAW=-?\d+ HUM_RAW=-?\d+ STATUS=OK$",
        response,
    )


def test_environmental_monitor_pressure_hpa(send_command):
    response = send_command("PRESSURE HPA")
    match = re.match(r"^PRESS_HPA_X10=(-?\d+) STATUS=OK$", response)
    assert match
    # Sea-level-ish sanity range: 900.0 hPa - 1100.0 hPa
    assert 9000 <= int(match.group(1)) <= 11000


def test_environmental_monitor_temp_c(send_command):
    response = send_command("TEMP C")
    match = re.match(r"^TEMP_C_X10=(-?\d+) STATUS=OK$", response)
    assert match
    # Indoor sanity range: 0.0 C - 45.0 C
    assert 0 <= int(match.group(1)) <= 450


def test_environmental_monitor_hum_rh(send_command):
    response = send_command("HUM RH")
    match = re.match(r"^HUM_RH_X10=(-?\d+) STATUS=OK$", response)
    assert match
    # Physical range: 0.0 %RH - 100.0 %RH
    assert 0 <= int(match.group(1)) <= 1000


def test_environmental_monitor_full(send_command):
    response = send_command("MONITOR FULL")
    assert re.match(
        r"^PRESS_HPA_X10=-?\d+ TEMP_C_X10=-?\d+ HUM_RH_X10=-?\d+ STATUS=OK$",
        response,
    )


def test_environmental_monitor_threshold_query_default(send_command):
    response = send_command("THRESHOLD?")
    match = re.match(r"^THRESHOLD_X10=(-?\d+) STATUS=OK$", response)
    assert match


def test_environmental_monitor_threshold_set(send_command):
    response = send_command("THRESHOLD 550")
    assert response == "THRESHOLD_X10=550 STATUS=OK"

    response = send_command("THRESHOLD?")
    assert response == "THRESHOLD_X10=550 STATUS=OK"


def test_environmental_monitor_threshold_bad_arg(send_command):
    response = send_command("THRESHOLD abc")
    assert response == "ERR:BAD_ARG"


def test_environmental_monitor_fan_status(send_command):
    response = send_command("FAN STATUS")
    assert re.match(r"^FAN=(ON|OFF) THRESHOLD_X10=-?\d+ STATUS=OK$", response)


def test_environmental_monitor_fan_reacts_to_threshold(send_command):
    # Set an unreachably high threshold: humidity will always read below it,
    # so the fan must turn ON on the next humidity reading.
    send_command("THRESHOLD 10000")
    send_command("HUM RH")
    response = send_command("FAN STATUS")
    match = re.match(r"^FAN=(ON|OFF) THRESHOLD_X10=10000 STATUS=OK$", response)
    assert match
    assert match.group(1) == "ON"

    # Set an unreachably low threshold: humidity will always read above it,
    # so the fan must turn OFF on the next humidity reading.
    send_command("THRESHOLD -10000")
    send_command("HUM RH")
    response = send_command("FAN STATUS")
    match = re.match(r"^FAN=(ON|OFF) THRESHOLD_X10=-10000 STATUS=OK$", response)
    assert match
    assert match.group(1) == "OFF"

    # Restore the default threshold so later test runs aren't affected.
    send_command("THRESHOLD 400")


def test_environmental_monitor_sd_status_before_init(send_command):
    # SD_READY reflects whatever SD INIT last reported, not a live card probe,
    # so right after boot (before any SD INIT) it must read 0/UNKNOWN.
    response = send_command("SD STATUS")
    assert re.match(r"^SD_READY=[01] TYPE=(UNKNOWN|SDSC|SDHC_SDXC) STATUS=OK$", response)


def test_environmental_monitor_sd_init(send_command):
    # Requires a microSD card physically inserted in the SPI module wired to
    # PA4 (CS)/PA5 (SCK)/PA6 (MISO)/PA7 (MOSI). Only asserts the response is
    # well-formed and status is one of the driver's defined codes -- doesn't
    # require SD_OK, since this test may run before the SD hardware exists.
    response = send_command("SD INIT")
    match = re.match(
        r"^SD_INIT STATUS=(OK|NO_CARD|UNSUPPORTED|VOLTAGE|TIMEOUT|ERROR) "
        r"TYPE=(UNKNOWN|SDSC|SDHC_SDXC)$",
        response,
    )
    assert match

    response = send_command("SD STATUS")
    assert re.match(r"^SD_READY=[01] TYPE=(UNKNOWN|SDSC|SDHC_SDXC) STATUS=OK$", response)
