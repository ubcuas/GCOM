#!/bin/bash

# Mission Planner Docker Helper Script
# Simplifies common Docker operations for the mission-planner project

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Functions
print_header() {
    echo -e "${BLUE}=== $1 ===${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

show_help() {
    cat << EOF
Mission Planner Docker Helper Script

Usage: ./docker-helper.sh [COMMAND] [OPTIONS]

Commands:
    build           Build the Docker image
    run             Run the container (standalone)
    compose-up      Start services with docker-compose
    compose-down    Stop services with docker-compose
    compose-logs    View logs from docker-compose
    logs            View container logs (standalone)
    stop            Stop the container (standalone)
    rm              Remove container and image
    shell           Open a shell in the running container
    test            Test connectivity to MPS
    status          Check container status
    clean           Clean up all Docker resources
    help            Show this help message

Examples:
    # Using docker-compose (recommended)
    ./docker-helper.sh compose-up
    ./docker-helper.sh compose-logs
    ./docker-helper.sh compose-down

    # Standalone mode
    ./docker-helper.sh build
    ./docker-helper.sh run
    ./docker-helper.sh logs
    ./docker-helper.sh test

Options:
    BUILD_ARGS      Additional arguments for docker build
    CONTAINER_NAME  Custom container name (default: mission-planner-mps)
    IMAGE_NAME      Custom image name (default: mission-planner:latest)

EOF
}

# Check prerequisites
check_docker() {
    if ! command -v docker &> /dev/null; then
        print_error "Docker is not installed"
        exit 1
    fi
    print_success "Docker is installed"
}

# Build image
build_image() {
    print_header "Building Docker image"

    check_docker

    IMAGE_NAME="${IMAGE_NAME:-mission-planner:latest}"
    BUILD_ARGS="${BUILD_ARGS:-}"

    echo "Building image: $IMAGE_NAME"
    docker build $BUILD_ARGS -t "$IMAGE_NAME" "$PROJECT_DIR"

    print_success "Image built: $IMAGE_NAME"
}

# Run container (standalone)
run_container() {
    print_header "Running Docker container"

    check_docker

    CONTAINER_NAME="${CONTAINER_NAME:-mission-planner-mps}"
    IMAGE_NAME="${IMAGE_NAME:-mission-planner:latest}"

    if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
        print_warning "Container $CONTAINER_NAME already exists"
        echo "Removing existing container..."
        docker rm -f "$CONTAINER_NAME" > /dev/null
    fi

    echo "Starting container: $CONTAINER_NAME"
    docker run -d \
        --name "$CONTAINER_NAME" \
        --restart unless-stopped \
        -p 14550:14550/udp \
        -p 14551:14551/udp \
        -p 14552:14552/udp \
        -p 9000:9000 \
        -p 8000:8000 \
        -e MAVLINK_MASTER="${MAVLINK_MASTER:-udpin:0.0.0.0:14550}" \
        -e MAVLINK_OUT_1="${MAVLINK_OUT_1:-udp:127.0.0.1:14551}" \
        -e MAVLINK_OUT_2="${MAVLINK_OUT_2:-udp:127.0.0.1:14552}" \
        -e MPS_HOST="${MPS_HOST:-0.0.0.0}" \
        -e MPS_PORT="${MPS_PORT:-9000}" \
        "$IMAGE_NAME"

    print_success "Container started: $CONTAINER_NAME"
    echo "Waiting for container to be ready..."
    sleep 5

    echo "Container logs:"
    docker logs "$CONTAINER_NAME"
}

# Docker compose up
compose_up() {
    print_header "Starting services with docker-compose"

    if ! command -v docker-compose &> /dev/null; then
        print_error "docker-compose is not installed"
        exit 1
    fi

    cd "$PROJECT_DIR"
    docker-compose up -d

    print_success "Services started"
    echo "Waiting for services to be ready..."
    sleep 5

    docker-compose ps
}

# Docker compose down
compose_down() {
    print_header "Stopping services with docker-compose"

    cd "$PROJECT_DIR"
    docker-compose down

    print_success "Services stopped"
}

# Docker compose logs
compose_logs() {
    print_header "Docker compose logs"

    cd "$PROJECT_DIR"
    docker-compose logs -f "${1:-mission-planner}"
}

# View container logs
view_logs() {
    print_header "Container logs"

    CONTAINER_NAME="${CONTAINER_NAME:-mission-planner-mps}"
    docker logs -f "$CONTAINER_NAME"
}

# Stop container
stop_container() {
    print_header "Stopping container"

    CONTAINER_NAME="${CONTAINER_NAME:-mission-planner-mps}"

    if docker ps --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
        docker stop "$CONTAINER_NAME"
        print_success "Container stopped: $CONTAINER_NAME"
    else
        print_warning "Container $CONTAINER_NAME is not running"
    fi
}

# Remove container and image
remove_all() {
    print_header "Removing containers and images"

    CONTAINER_NAME="${CONTAINER_NAME:-mission-planner-mps}"
    IMAGE_NAME="${IMAGE_NAME:-mission-planner:latest}"

    if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
        echo "Removing container: $CONTAINER_NAME"
        docker rm -f "$CONTAINER_NAME"
    fi

    if docker image ls --format '{{.Repository}}:{{.Tag}}' | grep -q "^${IMAGE_NAME}$"; then
        echo "Removing image: $IMAGE_NAME"
        docker rmi "$IMAGE_NAME"
    fi

    print_success "Cleanup complete"
}

# Open shell in container
shell() {
    print_header "Opening shell in container"

    CONTAINER_NAME="${CONTAINER_NAME:-mission-planner-mps}"

    if ! docker ps --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
        print_error "Container $CONTAINER_NAME is not running"
        exit 1
    fi

    docker exec -it "$CONTAINER_NAME" /bin/bash
}

# Test MPS connectivity
test_connectivity() {
    print_header "Testing MPS connectivity"

    CONTAINER_NAME="${CONTAINER_NAME:-mission-planner-mps}"

    if ! docker ps --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
        print_error "Container $CONTAINER_NAME is not running"
        exit 1
    fi

    echo "Testing HTTP connection to http://localhost:9000/status"

    if curl -s -f http://localhost:9000/status > /dev/null 2>&1; then
        print_success "MPS HTTP server is responding"
        echo "Response:"
        curl -s http://localhost:9000/status | head -20
    else
        print_error "Could not connect to MPS HTTP server"
        exit 1
    fi
}

# Check container status
check_status() {
    print_header "Container status"

    CONTAINER_NAME="${CONTAINER_NAME:-mission-planner-mps}"

    if docker ps --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
        echo -e "${GREEN}Container is running${NC}"
        docker ps --filter "name=$CONTAINER_NAME" --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"

        echo -e "\n${BLUE}Health status:${NC}"
        HEALTH=$(docker inspect --format='{{.State.Health.Status}}' "$CONTAINER_NAME" 2>/dev/null || echo "Not configured")
        echo "Health: $HEALTH"
    else
        print_warning "Container $CONTAINER_NAME is not running"
        if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
            echo "Container exists but is stopped. Start it with: ./docker-helper.sh run"
        else
            echo "Container does not exist. Build and start it with: ./docker-helper.sh build && ./docker-helper.sh run"
        fi
    fi
}

# Clean up everything
clean() {
    print_header "Cleaning up all Docker resources"

    echo "This will remove:"
    echo "  - Container: mission-planner-mps"
    echo "  - Image: mission-planner:latest"
    echo ""
    read -p "Are you sure? (yes/no): " -r

    if [[ $REPLY =~ ^[Yy][Ee][Ss]$ ]]; then
        remove_all
        print_success "Cleanup complete"
    else
        echo "Cleanup cancelled"
    fi
}

# Main script logic
COMMAND="${1:-help}"

case "$COMMAND" in
    build)
        build_image
        ;;
    run)
        build_image
        run_container
        ;;
    compose-up|compose_up)
        compose_up
        ;;
    compose-down|compose_down)
        compose_down
        ;;
    compose-logs|compose_logs)
        compose_logs "$2"
        ;;
    logs)
        view_logs
        ;;
    stop)
        stop_container
        ;;
    rm|remove)
        remove_all
        ;;
    shell)
        shell
        ;;
    test)
        test_connectivity
        ;;
    status)
        check_status
        ;;
    clean)
        clean
        ;;
    help|--help|-h)
        show_help
        ;;
    *)
        print_error "Unknown command: $COMMAND"
        echo ""
        show_help
        exit 1
        ;;
esac

