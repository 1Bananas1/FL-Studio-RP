#ifndef DISCORD_RP_H
#define DISCORD_RP_H

#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/socket.h>
#include <sys/un.h>
#endif

struct DiscordActivity {
    std::string state;
    std::string details;
    std::string largeImage;
    std::string largeText;
    std::string smallImage;
    std::string smallText;
    long long startTime;
    long long endTime;
    
    DiscordActivity() : startTime(0), endTime(0) {}
};

class DiscordRPC {
private:
#ifdef _WIN32
    HANDLE pipe;
#else
    int sock;
#endif
    bool connected;
    std::string clientId;
    
    std::string createHandshakeMessage();
    std::string createActivityMessage(const DiscordActivity& activity);
    std::string createClearActivityMessage();
    void writeMessage(const std::string& message, int opcode = 1);
    std::string readMessage();

public:
    DiscordRPC(const std::string& clientId);
    ~DiscordRPC();
    
    bool connect();
    void disconnect();
    bool updateActivity(const DiscordActivity& activity);
    bool clearActivity();
    bool isConnected() const;
    
    // Static helper to get current timestamp
    static long long getCurrentTimestamp();
};

#endif // DISCORD_RP_H