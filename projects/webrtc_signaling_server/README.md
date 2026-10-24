# WebRTC Signaling Server

A lightweight Socket.IO-based signaling server that coordinates WebRTC connection establishment between the Streaming Node and GCOM system.

## Overview

This server facilitates the exchange of WebRTC session descriptions (SDP) and ICE candidates between peers. It does not handle media or data transmission itself - only the signaling handshake required to establish a direct peer-to-peer connection.

## Requirements

- Python 3.8+

## Installation

1. Navigate to the signaling server directory:
```bash
cd infrastructure/webrtc_signaling_server
```

2. Install dependencies:
```bash
pip install -r requirements.txt
```

Or using a virtual environment (recommended):
```bash
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
pip install -r requirements.txt
```

## Usage

Start the server:
```bash
python server.py
```

The server will start on port 8081 and accept WebSocket connections.

## Events

> NOTE:
> This is the planned protocol for supporting multiple frontends connected to one SFU. `server.py` does not implement all of it yet.

There are two client roles:
- **`sfu`**: the video source (the SFU, or a mock/streaming node). Only one at a time.
- **`frontend`**: a GCOM web frontend instance. Any number can connect.

| Direction | Event | Payload | Meaning |
|---|---|---|---|
| client → server | `register` | `{"role": "frontend" \| "sfu"}` | Sent immediately after connecting. Clients that have not registered cannot send `signal`. |
| server → sfu | `peer_joined` | `{"peer_id": "<sid>"}` | A frontend has registered. The SFU should create a peer connection and send it an offer. When the SFU registers, it receives one `peer_joined` for every frontend already connected. |
| server → sfu | `peer_left` | `{"peer_id": "<sid>"}` | A frontend disconnected. The SFU should close that peer connection. |
| server → frontend | `sfu_status` | `{"available": true \| false}` | Sent when a frontend registers, and to all frontends when the SFU connects or disconnects. |
| client → server → client | `signal` | `{"to": "<sid>", "type": ..., "data": {...}}` | An offer, answer or ICE candidate (see [Message Format](#message-format)). `to` is required. The server adds `"from": "<sender sid>"` and forwards it to that one peer only. |
| server → client | `error` | `{"message": "..."}` | The client's last message was rejected. |

Routing rules:
- `signal` is only forwarded between a frontend and the SFU. Frontends never receive anything about each other.
- `signal` with a missing/unknown `to`, or `to` from an unregistered client is rejected with `error` by the server.
- The SFU always sends the offer; frontends only answer.

## Message Format

Clients should send signaling messages in the following format:

```json
{
  "to": { ... },
  "type": "offer" | "answer" | "ice-candidate",
  "data": { ... }
}
```

### Valid Message Types

- `offer`: WebRTC SDP offer
- `answer`: WebRTC SDP answer
- `ice-candidate`: ICE candidate for network path negotiation

### Example Messages

**Offer:**
```json
{
  "type": "offer",
  "data": {
    "sdp": "...",
    "type": "offer"
  }
}
```

**Answer:**
```json
{
  "type": "answer",
  "data": {
    "sdp": "...",
    "type": "answer"
  }
}
```

**ICE Candidate:**
```json
{
  "type": "ice-candidate",
  "data": {
    "candidate": "...",
    "sdpMid": "...",
    "sdpMLineIndex": 0
  }
}
```

## Client Connection

Clients connect using Socket.IO protocol. Example using JavaScript:

```javascript
const socket = io('http://localhost:8080');

socket.on('connect', () => {
  console.log('Connected to signaling server');
});

socket.on('signal', (data) => {
  console.log('Received signal:', data);
  // Handle incoming WebRTC signaling message
});

// Send signaling message
socket.emit('signal', {
  type: 'offer',
  data: { /* offer data */ }
});
```

## Behavior
> NOTE:
> undergoing changes

- **Expected Clients**: 2 (Streaming Node and GCOM), should change to be more 
- **Connection Limit**: No hard limit, but warns when more than 2 clients connect
- **Message Routing**: Forwards all messages from a sender to all other connected clients
- **Validation**: Validates message types and rejects invalid messages
- **Logging**: Logs all connections, disconnections, and messages

## Health Check

A simple HTTP endpoint is available at `http://localhost:8080/` for health checks.

## Architecture

This signaling server is a critical component in the Hawkeye-OS streaming architecture:

```
Streaming Node <---> Signaling Server <---> GCOM System
                           |
                    (Coordinates WebRTC
                     connection setup)
```

After the WebRTC connection is established, the signaling server is no longer needed for data transmission.
