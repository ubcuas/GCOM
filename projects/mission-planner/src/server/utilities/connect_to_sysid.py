import time

from pymavlink import mavutil
from pymavlink.mavutil import mavfile

from server.logging_config import logger


def connect_to_sysid(connection_str: str, sysid: int, timeout: float = 3) -> mavfile:
    """
    connect_to_sysid connects to a mavlink stream with a specific sysid

    Args:
        connection_str (str): String containing the connection information
        sysid (int): The system id to connect to
        timeout (float, optional): Maximum time to wait for the connection in seconds. Defaults to 3.

    Returns:
        mavutil.mavlink_connection: Returns the connection object if connection is successful,
        else returns None after the timeout
    """
    the_connection = mavutil.mavlink_connection(connection_str)
    time_start = time.time()

    while time.time() - time_start < timeout:
        try:
            print("Sending initial heartbeat to wake up MAVProxy router...")
            the_connection.mav.heartbeat_send(
                mavutil.mavlink.MAV_TYPE_GCS, 
                mavutil.mavlink.MAV_AUTOPILOT_INVALID, 
                0, 0, 0
            )
            msg = the_connection.wait_heartbeat(timeout=1)
            if not msg:
                raise ConnectionError("MAV connection failed. Is mavproxy running?")
            logger.debug(
                f"Heartbeat from system {the_connection.target_system} component {the_connection.target_component}"
            )
            if the_connection.target_system == sysid:
                logger.info(f"Now connected to SYSID {sysid}")
                return the_connection
        except Exception:
            elapsed = time.time() - time_start
            logger.debug(f"Waiting for heartbeat... ({elapsed:.1f}s elapsed)")
            if elapsed >= timeout:
                break

    logger.error(f"Connection timeout after {timeout} seconds")
    return None
