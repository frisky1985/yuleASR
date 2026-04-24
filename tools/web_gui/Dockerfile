# DDS Web GUI Server - Docker Image
FROM python:3.11-slim

LABEL maintainer="DDS Team"
LABEL description="DDS Visualizer Web GUI for Automotive Ethernet"

# Install system dependencies
RUN apt-get update && apt-get install -y \
    gcc \
    libyaml-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy requirements first for better caching
COPY server/requirements.txt ./
RUN pip install --no-cache-dir -r requirements.txt

# Copy application files
COPY server/ ./server/
COPY web/ ./web/

# Set environment variables
ENV FLASK_APP=server/app.py
ENV FLASK_ENV=production
ENV PYTHONUNBUFFERED=1
ENV PYTHONDONTWRITEBYTECODE=1

# Expose port
EXPOSE 5000

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD python -c "import requests; requests.get('http://localhost:5000/api/dashboard/summary')" || exit 1

# Run the application
CMD ["python", "server/app.py"]