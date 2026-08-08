# DDS Visualizer - Web GUI Tool

A comprehensive web-based visualization and management tool for DDS (Data Distribution Service) on Automotive Ethernet.

## Features

### Dashboard
- Real-time system status overview
- Live metrics charts (latency, throughput, packet loss)
- Alert notifications with severity levels
- ASIL-D safety level indicator

### Node Management
- DDS participant discovery and monitoring
- Node status and health checks
- Remote node control (start/stop/restart)
- Node type classification (sensor, controller, actuator, gateway)

### Topic Management
- Topic list with QoS profiles
- Publisher/subscriber relationship view
- Real-time message rate monitoring
- Topic data flow visualization

### QoS Monitor
- Latency distribution histograms
- Throughput over time charts
- Packet loss rate tracking
- Historical statistics summary

### Network Topology
- Interactive topology graph (Vis.js)
- Node relationship visualization
- Pub/sub connection mapping
- Dynamic layout adjustment

### Configuration Management
- YAML/JSON configuration editor
- Syntax validation
- Configuration version history
- One-click deployment to nodes

### Automotive-Specific Features
- **EOL Test Mode**: End-of-Line testing interface
- **OTA Updates**: Over-the-air update and rollback management
- **ASIL Monitoring**: Safety level status tracking
- **Security Events**: Real-time security alerts

## Architecture

```
tools/web_gui/
├── server/                 # Flask backend
│   ├── app.py             # Main application
│   ├── config.py          # Configuration classes
│   └── requirements.txt   # Python dependencies
├── web/                   # Frontend assets
│   ├── templates/
│   │   └── index.html     # Main dashboard
│   ├── js/
│   │   └── app.js         # Frontend application
│   ├── css/               # Custom styles (optional)
│   └── images/            # Static images
├── start_server.sh        # Startup script
├── dds-web-gui.service    # systemd service file
├── Dockerfile             # Container configuration
└── README.md             # This file
```

## Technology Stack

### Backend
- **Framework**: Flask 2.3.3
- **WebSocket**: Flask-SocketIO
- **Authentication**: JWT (PyJWT)
- **CORS**: Flask-CORS
- **WSGI**: Eventlet/Gunicorn

### Frontend
- **UI Framework**: Bootstrap 5.3
- **Icons**: Bootstrap Icons
- **Charts**: Chart.js 4.4
- **Topology**: Vis.js
- **WebSocket**: Socket.IO Client

## Installation

### Prerequisites
- Python 3.8+
- pip
- virtualenv (recommended)

### Quick Start

1. **Navigate to web_gui directory:**
   ```bash
   cd /home/admin/eth-dds-integration/tools/web_gui
   ```

2. **Start the server:**
   ```bash
   ./start_server.sh
   ```

3. **Access the dashboard:**
   Open http://localhost:5000 in your browser

### Manual Installation

1. **Create virtual environment:**
   ```bash
   python3 -m venv venv
   source venv/bin/activate
   ```

2. **Install dependencies:**
   ```bash
   pip install -r server/requirements.txt
   ```

3. **Run the server:**
   ```bash
   cd server
   python app.py
   ```

## API Documentation

### Authentication

#### POST /api/auth/login
Login to get JWT token.

**Request:**
```json
{
  "username": "admin",
  "password": "admin123"
}
```

**Response:**
```json
{
  "token": "eyJ0eXAiOiJKV1QiLCJhbGc...",
  "user": "admin",
  "role": "admin",
  "permissions": ["read", "write", "admin"]
}
```

### Dashboard

#### GET /api/dashboard/summary
Get dashboard summary data.

**Headers:**
```
Authorization: Bearer <token>
```

### Nodes

#### GET /api/nodes
List all DDS nodes.

#### GET /api/nodes/<node_id>
Get node details.

#### POST /api/nodes/<node_id>/control
Control node (start/stop/restart).

**Request:**
```json
{
  "action": "restart"
}
```

### Topics

#### GET /api/topics
List all DDS topics.

#### GET /api/topics/<topic_id>
Get topic details.

### QoS

#### GET /api/qos/metrics
Get QoS metrics history.

#### GET /api/qos/current
Get current QoS statistics.

### Configuration

#### GET /api/config
Get current configuration.

#### POST /api/config
Update configuration.

#### POST /api/config/validate
Validate configuration syntax.

#### POST /api/config/deploy
Deploy configuration to nodes.

### System

#### POST /api/system/mode
Set system mode (normal/eol_test/ota_update).

#### POST /api/ota/rollback
Initiate OTA rollback.

#### POST /api/eol/start
Start EOL test mode.

## WebSocket Events

### Client → Server
- `subscribe_metrics`: Subscribe to metrics updates
- `unsubscribe_metrics`: Unsubscribe from metrics
- `subscribe_alerts`: Subscribe to alerts
- `request_topology`: Request topology data

### Server → Client
- `metrics_update`: Real-time metrics data
- `new_alert`: New alert notification
- `topology_update`: Topology data update

## Default Credentials

| Username  | Password     | Role      | Permissions       |
|-----------|--------------|-----------|-------------------|
| admin     | admin123     | admin     | read, write, admin|
| operator  | operator123  | operator  | read, write       |
| viewer    | viewer123    | viewer    | read              |

## Production Deployment

### Using systemd

1. **Copy service file:**
   ```bash
   sudo cp dds-web-gui.service /etc/systemd/system/
   ```

2. **Edit service file:**
   Update paths and environment variables as needed.

3. **Enable and start:**
   ```bash
   sudo systemctl daemon-reload
   sudo systemctl enable dds-web-gui
   sudo systemctl start dds-web-gui
   ```

### Using Docker

1. **Build image:**
   ```bash
   docker build -t dds-web-gui .
   ```

2. **Run container:**
   ```bash
   docker run -d -p 5000:5000 \
     -e SECRET_KEY=your-secret \
     -e JWT_SECRET_KEY=your-jwt-secret \
     dds-web-gui
   ```

### Environment Variables

| Variable          | Description                  | Default                           |
|-------------------|------------------------------|-----------------------------------|
| SECRET_KEY        | Flask secret key             | dev-secret-key                    |
| JWT_SECRET_KEY    | JWT signing key              | jwt-secret-key                    |
| DDS_CONFIG_PATH   | Path to DDS config file      | /home/admin/eth-dds-integration/...|
| LOG_LEVEL         | Logging level                | INFO                              |
| FLASK_ENV         | Flask environment            | development                       |

## Security Considerations

1. **Change default credentials** in production
2. **Use HTTPS** in production (configure reverse proxy)
3. **Set strong SECRET_KEY and JWT_SECRET_KEY**
4. **Restrict CORS origins** in production
5. **Enable authentication** for all API endpoints
6. **Use firewall rules** to restrict access

## Troubleshooting

### Server won't start
- Check if port 5000 is available
- Verify Python version (3.8+)
- Check virtual environment activation

### WebSocket connection issues
- Ensure firewall allows WebSocket connections
- Check browser console for errors
- Verify Socket.IO client version compatibility

### Authentication errors
- Check JWT token expiration
- Verify user credentials
- Ensure proper Authorization header format

## Development

### Adding New Pages
1. Add navigation link in sidebar
2. Create page HTML section
3. Add route handler in app.js
4. Implement backend API if needed

### Adding New API Endpoints
1. Add route in app.py
2. Implement handler function
3. Add authentication decorator
4. Update API documentation

## License

Part of ETH-DDS-Integration project for Automotive Ethernet.

## Support

For issues and feature requests, please refer to the project documentation or contact the development team.