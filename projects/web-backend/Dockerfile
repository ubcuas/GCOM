FROM python:3.12-slim

# Prevent Python from buffering stdout/stderr and writing .pyc files
ENV PYTHONUNBUFFERED=1 \
    PYTHONDONTWRITEBYTECODE=1

WORKDIR /backend

COPY requirements.txt .

RUN pip install --no-cache-dir -r requirements.txt

COPY . .
# RUN chmod +x /src/server.py
# CMD ["source", "venv/bin/activate", "&&", 
CMD ["python", "src/server.py"]
EXPOSE 8000

