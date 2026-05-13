import pytest


pytestmark = pytest.mark.hil


def test_interrupt_ping(send_command):
    assert send_command("PING") == "PONG"


def test_interrupt_echo(send_command):
    assert send_command("ECHO interrupt") == "interrupt"
