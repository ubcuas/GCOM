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

The server will start on port 8080 and accept WebSocket connections.

## Message Format

Clients should send signaling messages in the following format:

```json
{
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

- **Expected Clients**: 2 (Streaming Node and GCOM)
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
