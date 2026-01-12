# ProjetPS - Network Chat System

## 📖 Overview

**ProjetPS** is a professional multi-client TCP/UDP chat server with automatic service discovery using multicast and a custom TLV (Type-Length-Value) protocol implementation. The system enables real-time communication between multiple clients through a centralized server with robust error handling and IPv6 support.

## ✨ Key Features

- 🌐 **Dual-Stack IPv6/IPv4 Support** - Full compatibility with both IPv4 and IPv6 networks
- 🔍 **Multicast Service Discovery** - Automatic server detection using UDP multicast (239.0.0.1:9999)
- 📦 **TLV Protocol** - Custom Type-Length-Value protocol for structured message exchange
- 🔒 **User Authentication** - Nickname-based login system with duplicate prevention
- 💬 **Real-time Chat** - Instant message delivery to all connected clients
- 🎯 **Interactive Menu** - User-friendly command-line interface
- ⚡ **Concurrent Client Support** - Handle up to 5000 simultaneous users
- 🛡️ **Robust Error Handling** - Comprehensive error checking and recovery

## 🏗️ Architecture

### Protocol Stack

```
┌─────────────────────────────────┐
│     Application Layer           │
│  (Chat Messages, Login, Menu)   │
├─────────────────────────────────┤
│     TLV Protocol Layer          │
│  (Message Encoding/Decoding)    │
├─────────────────────────────────┤
│     Transport Layer             │
│  TCP (Chat) | UDP (Discovery)   │
├─────────────────────────────────┤
│     Network Layer               │
│     IPv6 / IPv4-mapped          │
└─────────────────────────────────┘
```

### System Components

#### Server (`server.c`)
- Main TCP server listening on port 8080
- Handles client connections and message broadcasting
- Manages active user sessions

#### Client (`client.c`)
- Interactive chat client with menu system
- Automatic server discovery
- Message sending and receiving

#### TLV Protocol (`tlv.c/h`)
- Message type definitions
- Binary encoding/decoding
- Network byte order conversion

#### Multicast Discovery (`multicast_discovery.c/h`)
- UDP multicast server announcements
- Client discovery requests
- Server information broadcasting

#### Server Utilities (`server_utils.c/h`)
- Local IP address detection
- User authentication
- Nickname management (up to 5000 users)

#### Client Utilities (`client_utils.c/h`)
- Client-side protocol handling
- Connection management

#### Menu System (`menu.c/h`)
- ASCII art banner
- Interactive user interface
- Command processing

## 🔧 TLV Protocol Specification

### Message Types

| Type | Name | Description |
|------|------|-------------|
| 0x01 | DISCOVER_REQUEST | Client requests server discovery |
| 0x02 | DISCOVER_RESPONSE | Server responds with connection info |
| 0x03 | LOGIN_REQUEST | Client sends nickname for authentication |
| 0x04 | LOGIN_RESPONSE | Server confirms/rejects login |
| 0x05 | REQUEST_QUESTION | Client requests a question |
| 0x06 | QUESTION_DATA | Server sends question data |
| 0x07 | ANSWER_SUBMIT | Client submits an answer |
| 0x08 | ANSWER_RESULT | Server sends result of answer |

### Message Format

```
┌──────────────┬──────────────┬─────────────────┐
│ Type (2B)    │ Length (2B)  │ Value (N bytes) │
│ Network BO   │ Network BO   │ Payload data    │
└──────────────┴──────────────┴─────────────────┘
```

- **Type**: 2-byte unsigned integer (network byte order)
- **Length**: 2-byte unsigned integer indicating payload size (network byte order)
- **Value**: Variable-length payload data

## 🚀 Building the Project

### Prerequisites

```bash
sudo apt-get install build-essential cmake doxygen
```

### Build Instructions

```bash
cd ProjetPS
mkdir -p build
cd build
cmake ..
make
```

### Build Outputs

- `server` - Chat server executable
- `client` - Chat client executable

## 📝 Usage

### Starting the Server

```bash
cd build
./server
```

The server will:
1. Start TCP server on port 8080
2. Launch multicast discovery service
3. Display local IP address
4. Wait for client connections

### Running the Client

```bash
cd build
./client
```

The client will:
1. Display welcome banner
2. Automatically discover available servers
3. Prompt for nickname
4. Connect to selected server
5. Enter chat mode

### Client Commands

- **Type message** - Send message to all users
- **Ctrl+C** - Disconnect and exit

## 📊 Data Structures

### `tlv_header`
```c
struct tlv_header {
    uint16_t type;    // Message type
    uint16_t length;  // Payload length
};
```

### `server_info_t`
```c
typedef struct {
    char ip[INET6_ADDRSTRLEN];  // Server IPv6 address
    uint16_t port;               // Server TCP port
} server_info_t;
```

### `login_request`
```c
struct login_request {
    char nickname[32];  // User nickname
};
```

### `login_response`
```c
struct login_response {
    uint8_t status;         // 0=success, 1=error
    char message[256];      // Status message
};
```

## 🌐 Network Configuration

### Multicast Discovery
- **Group Address**: 239.0.0.1
- **Port**: 9999
- **Protocol**: UDP
- **Interval**: Server announces every 5 seconds

### Chat Service
- **Port**: 8080
- **Protocol**: TCP
- **IPv6**: Enabled (dual-stack)
- **Max Connections**: 5000

## 🔍 Key Functions

### TLV Protocol Functions
- `tlv_parse_header()` - Parse TLV header from buffer
- `tlv_create_login_request()` - Create login request message
- `tlv_create_login_response()` - Create login response message
- `tlv_parse_login_request()` - Parse login request from buffer

### Network Functions
- `snd_udp_socket()` - Create UDP socket with specific address
- `mcast_join()` - Join multicast group
- `get_local_ip()` - Detect local IP address
- `discover_server()` - Find available servers

### Server Functions
- `server_handle_login()` - Process user authentication
- `start_discovery_service()` - Launch multicast announcements

## 🛠️ Development

### Code Style
- **Language**: C (C99)
- **Formatting**: 4-space indentation
- **Comments**: Doxygen-compatible
- **Error Handling**: Return codes with perror()

### Documentation
Generate HTML documentation:
```bash
doxygen Doxyfile
xdg-open doc/html/index.html
```

## 📁 Project Structure

```
ProjetPS/
├── CMakeLists.txt              # Build configuration
├── Doxyfile                    # Documentation config
├── .gitignore                  # Git ignore rules
├── include/                    # Header files
│   ├── tlv.h
│   ├── server_utils.h
│   ├── client_utils.h
│   ├── multicast_discovery.h
│   └── menu.h
├── src/                        # Source files
│   ├── server.c
│   ├── client.c
│   ├── tlv.c
│   ├── server_utils.c
│   ├── client_utils.c
│   ├── multicast_discovery.c
│   └── menu.c
├── doc/                        # Documentation
│   ├── protocol.md
│   └── Zalozenia_Projektowe.md
└── build/                      # Build directory
    ├── server
    └── client
```

## 🎓 Technical Concepts

### Network Byte Order
All multi-byte integers in the TLV protocol use **network byte order** (big-endian) for cross-platform compatibility:
- `htons()` - Host to network short (16-bit)
- `ntohs()` - Network to host short (16-bit)

### IPv4-Mapped IPv6 Addresses
The system uses IPv6 sockets with IPv4 compatibility:
- IPv4 address `192.168.1.100` → IPv6 `::ffff:192.168.1.100`
- Enables single socket to handle both protocols

### Multicast Groups
UDP multicast allows one-to-many communication:
- Server joins multicast group
- Multiple clients receive announcements
- `SO_REUSEADDR` allows multiple listeners

## 🐛 Debugging

### Enable Verbose Output
```bash
# Server with debug info
./server -v

# Client with debug info
./client -v
```

### Common Issues

**Port already in use:**
```bash
# Find process using port 8080
sudo lsof -i :8080
# Kill the process
kill -9 <PID>
```

**Multicast not working:**
- Check firewall rules
- Verify network supports multicast
- Ensure `SO_REUSEADDR` is set

## 📜 License

This project is developed for educational purposes as part of Network Programming course (Programowanie Sieciowe) at the university.

## 👤 Author

**Bartosz Hadała**
- Semester 5, Network Programming Course
- University Project (ProjetPS)

## 🔮 Future Enhancements

- [ ] Implement `epoll()` for better concurrent connection handling
- [ ] Add SSL/TLS encryption
- [ ] Private messaging between users
- [ ] Chat rooms/channels
- [ ] Message persistence
- [ ] User authentication with passwords
- [ ] File transfer capability
- [ ] Web-based admin panel

---

**Built with ❤️ using C, CMake, and Doxygen**
