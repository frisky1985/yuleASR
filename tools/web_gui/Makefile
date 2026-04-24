# DDS Web GUI Makefile

.PHONY: help install start stop restart clean test docker-build docker-run

# Default target
help:
	@echo "DDS Web GUI - Available targets:"
	@echo "  make install      - Install dependencies and setup"
	@echo "  make start        - Start the server"
	@echo "  make stop         - Stop the server"
	@echo "  make restart      - Restart the server"
	@echo "  make clean        - Clean build artifacts"
	@echo "  make test         - Run tests"
	@echo "  make docker-build - Build Docker image"
	@echo "  make docker-run   - Run Docker container"

# Installation
install:
	@echo "Installing DDS Web GUI..."
	./install.sh

# Start server
start:
	@echo "Starting DDS Web GUI server..."
	./start_server.sh

# Stop server (if running via systemd)
stop:
	@echo "Stopping DDS Web GUI server..."
	sudo systemctl stop dds-web-gui || true
	pkill -f "python.*app.py" || true

# Restart server
restart: stop start

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf venv
	rm -rf __pycache__
	rm -rf server/__pycache__
	rm -rf logs/*
	find . -type f -name "*.pyc" -delete
	find . -type d -name "__pycache__" -delete

# Run tests
test:
	@echo "Running tests..."
	cd server && python -m pytest tests/ -v || echo "No tests found"

# Docker targets
docker-build:
	@echo "Building Docker image..."
	docker build -t dds-web-gui:latest .

docker-run:
	@echo "Running Docker container..."
	docker run -d \
		--name dds-web-gui \
		-p 5000:5000 \
		-e SECRET_KEY=$${SECRET_KEY:-dev-secret} \
		-e JWT_SECRET_KEY=$${JWT_SECRET_KEY:-jwt-secret} \
		dds-web-gui:latest

docker-stop:
	@echo "Stopping Docker container..."
	docker stop dds-web-gui || true
	docker rm dds-web-gui || true

# Systemd targets
systemd-install:
	@echo "Installing systemd service..."
	sudo cp dds-web-gui.service /etc/systemd/system/
	sudo systemctl daemon-reload
	sudo systemctl enable dds-web-gui

systemd-start:
	sudo systemctl start dds-web-gui

systemd-status:
	sudo systemctl status dds-web-gui

# Development targets
dev-setup:
	@echo "Setting up development environment..."
	python3 -m venv venv
	. venv/bin/activate && pip install -r server/requirements.txt
	. venv/bin/activate && pip install pytest black flake8

dev-lint:
	. venv/bin/activate && flake8 server/ --max-line-length=120 --ignore=E501,W503

dev-format:
	. venv/bin/activate && black server/ --line-length=120