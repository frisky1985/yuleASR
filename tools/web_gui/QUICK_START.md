# DDS Web GUI - Quick Start Guide

## 🚀 Getting Started in 3 Steps

### Step 1: Install Dependencies
```bash
cd /home/admin/eth-dds-integration/tools/web_gui
./install.sh
```

This will:
- Create a Python virtual environment
- Install all required packages
- Set up necessary directories

### Step 2: Start the Server
```bash
./start_server.sh
```

Or using Make:
```bash
make start
```

### Step 3: Open Dashboard
Open your browser and navigate to:
```
http://localhost:5000
```

Default login credentials:
- **Admin**: `admin` / `admin123`
- **Operator**: `operator` / `operator123`
- **Viewer**: `viewer` / `viewer123`

---

## 📁 Project Structure

```
web_gui/
├── server/              # Flask backend
│   ├── app.py          # Main application (615 lines)
│   ├── dds_client.py   # DDS runtime interface
│   ├── config.py       # Configuration classes
│   └── requirements.txt # Python deps
├── web/                 # Frontend assets
│   ├── templates/
│   │   └── index.html  # Main dashboard (825 lines)
│   ├── js/
│   │   ├── app.js      # Main app logic (698 lines)
│   │   └── charts.js   # Chart utilities (378 lines)
│   └── css/
│       └── custom.css  # Custom styles (186 lines)
├── start_server.sh      # Startup script
├── install.sh           # Installation script
├── test_server.py       # Test suite
├── Makefile             # Build automation
├── Dockerfile           # Container config
├── dds-web-gui.service  # systemd service
└── README.md            # Full documentation
```

---

## 🔧 Available Make Commands

```bash
make install       # Install dependencies
make start         # Start server
make stop          # Stop server
make restart       # Restart server
make clean         # Clean build artifacts
make test          # Run tests
make docker-build  # Build Docker image
make docker-run    # Run Docker container
```

---

## 📦 Docker Deployment

Build and run with Docker:

```bash
# Build image
make docker-build

# Run container
make docker-run

# Or manually:
docker build -t dds-web-gui .
docker run -d -p 5000:5000 dds-web-gui
```

---

## 💾 systemd Service

Install as a system service:

```bash
# Install service
make systemd-install

# Start service
make systemd-start

# Check status
make systemd-status
```

---

## 📊 Features Overview

| Feature | Description |
|---------|-------------|
| **Dashboard** | Real-time system overview with live charts |
| **Node Management** | View and control DDS participants |
| **Topic Monitor** | Track DDS topics and pub/sub relationships |
| **QoS Metrics** | Latency, throughput, packet loss monitoring |
| **Topology View** | Interactive network visualization |
| **Configuration** | YAML editor with validation |
| **EOL Test** | End-of-Line testing mode |
| **OTA Updates** | Over-the-air update management |

---

## 🔐 API Endpoints

### Authentication
- `POST /api/auth/login` - Get JWT token

### Dashboard
- `GET /api/dashboard/summary` - System overview

### Nodes
- `GET /api/nodes` - List all nodes
- `GET /api/nodes/<id>` - Get node details
- `POST /api/nodes/<id>/control` - Control node

### Topics
- `GET /api/topics` - List all topics

### QoS
- `GET /api/qos/metrics` - QoS metrics history
- `GET /api/qos/current` - Current QoS stats

### Configuration
- `GET /api/config` - Get configuration
- `POST /api/config` - Update configuration
- `POST /api/config/validate` - Validate config
- `POST /api/config/deploy` - Deploy config

---

## 📱 WebSocket Events

### Subscribe to real-time updates:
```javascript
socket.emit('subscribe_metrics');  // Real-time metrics
socket.emit('subscribe_alerts');   // Alert notifications
```

### Receive updates:
```javascript
socket.on('metrics_update', (data) => {
    // Update charts with new data
});

socket.on('new_alert', (alert) => {
    // Display new alert
});
```

---

## 📝 Troubleshooting

### Port already in use
```bash
# Find process using port 5000
sudo lsof -i :5000

# Kill the process
kill -9 <PID>
```

### Permission denied
```bash
# Make scripts executable
chmod +x install.sh start_server.sh test_server.py
```

### Import errors
```bash
# Reinstall dependencies
rm -rf venv
./install.sh
```

---

## 📖 Documentation

- Full documentation: [README.md](README.md)
- API reference: See README.md API section
- Configuration: See `server/config.py`

---

## 🎯 Development Status

- ✅ Backend API (Flask + WebSocket)
- ✅ Frontend Dashboard (Bootstrap 5)
- ✅ Real-time Charts (Chart.js)
- ✅ Topology Visualization (Vis.js)
- ✅ Authentication (JWT)
- ✅ DDS Client Integration
- ✅ Docker Support
- ✅ systemd Service
- ✅ EOL Test Mode
- ✅ OTA Management

---

**Total Lines of Code**: ~3,000+

Ready to use for DDS monitoring and management!