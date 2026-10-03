from threading import Event, Thread

from server.httpserver import HTTP_Server
from server.logging_config import logger
from server.services import MavlinkHandler, StatusCache
from server.status_wsclient import Status_Client
from server.utilities.connect_to_sysid import connect_to_sysid

import os

stop_event = Event()


def run_http_server(http_server, production, host, port):
    http_server.serve_forever(production, host, port)


def run_status_client(ws_client, production, host, port):
    while not stop_event.is_set():
        ws_client.connect_to(production, host, port)

    # for some reason, only after hitting keyboard interrupt ctrl-c THREE times to interupt sio
    # not a clean disconnect, not calling this code for some reason
    ws_client.stop()


# def signal_handler(sig, frame):
#     print("Caught Ctrl+C, shutting down...")
#     stop_event.set()

if __name__ == "__main__":
    # Simplified argument parsing here, replace with your own
    production = False
    # HOST, PORT = "localhost", 9000
    # STATUS_HOST, STATUS_PORT = "localhost", 8000
    HOST, PORT = os.getenv("HOST"), os.getenv("PORT")
    STATUS_HOST, STATUS_PORT = os.getenv("STATUS_HOST"), os.getenv("STATUS_PORT")
    DISABLE_STATUS = False
    # MAVLINK_CONNECTION_STRING = "udpin:localhost:14551"
    MAVLINK_CONNECTION_STRING = os.getenv("MAVLINK_CONNECTION_STRING")

    mav_connection = connect_to_sysid(MAVLINK_CONNECTION_STRING, 1)
    if mav_connection is None:
        # Often this can be fixed by restarting mavproxy
        raise ConnectionError("MAV connection failed. Is mavproxy running?")
    else:
        logger.info("MAV connection successful")

    # set_message_streaming_rates(mav_connection) # optional - set update rate (applies to both MissionPlanner and this server)

    # Create global MAVLink services
    logger.info("Creating MAVLink handler and status cache...")
    status_cache = StatusCache()
    status_cache.set_mode_mapping(mav_connection.mode_mapping())
    handler = MavlinkHandler(mav_connection, status_cache)
    handler.start()
    logger.info("MAVLink handler started")

    # Create server instances
    http_server = HTTP_Server(mav_connection, status_cache, handler)
    ws_client = Status_Client(status_cache) if not DISABLE_STATUS else None

    # Start HTTP server thread
    http_thread = Thread(
        target=run_http_server, args=(http_server, production, HOST, PORT)
    )
    http_thread.start()

    # Start status websocket client thread if enabled
    status_thread = None
    if not DISABLE_STATUS:
        status_thread = Thread(
            target=run_status_client,
            args=(ws_client, production, STATUS_HOST, STATUS_PORT),
        )
        status_thread.start()

    try:
        # Wait for stop event
        while not stop_event.is_set():
            stop_event.wait(1)
    except KeyboardInterrupt:
        stop_event.set()

    logger.info("Shutting down threads...")

    # Stop the MAVLink handler
    logger.info("Stopping MAVLink handler...")
    handler.stop()

    # Flask's server doesn't provide a clean stop API,
    # but exiting main thread will kill the app.
    # You might want to set socketio.stop() inside HTTP_Server to support this.

    http_thread.join(timeout=3)
    if status_thread:
        status_thread.join(timeout=3)

    logger.info("Exited cleanly.")
