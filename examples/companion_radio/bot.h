#pragma once

#include <Mesh.h>

// Forward declarations to avoid circular dependency
class MyMesh;
struct ContactInfo;

// Node discovery constants
#define ND_CTL_TYPE_NODE_DISCOVER_REQ   0x80
#define ND_CTL_TYPE_NODE_DISCOVER_RESP  0x90
#define ND_ADV_TYPE_REPEATER            2
#define ND_MAX_DISCOVERED_NODES         16

/**
 * @brief Bot class for analyzing incoming messages and responding to commands
 * 
 * This bot monitors incoming messages and automatically responds to commands
 * that start with '/' or '!' prefix.
 */
class Bot {
public:
    Bot();
    
    /**
     * @brief Initialize the bot with a reference to the mesh
     * @param mesh Pointer to the MyMesh instance
     */
    void begin(MyMesh* mesh);
    
    /**
     * @brief Check if the bot is enabled
     * @return true if bot is enabled, false otherwise
     */
    bool isEnabled() const;
    
    /**
     * @brief Enable or disable the bot
     * @param enabled true to enable, false to disable
     */
    void setEnabled(bool enabled);
    
    /**
     * @brief Analyze an incoming message and respond if it's a command
     * @param text The message text to analyze
     * @param from The contact who sent the message
     * @return true if the message was a command that was handled, false otherwise
     */
    bool analyzeMessage(const char* text, const ContactInfo& from);
    
    /**
     * @brief Handle node discover response packet
     * @param packet The received control packet
     */
    void onNodeDiscoverResponse(mesh::Packet* packet);

private:
    MyMesh* _mesh;
    bool _enabled;
    
    // Node discovery state
    uint32_t _nd_tag;
    unsigned long _nd_until;
    bool _nd_active;
    uint8_t _nd_count;
    char _nd_prefixes[ND_MAX_DISCOVERED_NODES][17];  // Store up to 16 prefixes (8 hex chars + null)
    
    /**
     * @brief Send a text message response to a contact
     */
    void sendResponse(const ContactInfo& to, const char* text);
    
    // Command handlers
    void handleHelpCommand(const ContactInfo& from);
    void handlePingCommand(const ContactInfo& from);
    void handleTimeCommand(const ContactInfo& from);
    void handleInfoCommand(const ContactInfo& from);
    void handleEchoCommand(const ContactInfo& from, const char* args);
    void handleNdCommand(const ContactInfo& from);
    
    // Node discovery helpers
    void startNodeDiscovery(const ContactInfo& from);
    void sendNodeDiscoverReq();
    void finalizeNodeDiscovery(const ContactInfo& from);
};

// Global bot instance
extern Bot the_bot;
