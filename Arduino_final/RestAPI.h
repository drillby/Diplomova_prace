#ifndef REST_API_H
#define REST_API_H

#include <Arduino.h>
#include <WiFiNINA.h>
#include "EMGSystem.h"

/**
 * @class RestAPI
 * @brief Simple non-blocking REST API server for EMG system status
 */
class RestAPI
{
private:
    WiFiServer server;        // HTTP server for REST API
    EMGSystem *emgSystem;     // Pointer to EMG system
    bool initialized = false; // Initialization flag

    /**
     * @brief Send HTTP response with JSON content
     * @param client WiFi client to send response to
     * @param json JSON string to send
     */
    void sendJSONResponse(WiFiClient &client, const char *json);

    /**
     * @brief Send HTTP 404 error response
     * @param client WiFi client to send response to
     */
    void send404Response(WiFiClient &client);

    /**
     * @brief Handle GET /status endpoint
     * @param client WiFi client to send response to
     */
    void handleStatusEndpoint(WiFiClient &client);

    /**
     * @brief Handle POST /send-command endpoint
     * @param client WiFi client to send response to
     */
    void handleSendCommandEndpoint(WiFiClient &client);

    /**
     * @brief Parse HTTP request line to extract method and path
     * @param requestLine HTTP request line
     * @param method Buffer to store HTTP method
     * @param path Buffer to store request path
     * @param methodSize Size of method buffer
     * @param pathSize Size of path buffer
     */
    void parseRequest(const char *requestLine, char *method, char *path,
                      size_t methodSize, size_t pathSize);

public:
    /**
     * @brief Constructor for RestAPI
     * @param port HTTP port for REST API server
     * @param emgSys Pointer to EMG system
     */
    RestAPI(int port, EMGSystem *emgSys);

    /**
     * @brief Initialize the REST API server
     */
    void begin();

    /**
     * @brief Main update method (call in loop) - non-blocking
     */
    void update();

    /**
     * @brief Check if REST API is initialized
     * @return True if initialized
     */
    bool isInitialized() const;
};

#endif // REST_API_H
