#!/usr/bin/env python3
"""
WebRTC Signaling Server

Facilitates WebRTC connection establishment between the Streaming Node
and GCOM system by coordinating the exchange of session descriptions
(SDP) and ICE candidates.
"""

import logging
from typing import Set
from aiohttp import web
import socketio

# Configure logging
logging.basicConfig(
    level=logging.INFO, format="%(asctime)s - %(name)s - %(levelname)s - %(message)s"
)
logger = logging.getLogger(__name__)

# Valid WebRTC message types
VALID_MESSAGE_TYPES = {"offer", "answer", "ice-candidate"}

# Track connected clients
connected_clients: Set[str] = set()

# Create Socket.IO server
sio = socketio.AsyncServer(
    async_mode="aiohttp", cors_allowed_origins="*", logger=True, engineio_logger=True
)

app = web.Application()
sio.attach(app)


@sio.event
async def connect(sid, environ):
    """Handle client connection"""
    # Notify existing clients about the new connection
    for client_id in connected_clients:
        logger.info(f"Notifying {client_id} about new peer {sid}")
        await sio.emit("peer_joined", {"peer_id": sid}, room=client_id)

    # Add the new client to the set
    connected_clients.add(sid)
    logger.info(f"Client connected: {sid}")
    logger.info(f"Total connected clients: {len(connected_clients)}")

    if len(connected_clients) > 2:
        logger.warning(
            f"More than 2 clients connected ({len(connected_clients)}). "
            f"Expected only 2 peers (Streaming Node and GCOM)."
        )

    return True


@sio.event
async def disconnect(sid):
    """Handle client disconnection"""
    connected_clients.discard(sid)
    logger.info(f"Client disconnected: {sid}")
    logger.info(f"Total connected clients: {len(connected_clients)}")


@sio.event
async def signal(sid, data):
    """
    Handle WebRTC signaling messages.

    Expected data format:
    {
        "type": "offer" | "answer" | "ice-candidate",
        "data": <message-specific-data>
    }
    """
    logger.info(f"Received signal from {sid}: {data}")

    # Validate message structure
    if not isinstance(data, dict):
        logger.error(
            f"Invalid message format from {sid}: expected dict, got {type(data)}"
        )
        await sio.emit("error", {"message": "Invalid message format"}, room=sid)
        return

    message_type = data.get("type")

    # Validate message type
    if message_type not in VALID_MESSAGE_TYPES:
        logger.error(
            f"Invalid message type from {sid}: '{message_type}'. "
            f"Valid types: {VALID_MESSAGE_TYPES}"
        )
        await sio.emit(
            "error",
            {
                "message": f"Invalid message type: {message_type}",
                "valid_types": list(VALID_MESSAGE_TYPES),
            },
            room=sid,
        )
        return

    # Get the target peer ID if specified
    target_peer_id = data.get("to")

    if target_peer_id:
        # Send to specific peer
        if target_peer_id in connected_clients:
            logger.info(f"Forwarding {message_type} from {sid} to {target_peer_id}")
            # Add "from" field so recipient knows who sent it
            forwarded_data = {**data, "from": sid}
            await sio.emit("signal", forwarded_data, room=target_peer_id)
        else:
            logger.error(f"Target peer {target_peer_id} not connected")
            await sio.emit("error", {"message": f"Peer {target_peer_id} not found"}, room=sid)
    else:
        # Broadcast to all other connected clients
        for client_id in connected_clients:
            if client_id != sid:
                logger.info(f"Forwarding {message_type} from {sid} to {client_id}")
                # Add "from" field so recipient knows who sent it
                forwarded_data = {**data, "from": sid}
                await sio.emit("signal", forwarded_data, room=client_id)


async def index(request):
    """Simple health check endpoint"""
    return web.Response(text="WebRTC Signaling Server is running")


# Add routes
app.router.add_get("/", index)


def main():
    """Start the signaling server"""
    port = 8081
    logger.info(f"Starting WebRTC Signaling Server on port {port}")
    logger.info(f"Valid message types: {VALID_MESSAGE_TYPES}")
    web.run_app(app, host="0.0.0.0", port=port)


if __name__ == "__main__":
    main()
