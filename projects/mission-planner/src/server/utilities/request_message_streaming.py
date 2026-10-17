import math

from pymavlink import mavutil

from server.logging_config import logger
from server.services.mavlink_handler import MavlinkHandler


def set_message_streaming_rates(handler: MavlinkHandler):
    GPS_RATE_HZ = 0.25
    ATTITUDE_RATE_HZ = 1
    VFR_HUD_RATE_HZ = 1
    SYS_STATUS_RATE_HZ = 5
    MISSION_ITEM_REACHED_RATE_HZ = 1
    WIND_COV_RATE_HZ = 10

    message = handler.mav.command_long_encode(
        handler.target_system,  # Target system ID
        handler.target_component,  # Target component ID
        mavutil.mavlink.MAV_CMD_SET_MESSAGE_INTERVAL,  # ID of command to send
        0,  # Confirmation
        mavutil.mavlink.MAVLINK_MSG_ID_GLOBAL_POSITION_INT,  # param1: Message ID to be streamed
        int(GPS_RATE_HZ * 1000000),  # param2: Interval in microseconds
        0,  # param3 (unused)
        0,  # param4 (unused)
        0,  # param5 (unused)
        0,  # param6 (unused)
        2,  # param7: Response Target: (2 - broadcast)
    )

    handler.mav.send(message)

    response = handler.wait_for_message(
        "COMMAND_ACK",
        timeout=3.0,
        filter_func=lambda msg: msg.command
        == mavutil.mavlink.MAV_CMD_SET_MESSAGE_INTERVAL,
    )
    if response and response.result == mavutil.mavlink.MAV_RESULT_ACCEPTED:
        logger.info(f"Setting GPS_RATE_HZ = {GPS_RATE_HZ} Hz")
    else:
        logger.warning(f"Setting GPS_RATE_HZ = {GPS_RATE_HZ} failed")

    message = handler.mav.command_long_encode(
        handler.target_system,  # Target system ID
        handler.target_component,  # Target component ID
        mavutil.mavlink.MAV_CMD_SET_MESSAGE_INTERVAL,  # ID of command to send
        0,  # Confirmation
        mavutil.mavlink.MAVLINK_MSG_ID_ATTITUDE,  # param1: Message ID to be streamed
        int(ATTITUDE_RATE_HZ * 1000000),  # param2: Interval in microseconds
        0,  # param3 (unused)
        0,  # param4 (unused)
        0,  # param5 (unused)
        0,  # param6 (unused)
        2,  # param7: Response Target: (2 - broadcast)
    )

    handler.mav.send(message)

    response = handler.wait_for_message(
        "COMMAND_ACK",
        timeout=3.0,
        filter_func=lambda msg: msg.command
        == mavutil.mavlink.MAV_CMD_SET_MESSAGE_INTERVAL,
    )
    if response and response.result == mavutil.mavlink.MAV_RESULT_ACCEPTED:
        logger.info(f"Setting ATTITUDE_RATE_HZ = {ATTITUDE_RATE_HZ} Hz")
    else:
        logger.warning(f"Setting ATTITUDE_RATE_HZ = {ATTITUDE_RATE_HZ} failed")

    message = handler.mav.command_long_encode(
        handler.target_system,  # Target system ID
        handler.target_component,  # Target component ID
        mavutil.mavlink.MAV_CMD_SET_MESSAGE_INTERVAL,  # ID of command to send
        0,  # Confirmation
        mavutil.mavlink.MAVLINK_MSG_ID_VFR_HUD,  # param1: Message ID to be streamed
        int(VFR_HUD_RATE_HZ * 1000000),  # param2: Interval in microseconds
        0,  # param3 (unused)
        0,  # param4 (unused)
        0,  # param5 (unused)
        0,  # param6 (unused)
        2,  # param7: Response Target: (2 - broadcast)
    )

    handler.mav.send(message)

    response = handler.wait_for_message(
        "COMMAND_ACK",
        timeout=3.0,
        filter_func=lambda msg: msg.command
        == mavutil.mavlink.MAV_CMD_SET_MESSAGE_INTERVAL,
    )
    if response and response.result == mavutil.mavlink.MAV_RESULT_ACCEPTED:
        logger.info(f"Setting VFR_HUD_RATE_HZ = {VFR_HUD_RATE_HZ} Hz")
    else:
        logger.warning(f"Setting VFR_HUD_RATE_HZ = {VFR_HUD_RATE_HZ} failed")

    message = handler.mav.command_long_encode(
        handler.target_system,  # Target system ID
        handler.target_component,  # Target component ID
        mavutil.mavlink.MAV_CMD_SET_MESSAGE_INTERVAL,  # ID of command to send
        0,  # Confirmation
        mavutil.mavlink.MAVLINK_MSG_ID_SYS_STATUS,  # param1: Message ID to be streamed
        int(SYS_STATUS_RATE_HZ * 1000000),  # param2: Interval in microseconds
        0,  # param3 (unused)
        0,  # param4 (unused)
        0,  # param5 (unused)
        0,  # param6 (unused)
        2,  # param7: Response Target: (2 - broadcast)
    )

    handler.mav.send(message)

    response = handler.wait_for_message(
        "COMMAND_ACK",
        timeout=3.0,
        filter_func=lambda msg: msg.command
        == mavutil.mavlink.MAV_CMD_SET_MESSAGE_INTERVAL,
    )
    if response and response.result == mavutil.mavlink.MAV_RESULT_ACCEPTED:
        logger.info(f"Setting SYS_STATUS_RATE_HZ = {SYS_STATUS_RATE_HZ} Hz")
    else:
        logger.warning(f"Setting SYS_STATUS_RATE_HZ = {SYS_STATUS_RATE_HZ} failed")

    message = handler.mav.command_long_encode(
        handler.target_system,  # Target system ID
        handler.target_component,  # Target component ID
        mavutil.mavlink.MAV_CMD_SET_MESSAGE_INTERVAL,  # ID of command to send
        0,  # Confirmation
        mavutil.mavlink.MAVLINK_MSG_ID_MISSION_ITEM_REACHED,  # param1: Message ID to be streamed
        int(MISSION_ITEM_REACHED_RATE_HZ * 1000000),  # param2: Interval in microseconds
        0,  # param3 (unused)
        0,  # param4 (unused)
        0,  # param5 (unused)
        0,  # param6 (unused)
        2,  # param7: Response Target: (2 - broadcast)
    )

    handler.mav.send(message)

    response = handler.wait_for_message(
        "COMMAND_ACK",
        timeout=3.0,
        filter_func=lambda msg: msg.command
        == mavutil.mavlink.MAV_CMD_SET_MESSAGE_INTERVAL,
    )
    if response and response.result == mavutil.mavlink.MAV_RESULT_ACCEPTED:
        logger.info(
            f"Setting MISSION_ITEM_REACHED_RATE_HZ = {MISSION_ITEM_REACHED_RATE_HZ} Hz"
        )
    else:
        logger.warning(
            f"Setting MISSION_ITEM_REACHED_RATE_HZ = {MISSION_ITEM_REACHED_RATE_HZ} failed"
        )

    message = handler.mav.command_long_encode(
        handler.target_system,  # Target system ID
        handler.target_component,  # Target component ID
        mavutil.mavlink.MAV_CMD_SET_MESSAGE_INTERVAL,  # ID of command to send
        0,  # Confirmation
        mavutil.mavlink.MAVLINK_MSG_ID_WIND_COV,  # param1: Message ID to be streamed
        int(WIND_COV_RATE_HZ * 1000000),  # param2: Interval in microseconds
        0,  # param3 (unused)
        0,  # param4 (unused)
        0,  # param5 (unused)
        0,  # param6 (unused)
        2,  # param7: Response Target: (2 - broadcast)
    )

    handler.mav.send(message)

    response = handler.wait_for_message(
        "COMMAND_ACK",
        timeout=3.0,
        filter_func=lambda msg: msg.command
        == mavutil.mavlink.MAV_CMD_SET_MESSAGE_INTERVAL,
    )
    if response and response.result == mavutil.mavlink.MAV_RESULT_ACCEPTED:
        logger.info(f"Setting WIND_COV_RATE_HZ = {WIND_COV_RATE_HZ} Hz")
    else:
        logger.warning(f"Setting WIND_COV_RATE_HZ = {WIND_COV_RATE_HZ} failed")


def request_messages(handler: MavlinkHandler, message_types: list) -> bool:
    for message_type in message_types:
        message = handler.mav.command_long_encode(
            handler.target_system,  # Target system ID
            handler.target_component,  # Target component ID
            mavutil.mavlink.MAV_CMD_REQUEST_MESSAGE,  # ID of command to send
            0,  # Confirmation
            message_type,  # param1: Message ID to be streamed
            0,  # param2 (unused)
            0,  # param3 (unused)
            0,  # param4 (unused)
            0,  # param5 (unused)
            0,  # param6 (unused)
            0,  # param7: Response Target (0 - default, 1 - requestor, 2 - broadcast)
        )

        handler.mav.send(message)

        response = handler.wait_for_message(
            "COMMAND_ACK",
            timeout=3.0,
            filter_func=lambda msg: msg.command
            == mavutil.mavlink.MAV_CMD_REQUEST_MESSAGE,
        )
        if response and response.result == mavutil.mavlink.MAV_RESULT_ACCEPTED:
            # logger.debug(f"Request for Message of type {message_type} ACCEPTED")
            pass
        else:
            logger.warning(f"Request for Message of type {message_type} DENIED")


def get_parameter(handler: MavlinkHandler, param_id: str):
    param_id_bytes = param_id.encode("ascii").ljust(16, b"\x00")

    # Sometimes if the SITL is failing to respond with the param values, you may be accidentally
    # using the plane SITL instead of the copter
    for attempt in range(2):
        handler.mav.param_request_read_send(
            handler.target_system, handler.target_component, param_id_bytes, -1
        )

        def filter_param_id(msg):
            # Pymavlink already decodes param_id to string and strips null padding
            logger.debug(f"Comparing param_id: '{msg.param_id}' with '{param_id}'")
            return msg.param_id == param_id

        param_value_msg = handler.wait_for_message(
            "PARAM_VALUE", timeout=3.0, filter_func=filter_param_id
        )

        if param_value_msg is not None:
            received_param_id = param_value_msg.param_id
            logger.debug(
                f"Param value message for {received_param_id}: {param_value_msg}"
            )

            return {
                "param_id": received_param_id,
                "param_value": param_value_msg.param_value,
                "param_type": param_value_msg.param_type,
            }

        if attempt == 0:
            logger.warning(
                f"Timeout getting parameter {param_id}, retrying (attempt {attempt + 1}/2)"
            )

    logger.error(f"Failed to get parameter {param_id} after 2 attempts")
    return None


def set_parameter(handler: MavlinkHandler, param_id, param_value) -> bool:
    param_id = bytes(param_id, "ascii")
    # obtain parameter type
    handler.mav.param_request_read_send(
        handler.target_system, handler.target_component, param_id, -1
    )

    param_value_msg = handler.wait_for_message("PARAM_VALUE", timeout=3.0)
    logger.debug(f"Param value message: {param_value_msg}")

    if param_value_msg is None:
        return False
    before = param_value_msg.param_value
    param_type = param_value_msg.param_type

    handler.mav.param_set_send(
        handler.target_system,
        handler.target_component,
        param_id,
        param_value,
        param_type,
    )

    param_value_msg = handler.wait_for_message("PARAM_VALUE", timeout=3.0)
    logger.debug(f"Param value message: {param_value_msg}")

    if param_value_msg is None:
        return False
    after = param_value_msg.param_value

    logger.info(f"Value of parameter {param_id} changed from {before} to {after}")

    # A parameter that already has the requested value is a successful
    # idempotent update. MAVLink reports parameter values as floats, so use a
    # tolerance instead of requiring exact equality.
    try:
        return math.isclose(float(after), float(param_value), rel_tol=1e-6, abs_tol=1e-3)
    except (TypeError, ValueError):
        return after == param_value
