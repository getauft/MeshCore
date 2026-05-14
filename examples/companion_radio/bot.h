#pragma once

#include <Mesh.h>

// Forward declarations to avoid circular dependency
class MyMesh;
struct ContactInfo;

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

private:
    MyMesh* _mesh;
    bool _enabled;
    
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
};

// Global bot instance
extern Bot the_bot;
