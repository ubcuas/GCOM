#!/bin/bash

# Configuration from environment variables with defaults
MAVLINK_MASTER=${MAVLINK_MASTER:-"udp:mavproxy:14551"}
MPS_HOST=${MPS_HOST:-"0.0.0.0"}
MPS_PORT=${MPS_PORT:-"9000"}
MPS_DEV=${MPS_DEV:-""}
MPS_STATUS_HOST=${MPS_STATUS_HOST:-"0.0.0.0"}
MPS_STATUS_PORT=${MPS_STATUS_PORT:-"8000"}
DISABLE_STATUS=${DISABLE_STATUS:-"false"}

echo "Starting Mission Planner Server (MPS)..."
echo "  MAVLink Master: $MAVLINK_MASTER"
echo "  Host: $MPS_HOST"
echo "  Port: $MPS_PORT"
echo "  Status Host: $MPS_STATUS_HOST"
echo "  Status Port: $MPS_STATUS_PORT"

# Build MPS command line arguments
MPS_ARGS="--port=$MPS_PORT"
[ -n "$MPS_DEV" ] && MPS_ARGS="$MPS_ARGS --dev"
[ "$DISABLE_STATUS" = "true" ] && MPS_ARGS="$MPS_ARGS --disable-status"
[ -n "$MPS_STATUS_HOST" ] && MPS_ARGS="$MPS_ARGS --status-host=$MPS_STATUS_HOST"
[ -n "$MPS_STATUS_PORT" ] && MPS_ARGS="$MPS_ARGS --status-port=$MPS_STATUS_PORT"

# Start MPS in foreground
python src/main.py $MPS_ARGS
