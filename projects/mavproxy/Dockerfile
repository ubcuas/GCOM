FROM python:3.12-slim

# Install core build dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc \
    python3-dev \
    libxml2-dev \
    libxslt1-dev \
    && rm -rf /var/lib/apt/lists/*

# Install MAVProxy
RUN pip install --no-cache-dir MAVProxy

# Patch the SRTM terrain downloader to fix the 0x80 byte crash
RUN grep -rl "pickle.loads(" /usr/local/lib/python3.*/site-packages/ | xargs -r sed -i 's/pickle\.loads(\([^)]*\))/pickle.loads(\1, encoding="latin1")/g'

EXPOSE 14550/udp 14551/udp