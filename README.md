# Distributed System Resource Monitor

A high-performance, secure distributed system monitoring solution built with C++ and gRPC. This tool provides real-time monitoring of system resources across multiple machines with an interactive terminal UI and configurable alerts.

## Features

- **Real-time Resource Monitoring**
  - CPU usage tracking
  - Memory utilization
  - Disk statistics
  - Process monitoring
  - Network statistics

- **Interactive Terminal UI**
  - Color-coded resource bars
  - Live process list
  - Real-time updates
  - Keyboard navigation

- **Security Features**
  - SSL/TLS encryption
  - Client certificate authentication
  - JWT token validation
  - Secure credential management

- **Alert System**
  - Configurable thresholds
  - Visual and audio alerts
  - Alert logging and history
  - Email notifications

## Prerequisites

- C++17 compatible compiler
- gRPC and Protocol Buffers
- OpenSSL
- ncurses library
- Docker (optional)
- VS Code (optional)

## Installation

1. **Install Dependencies (Ubuntu/Debian)**
```bash
# Install required packages
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libgrpc++-dev \
    protobuf-compiler \
    libprotobuf-dev \
    libssl-dev \
    libncurses5-dev \
    libprocps-dev \
    libfmt-dev
```

2. **Build from Source**
```bash
# Clone the repository
git clone https://github.com/yourusername/system-monitor.git
cd system-monitor

# Generate gRPC code
protoc --grpc_out=. --cpp_out=. system_monitor.proto

# Build server
g++ -std=c++17 server.cpp system_monitor.grpc.pb.cc system_monitor.pb.cc \
    -lgrpc++ -lprotobuf -lprocps -lssl -lcrypto -o server

# Build client
g++ -std=c++17 client.cpp system_monitor.grpc.pb.cc system_monitor.pb.cc \
    -lgrpc++ -lprotobuf -lncurses -lpanel -lfmt -lssl -lcrypto -o client
```

3. **Docker Installation**
```bash
# Build and run using Docker
docker-compose up -d
```

## Usage

1. **Start the Server**
```bash
./server
```

2. **Start the Client**
```bash
./client
```

3. **Using Docker**
```bash
# Start both server and client
docker-compose up

# Access container shell
docker-compose exec system-monitor bash
```

## Development with VS Code

1. Install required VS Code extensions:
   - Remote - SSH
   - C/C++
   - Docker
   - Remote - Containers

2. Connect to development container:
   - Press F1 or Ctrl+Shift+P
   - Select "Remote-SSH: Connect to Host"
   - Choose "system-monitor-dev"

## Configuration

- Server configuration in `config/server.conf`
- Alert thresholds in `config/alerts.conf`
- SSL certificates in `certs/`
- Logging configuration in `config/logging.conf`

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
