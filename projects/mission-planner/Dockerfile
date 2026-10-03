# Multi-stage Dockerfile for Mission Planner with ArduCopter 4.5.5 SITL on ARM64
# For Linux devices running ARM architecture
# Note: Mavproxy runs as a separate service in docker-compose

# Stage 1: Base image with essential tools
# FROM arm64v8/python:3.11-slim AS base
FROM python:3.12-slim AS base

# Set environment variables
# ENV PYTHONUNBUFFERED=1 \
#     PYTHONDONTWRITEBYTECODE=1 
# ENV PATH="/root/.local/bin:$PATH"

# Install system dependencies
# RUN apt-get update && apt-get install -y \
#     build-essential \
#     git \
#     wget \
#     curl \
#     sudo \
#     pkg-config \
#     libfreetype6-dev \
#     libpng-dev \
#     && rm -rf /var/lib/apt/lists/*

# Stage 2: Mission Planner application
FROM base AS mps

# Set working directory
WORKDIR /mission-planner

# Copy mission planner project files
COPY requirements.txt .
# COPY pyproject.toml .
COPY src/ ./src/
# COPY 
# COPY technical-documentation/ ./technical-documentation/

# Install Python dependencies from requirements.txt
# RUN pip install --no-cache-dir --upgrade pip && \
#     pip install --no-cache-dir -r requirements.txt && \
    # pip install --no-cache-dir future

RUN pip install --no-cache-dir --upgrade pip && \
    pip install --no-cache-dir -r requirements.txt
    
# Create entrypoint script
# RUN mkdir -p /app/scripts && \
#     cat > /app/scripts/entrypoint.sh << 'ENTRYPOINT_EOF' && 
RUN mkdir ./scripts
COPY entrypoint.sh ./scripts/
RUN chmod +x ./scripts/entrypoint.sh

# Expose ports
# Mission Planner Server HTTP port
EXPOSE 9000

# Status WebSocket port
EXPOSE 8000

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=40s --retries=3 \
    CMD curl -f http://localhost:9000/status || exit 1


# Set the entrypoint
ENTRYPOINT ["./scripts/entrypoint.sh"]
