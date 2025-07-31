#include "discord_rp.h"
#include <iostream>
#include <sstream>
#include <chrono>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#include <cstring>
#endif

DiscordRPC::DiscordRPC(const std::string& clientId) : clientId(clientId), connected(false) {
#ifdef _WIN32
    pipe = INVALID_HANDLE_VALUE;
#else
    sock = -1;
#endif
}

DiscordRPC::~DiscordRPC() {
    disconnect();
}

std::string DiscordRPC::createHandshakeMessage() {
    std::string handshake = R"({"v":1,"client_id":")" + clientId + R"("})";
    return handshake;
}

std::string DiscordRPC::createActivityMessage(const DiscordActivity& activity) {
    auto now = getCurrentTimestamp();
    
    std::stringstream ss;
    ss << R"({
        "cmd": "SET_ACTIVITY",
        "nonce": ")" << now << R"(",
        "args": {
            "pid": )" << 
#ifdef _WIN32
    GetCurrentProcessId() 
#else
    getpid()
#endif
    << R"(,
            "activity": {)";
    
    if (!activity.state.empty()) {
        ss << R"("state": ")" << activity.state << R"(",)";
    }
    
    if (!activity.details.empty()) {
        ss << R"("details": ")" << activity.details << R"(",)";
    }
    
    // Add timestamps if provided
    if (activity.startTime > 0 || activity.endTime > 0) {
        ss << R"("timestamps": {)";
        if (activity.startTime > 0) {
            ss << R"("start": )" << activity.startTime;
            if (activity.endTime > 0) ss << ",";
        }
        if (activity.endTime > 0) {
            ss << R"("end": )" << activity.endTime;
        }
        ss << "},";
    }
    
    // Add assets if provided
    if (!activity.largeImage.empty() || !activity.smallImage.empty()) {
        ss << R"("assets": {)";
        if (!activity.largeImage.empty()) {
            ss << R"("large_image": ")" << activity.largeImage << R"(")";
            if (!activity.largeText.empty()) {
                ss << R"(, "large_text": ")" << activity.largeText << R"(")";
            }
            if (!activity.smallImage.empty()) ss << ",";
        }
        if (!activity.smallImage.empty()) {
            ss << R"("small_image": ")" << activity.smallImage << R"(")";
            if (!activity.smallText.empty()) {
                ss << R"(, "small_text": ")" << activity.smallText << R"(")";
            }
        }
        ss << "},";
    }
    
    // Remove trailing comma if exists
    std::string result = ss.str();
    if (result.back() == ',') {
        result.pop_back();
    }
    
    result += R"(
            }
        }
    })";
    
    return result;
}

std::string DiscordRPC::createClearActivityMessage() {
    auto now = getCurrentTimestamp();
    
    std::stringstream ss;
    ss << R"({
        "cmd": "SET_ACTIVITY",
        "nonce": ")" << now << R"(",
        "args": {
            "pid": )" << 
#ifdef _WIN32
    GetCurrentProcessId() 
#else
    getpid()
#endif
    << R"(,
            "activity": null
        }
    })";
    
    return ss.str();
}

void DiscordRPC::writeMessage(const std::string& message, int opcode) {
    uint32_t length = static_cast<uint32_t>(message.length());
    uint32_t op = static_cast<uint32_t>(opcode);
    
#ifdef _WIN32
    DWORD written;
    WriteFile(pipe, &op, sizeof(op), &written, nullptr);
    WriteFile(pipe, &length, sizeof(length), &written, nullptr);
    WriteFile(pipe, message.c_str(), length, &written, nullptr);
#else
    write(sock, &op, sizeof(op));
    write(sock, &length, sizeof(length));
    write(sock, message.c_str(), length);
#endif
}

std::string DiscordRPC::readMessage() {
    uint32_t opcode, length;
    
#ifdef _WIN32
    DWORD read;
    if (!ReadFile(pipe, &opcode, sizeof(opcode), &read, nullptr) || read != sizeof(opcode)) {
        return "";
    }
    if (!ReadFile(pipe, &length, sizeof(length), &read, nullptr) || read != sizeof(length)) {
        return "";
    }
    
    if (length > 0 && length < 65536) { // Sanity check
        char* buffer = new char[length + 1];
        if (ReadFile(pipe, buffer, length, &read, nullptr) && read == length) {
            buffer[length] = '\0';
            std::string result(buffer);
            delete[] buffer;
            return result;
        }
        delete[] buffer;
    }
#else
    if (read(sock, &opcode, sizeof(opcode)) != sizeof(opcode)) {
        return "";
    }
    if (read(sock, &length, sizeof(length)) != sizeof(length)) {
        return "";
    }
    
    if (length > 0 && length < 65536) { // Sanity check
        char* buffer = new char[length + 1];
        if (read(sock, buffer, length) == static_cast<ssize_t>(length)) {
            buffer[length] = '\0';
            std::string result(buffer);
            delete[] buffer;
            return result;
        }
        delete[] buffer;
    }
#endif
    return "";
}

bool DiscordRPC::connect() {
#ifdef _WIN32
    // Try connecting to Discord IPC pipes (discord-ipc-0 through discord-ipc-9)
    for (int i = 0; i < 10; i++) {
        std::string pipeName = "\\\\.\\pipe\\discord-ipc-" + std::to_string(i);
        
        pipe = CreateFileA(
            pipeName.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );
        
        if (pipe != INVALID_HANDLE_VALUE) {
            connected = true;
            break;
        }
    }
#else
    // Try connecting to Discord IPC sockets on Unix
    for (int i = 0; i < 10; i++) {
        sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock == -1) continue;
        
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        
        std::string socketPath;
        const char* tmpdir = getenv("XDG_RUNTIME_DIR");
        if (!tmpdir) tmpdir = getenv("TMPDIR");
        if (!tmpdir) tmpdir = "/tmp";
        
        socketPath = std::string(tmpdir) + "/discord-ipc-" + std::to_string(i);
        strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);
        
        if (::connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            connected = true;
            break;
        }
        
        close(sock);
        sock = -1;
    }
#endif
    
    if (!connected) {
        return false;
    }
    
    // Send handshake
    std::string handshake = createHandshakeMessage();
    writeMessage(handshake, 0);  // Opcode 0 for handshake
    
    // Read response
    std::string response = readMessage();
    return !response.empty();
}

void DiscordRPC::disconnect() {
    if (connected) {
#ifdef _WIN32
        if (pipe != INVALID_HANDLE_VALUE) {
            CloseHandle(pipe);
            pipe = INVALID_HANDLE_VALUE;
        }
#else
        if (sock != -1) {
            close(sock);
            sock = -1;
        }
#endif
        connected = false;
    }
}

bool DiscordRPC::updateActivity(const DiscordActivity& activity) {
    if (!connected) return false;
    
    std::string activityJson = createActivityMessage(activity);
    writeMessage(activityJson, 1);  // Opcode 1 for frame
    
    // Read response
    std::string response = readMessage();
    return !response.empty();
}

bool DiscordRPC::clearActivity() {
    if (!connected) return false;
    
    std::string clearJson = createClearActivityMessage();
    writeMessage(clearJson, 1);  // Opcode 1 for frame
    
    // Read response
    std::string response = readMessage();
    return !response.empty();
}

bool DiscordRPC::isConnected() const {
    return connected;
}

long long DiscordRPC::getCurrentTimestamp() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}