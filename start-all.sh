#!/bin/bash

SESSION_NAME="gcom"
REPO_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
PROJECTS_DIR="$REPO_ROOT/projects"

VENV_PATH=".venv"
ARCH=$(uname -m)
OS_NAME=$(uname -s)
NETWORK_NAME="gcom-x_uasnet"
SKIP_MISSION=false
RUN_TARGET=""
SIGNALING_SERVER_URL=""

usage() {
    echo "Usage: $0 [--skip-mission]"
}

parse_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --skip-mission)
                SKIP_MISSION=true
                shift
                ;;
            -h|--help)
                usage
                exit 0
                ;;
            *)
                echo "Unknown option: $1"
                usage
                exit 1
                ;;
        esac
    done
}

select_python() {
    if command -v python3 &>/dev/null; then
        PY_CMD="python3"
    elif command -v python &>/dev/null; then
        PY_CMD="python"
    else
        echo "Error: Python is not installed on this system."
        exit 1
    fi
}

venv_activate_path() {
    case "$OS_NAME" in
        CYGWIN*|MINGW*|MSYS*)
            echo "$VENV_PATH/Scripts/activate"
            ;;
        *)
            echo "$VENV_PATH/bin/activate"
            ;;
    esac
}

prompt_inputs() {
    read -r -p "Signaling server URL (blank to use projects/web-frontend/.env): " SIGNALING_SERVER_URL

    if [ "$SKIP_MISSION" = true ]; then
        return
    fi

    while true; do
        read -r -p "Run via drone or SITL? [drone/sitl]: " RUN_TARGET
        RUN_TARGET=$(echo "$RUN_TARGET" | tr '[:upper:]' '[:lower:]')
        case "$RUN_TARGET" in
            drone|d)
                RUN_TARGET="drone"
                return
                ;;
            sitl|s)
                RUN_TARGET="sitl"
                return
                ;;
            *)
                echo "Please enter 'drone' or 'sitl'."
                ;;
        esac
    done
}

select_sitl_image() {
    if [[ "$ARCH" == "aarch64" || "$ARCH" == "arm64" ]]; then
        UASITL_IMAGE="ubcuas/uasitl:copter-arm-4.5.5"
        echo "ARM architecture detected. Using ARM image."
    else
        UASITL_IMAGE="ubcuas/uasitl:copter-4.5.5"
        echo "non-ARM architecture detected. Using standard image."
    fi
}

setup_network() {
    if ! docker network ls | grep -q "$NETWORK_NAME"; then
        echo "Creating docker network: $NETWORK_NAME"
        docker network create "$NETWORK_NAME"
    else
        echo "Network $NETWORK_NAME already exists, skipping creation."
    fi
}

select_mavproxy_command() {
    if command -v mavproxy.py &>/dev/null; then
        MAV_CMD="mavproxy.py"
    elif command -v mavproxy &>/dev/null; then
        MAV_CMD="mavproxy"
    else
        MAV_CMD=""
    fi

    if [ "$RUN_TARGET" = "sitl" ]; then
        MAV_ARGS="--master=tcp:127.0.0.1:5760 --out=udp:127.0.0.1:14550 --out=udp:127.0.0.1:14551"
    else
        MAV_ARGS="--out=udp:127.0.0.1:14550 --out=udp:127.0.0.1:14551 --baudrate 115200" 
    fi
}

setup_tmux() {
    if tmux has-session -t "$SESSION_NAME" 2>/dev/null; then
        echo "Session '$SESSION_NAME' already exists. Attaching..."
        tmux attach-session -t "$SESSION_NAME"
        exit 0
    fi

    tmux new-session -d -s "$SESSION_NAME"

    if [ "$SKIP_MISSION" = true ]; then
        tmux split-window -t "$SESSION_NAME:0" -h
    else
        tmux split-window -t "$SESSION_NAME:0" -v
        tmux split-window -t "$SESSION_NAME:0" -h
        tmux select-pane -t "$SESSION_NAME:0.2"
        tmux split-window -t "$SESSION_NAME:0" -v
    fi

    tmux select-layout -t "$SESSION_NAME:0" tiled
}

require_venv_command() {
    local service_name=$1
    local venv_dir=$2

    echo "if [ ! -d '$venv_dir' ]; then echo 'ERROR: setup required for $service_name. Missing $venv_dir'; exec bash; fi"
}

setup_sitl() {
    if [ "$RUN_TARGET" != "sitl" ]; then
        echo "Drone mode selected. SITL bypassed."
        return
    fi

    select_sitl_image
    setup_network

    if docker ps --filter "name=^/uasitl$" --filter "status=running" --format "{{.Names}}" | grep -qx "uasitl"; then
        echo "SITL already running in Docker container uasitl."
        return
    fi

    echo "Starting SITL as detached Docker container uasitl..."
    docker rm -f uasitl 2>/dev/null
    if ! docker run -d --rm -p 5760-5780:5760-5780 --network="$NETWORK_NAME" --name=uasitl "$UASITL_IMAGE" >/dev/null; then
        echo "ERROR: failed to start SITL container."
        tmux kill-session -t "$SESSION_NAME" 2>/dev/null
        exit 1
    fi
    echo "Waiting for SITL to initialize..."
    sleep 3
}

setup_mavproxy() {
    local check
    local activate_path
    check=$(require_venv_command "mission-planner" "$PROJECTS_DIR/mission-planner/$VENV_PATH")
    activate_path=$(venv_activate_path)
    if [ -z "$MAV_CMD" ]; then
        tmux send-keys -t "$SESSION_NAME:0.0" "echo 'ERROR: setup required for MAVProxy. Neither mavproxy.py nor mavproxy was found on PATH.'; exec bash" C-m
        return
    fi

    tmux send-keys -t "$SESSION_NAME:0.0" "$check; echo 'Starting MAVProxy'; cd '$PROJECTS_DIR/mission-planner' && source '$activate_path' && $MAV_CMD $MAV_ARGS" C-m
    echo "Giving MAVProxy time to start..."
    sleep 3
}

setup_mission_planner() {
    local check
    local activate_path
    check=$(require_venv_command "mission-planner" "$PROJECTS_DIR/mission-planner/$VENV_PATH")
    activate_path=$(venv_activate_path)
    tmux send-keys -t "$SESSION_NAME:0.1" "$check; echo 'Starting mission planner'; cd '$PROJECTS_DIR/mission-planner' && source '$activate_path' && $PY_CMD src/main.py" C-m
}

setup_web_backend() {
    local pane=$1
    local check
    local activate_path
    check=$(require_venv_command "web-backend" "$PROJECTS_DIR/web-backend/$VENV_PATH")
    activate_path=$(venv_activate_path)
    tmux send-keys -t "$SESSION_NAME:0.$pane" "$check; echo 'Starting web backend'; cd '$PROJECTS_DIR/web-backend' && source '$activate_path' && $PY_CMD src/server.py" C-m
}

setup_web_frontend() {
    local pane=$1
    local env_prefix=""

    if [ -n "$SIGNALING_SERVER_URL" ]; then
        printf -v env_prefix "VITE_SIGNALING_SERVER_URL=%q " "$SIGNALING_SERVER_URL"
    fi

    tmux send-keys -t "$SESSION_NAME:0.$pane" "echo 'Starting web frontend'; cd '$PROJECTS_DIR/web-frontend' && ${env_prefix}npm run dev" C-m
}

start_all() {
    parse_args "$@"
    select_python
    prompt_inputs

    if [ "$SKIP_MISSION" = false ]; then
        select_mavproxy_command
    fi

    setup_tmux

    if [ "$SKIP_MISSION" = true ]; then
        setup_web_backend 0
        setup_web_frontend 1
    else
        setup_sitl
        setup_mavproxy
        setup_mission_planner
        setup_web_backend 2
        setup_web_frontend 3
    fi

    tmux attach-session -t "$SESSION_NAME"
}

start_all "$@"
