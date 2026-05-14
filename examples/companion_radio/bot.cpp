#include "bot.h"
#include <cstring>
#include <cctype>

// Bot implementation for analyzing incoming messages and responding to commands

Bot::Bot() {
    _enabled = true;
}

void Bot::begin(MyMesh* mesh) {
    _mesh = mesh;
}

bool Bot::isEnabled() const {
    return _enabled;
}

void Bot::setEnabled(bool enabled) {
    _enabled = enabled;
}

/**
 * @brief Analyze incoming message text and check if it contains a command
 * @param text The message text to analyze
 * @param from The contact who sent the message
 * @return true if the message was a command that was handled, false otherwise
 */
bool Bot::analyzeMessage(const char* text, const ContactInfo& from) {
    if (!_enabled || !text || !_mesh) {
        return false;
    }

    // Skip empty messages
    if (strlen(text) == 0) {
        return false;
    }

    // Check if message starts with a command prefix
    const char* cmdStart = text;
    
    // Skip leading whitespace
    while (*cmdStart && isspace(*cmdStart)) {
        cmdStart++;
    }
    
    // Check for command prefix '/' or '!'
    if (*cmdStart != '/' && *cmdStart != '!') {
        return false;  // Not a command
    }
    
    cmdStart++;  // Skip the prefix
    
    // Extract command name
    char command[32];
    int i = 0;
    while (cmdStart[i] && !isspace(cmdStart[i]) && i < sizeof(command) - 1) {
        command[i] = tolower(cmdStart[i]);
        i++;
    }
    command[i] = '\0';
    
    // Get arguments (if any)
    const char* args = cmdStart + i;
    while (*args && isspace(*args)) {
        args++;
    }
    
    // Process known commands
    if (strcmp(command, "help") == 0) {
        handleHelpCommand(from);
        return true;
    }
    else if (strcmp(command, "ping") == 0) {
        handlePingCommand(from);
        return true;
    }
    else if (strcmp(command, "time") == 0) {
        handleTimeCommand(from);
        return true;
    }
    else if (strcmp(command, "info") == 0) {
        handleInfoCommand(from);
        return true;
    }
    else if (strcmp(command, "echo") == 0) {
        handleEchoCommand(from, args);
        return true;
    }
    
    // Unknown command
    return false;
}

/**
 * @brief Send a text message response to a contact
 */
void Bot::sendResponse(const ContactInfo& to, const char* text) {
    if (!_mesh || !text) {
        return;
    }
    
    uint32_t timestamp = _mesh->getRTCClock()->getCurrentTime();
    uint32_t expected_ack = 0;
    uint32_t est_timeout = 0;
    
    _mesh->sendMessage(to, timestamp, 0, text, expected_ack, est_timeout);
}

/**
 * @brief Handle the /help command
 */
void Bot::handleHelpCommand(const ContactInfo& from) {
    const char* helpText = "Bot Commands:\n"
                           "/help - Show this help\n"
                           "/ping - Get pong response\n"
                           "/time - Get current time\n"
                           "/info - Get bot info\n"
                           "/echo <text> - Echo back text";
    sendResponse(from, helpText);
}

/**
 * @brief Handle the /ping command
 */
void Bot::handlePingCommand(const ContactInfo& from) {
    sendResponse(from, "Pong!");
}

/**
 * @brief Handle the /time command
 */
void Bot::handleTimeCommand(const ContactInfo& from) {
    if (!_mesh) {
        return;
    }
    
    uint32_t currentTime = _mesh->getRTCClock()->getCurrentTime();
    char timeStr[64];
    
    // Convert to hours, minutes, seconds
    uint32_t secs = currentTime % 60;
    uint32_t mins = (currentTime / 60) % 60;
    uint32_t hours = (currentTime / 3600) % 24;
    
    snprintf(timeStr, sizeof(timeStr), "Current time: %02lu:%02lu:%02lu (uptime: %lu sec)", 
             hours, mins, secs, currentTime);
    
    sendResponse(from, timeStr);
}

/**
 * @brief Handle the /info command
 */
void Bot::handleInfoCommand(const ContactInfo& from) {
    char infoStr[128];
    snprintf(infoStr, sizeof(infoStr), "MeshCore Bot v1.0\nNode: %s", 
             _mesh ? _mesh->getNodeName() : "unknown");
    sendResponse(from, infoStr);
}

/**
 * @brief Handle the /echo command
 */
void Bot::handleEchoCommand(const ContactInfo& from, const char* args) {
    if (args && strlen(args) > 0) {
        char echoStr[256];
        snprintf(echoStr, sizeof(echoStr), "Echo: %s", args);
        sendResponse(from, echoStr);
    } else {
        sendResponse(from, "Usage: /echo <text>");
    }
}
