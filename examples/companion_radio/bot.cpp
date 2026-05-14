#include "bot.h"
#include "MyMesh.h"
#include <cstring>
#include <cctype>
#include <cstdio>

// Bot implementation for analyzing incoming messages and responding to commands

#define BOT_MAX_MSG_LEN 120

Bot::Bot() {
    _enabled = true;
    _nd_active = false;
    _nd_tag = 0;
    _nd_until = 0;
    _nd_count = 0;
    memset(&_nd_requestor, 0, sizeof(_nd_requestor));
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
    else if (strcmp(command, "nd") == 0) {
        handleNdCommand(from);
        return true;
    }
    
    // Unknown command
    return false;
}

/**
 * @brief Handle node discover response packet
 */
void Bot::onNodeDiscoverResponse(mesh::Packet* packet) {
    if (!_nd_active || !_mesh) {
        return;
    }
    
    // Check if discovery has timed out
    if (_mesh->millisHasNowPassed(_nd_until)) {
        finalizeNodeDiscovery();  // timeout
        return;
    }
    
    // Check tag matches
    uint32_t tag;
    memcpy(&tag, &packet->payload[2], 4);
    if (tag != _nd_tag) {
        return;  // Not our discovery request
    }
    
    // Check we have room for another node
    if (_nd_count >= ND_MAX_DISCOVERED_NODES) {
        return;
    }
    
    // Extract the public key prefix (first 8 bytes = 16 hex chars, but we store 8 hex = 4 bytes prefix)
    uint8_t* pub_key = &packet->payload[6];
    char prefix[17];
    // Format first 4 bytes as 8 hex characters
    snprintf(prefix, sizeof(prefix), "%02X%02X%02X%02X", 
             pub_key[0], pub_key[1], pub_key[2], pub_key[3]);
    
    // Store the prefix
    strncpy(_nd_prefixes[_nd_count], prefix, 16);
    _nd_prefixes[_nd_count][16] = '\0';
    _nd_count++;
}

/**
 * @brief Send a text message response to a contact
 * If the message is longer than BOT_MAX_MSG_LEN, it will be split into multiple parts
 */
void Bot::sendResponse(const ContactInfo& to, const char* text) {
    if (!_mesh || !text) {
        return;
    }
    
    size_t text_len = strlen(text);
    size_t offset = 0;
    uint8_t part_num = 1;
    uint8_t total_parts = (text_len + BOT_MAX_MSG_LEN - 1) / BOT_MAX_MSG_LEN;
    
    while (offset < text_len) {
        char chunk[BOT_MAX_MSG_LEN + 1];
        size_t chunk_len = min(BOT_MAX_MSG_LEN, text_len - offset);
        
        // If multiple parts, add part indicator
        if (total_parts > 1) {
            snprintf(chunk, sizeof(chunk), "[%u/%u] %.*s", 
                     part_num, total_parts, 
                     (int)chunk_len, text + offset);
        } else {
            memcpy(chunk, text + offset, chunk_len);
            chunk[chunk_len] = '\0';
        }
        
        uint32_t timestamp = _mesh->getRTCClock()->getCurrentTime();
        uint32_t expected_ack = 0;
        uint32_t est_timeout = 0;
        
        _mesh->sendMessage(to, timestamp, 0, chunk, expected_ack, est_timeout);
        
        offset += chunk_len;
        part_num++;
        
        // Small delay between parts
        if (offset < text_len) {
            delay(100);
        }
    }
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
                           "/echo <text> - Echo back text\n"
                           "/nd - Discover nearby repeaters";
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

/**
 * @brief Handle the /nd command - Node Discovery
 */
void Bot::handleNdCommand(const ContactInfo& from) {
    if (!_mesh) {
        return;
    }
    
    // Check if already running
    if (_nd_active) {
        sendResponse(from, "Discovery already in progress...");
        return;
    }
    
    startNodeDiscovery(from);
}

/**
 * @brief Start node discovery process
 */
void Bot::startNodeDiscovery(const ContactInfo& from) {
    // Store the requestor info
    _nd_requestor = from;
    _nd_active = true;
    _nd_count = 0;
    _nd_tag = 0;  // Will be set by sendNodeDiscoverReq
    
    // Clear prefixes array
    memset(_nd_prefixes, 0, sizeof(_nd_prefixes));
    
    // Send the discovery request
    sendNodeDiscoverReq();
    
    // Set timeout (60 seconds from now)
    _nd_until = millis() + 60000;
    
    // Send acknowledgment that discovery started
    sendResponse(from, "Starting node discovery... (wait ~60s)");
}

/**
 * @brief Send node discover request packet
 */
void Bot::sendNodeDiscoverReq() {
    if (!_mesh) {
        return;
    }
    
    uint8_t data[10];
    data[0] = ND_CTL_TYPE_NODE_DISCOVER_REQ;  // prefix_only=0
    data[1] = (1 << ND_ADV_TYPE_REPEATER);
    
    // Generate random tag
    _mesh->getRNG()->random(&data[2], 4);
    memcpy(&_nd_tag, &data[2], 4);
    
    uint32_t since = 0;
    memcpy(&data[6], &since, 4);
    
    auto pkt = _mesh->createControlData(data, sizeof(data));
    if (pkt) {
        _mesh->sendZeroHop(pkt);
    }
}

/**
 * @brief Finalize node discovery and send results
 */
void Bot::finalizeNodeDiscovery() {
    if (!_nd_active) {
        return;
    }
    
    _nd_active = false;
    
    // Build response message
    char response[512];
    int offset = 0;
    
    offset += snprintf(response + offset, sizeof(response) - offset, 
                       "Found %d repeater(s):\n", _nd_count);
    
    if (_nd_count > 0) {
        for (uint8_t i = 0; i < _nd_count; i++) {
            offset += snprintf(response + offset, sizeof(response) - offset, 
                               "%d: %s\n", i + 1, _nd_prefixes[i]);
        }
    } else {
        offset += snprintf(response + offset, sizeof(response) - offset, 
                           "No repeaters found.");
    }
    
    // Send response to the original requestor
    if (_nd_requestor.pubkey != nullptr && _nd_requestor.pubkey[0] != '\0') {
        sendResponse(_nd_requestor, response);
    }
    
    // Clear the requestor
    memset(&_nd_requestor, 0, sizeof(_nd_requestor));
}
