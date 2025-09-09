#include "RestAPI.h"
#include "Config.h"
#include "Utils.h"
#include "CommandTable.h"

/**
 * @brief Constructor for RestAPI
 * @param port HTTP port for REST API server
 * @param emgSys Pointer to EMG system
 */
RestAPI::RestAPI(int port, EMGSystem *emgSys) : server(port), emgSystem(emgSys) {}

/**
 * @brief Initialize the REST API server
 */
void RestAPI::begin()
{
    if (emgSystem != nullptr)
    {
        server.begin();
        initialized = true;
        printIfPinLow(F("REST API server started"), debugPin);
    }
    else
    {
        printIfPinLow(F("ERROR: EMGSystem not set for REST API"), debugPin);
    }
}

/**
 * @brief Main update method (call in loop) - non-blocking
 */
void RestAPI::update()
{
    if (!initialized)
        return;

    // Check for new client (non-blocking)
    WiFiClient client = server.available();
    if (!client)
        return;

    printIfPinLow(F("REST API client connected"), debugPin);

    // Read HTTP request line with timeout
    char requestLine[128] = "";
    int reqIndex = 0;
    unsigned long timeout = millis();
    bool requestComplete = false;

    while (client.connected() && millis() - timeout < 1000 && !requestComplete)
    {
        if (client.available())
        {
            char c = client.read();
            if (reqIndex < sizeof(requestLine) - 1)
            {
                requestLine[reqIndex++] = c;
                requestLine[reqIndex] = '\0';
            }
            if (c == '\n')
            {
                requestComplete = true;
            }
        }
    }

    if (requestComplete)
    {
        // Parse request
        char method[8] = "";
        char path[64] = "";
        parseRequest(requestLine, method, path, sizeof(method), sizeof(path));

        printIfPinLow(F("REST API request:"), debugPin);
        printIfPinLow(method, debugPin);
        printIfPinLow(path, debugPin);

        // Handle GET /status
        if (strcmp(method, "GET") == 0 && strcmp(path, "/status") == 0)
        {
            handleStatusEndpoint(client);
        }
        // Handle POST /send-command
        else if (strcmp(method, "POST") == 0 && strcmp(path, "/send-command") == 0)
        {
            handleSendCommandEndpoint(client);
        }
        else
        {
            // Send 404 for unknown endpoints
            send404Response(client);
        }
    }

    // Clean up client connection
    delay(10); // Small delay to ensure response is sent
    client.stop();
    printIfPinLow(F("REST API client disconnected"), debugPin);
}

/**
 * @brief Check if REST API is initialized
 * @return True if initialized
 */
bool RestAPI::isInitialized() const
{
    return initialized;
}

/**
 * @brief Send HTTP response with JSON content
 * @param client WiFi client to send response to
 * @param json JSON string to send
 */
void RestAPI::sendJSONResponse(WiFiClient &client, const char *json)
{
    // Calculate content length
    int contentLength = strlen(json);

    // Send HTTP headers
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(contentLength);
    client.println(); // Empty line to end headers

    // Send JSON body
    client.print(json);
}

/**
 * @brief Send HTTP 404 error response
 * @param client WiFi client to send response to
 */
void RestAPI::send404Response(WiFiClient &client)
{
    const char *errorJson = "{\"error\":\"Not Found\",\"message\":\"Endpoint not found\"}";
    int contentLength = strlen(errorJson);

    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(contentLength);
    client.println(); // Empty line to end headers

    client.print(errorJson);
}

/**
 * @brief Handle GET /status endpoint
 * @param client WiFi client to send response to
 */
void RestAPI::handleStatusEndpoint(WiFiClient &client)
{
    if (emgSystem == nullptr)
    {
        const char *errorJson = "{\"error\":\"Internal Server Error\",\"message\":\"EMG system not available\"}";
        int contentLength = strlen(errorJson);

        client.println("HTTP/1.1 500 Internal Server Error");
        client.println("Content-Type: application/json");
        client.println("Access-Control-Allow-Origin: *");
        client.println("Connection: close");
        client.print("Content-Length: ");
        client.println(contentLength);
        client.println(); // Empty line to end headers

        client.print(errorJson);
        return;
    }

    // Get current command from EMG system
    int currentCommand = emgSystem->getCurrentCommand();
    const char *commandLabel = getCommandLabel(currentCommand);

    char jsonResponse[300];
    snprintf(jsonResponse, sizeof(jsonResponse),
             "{"
             "\"status\":\"active\","
             "\"emg_initialized\":%s,"
             "\"current_command\":{"
             "\"code\":%d,"
             "\"label\":\"%s\""
             "},"
             "\"timestamp\":%lu"
             "}",
             emgSystem->isInitialized() ? "true" : "false",
             currentCommand,
             commandLabel,
             millis());

    sendJSONResponse(client, jsonResponse);
}

/**
 * @brief Handle POST /send-command endpoint
 * @param client WiFi client to send response to
 */
void RestAPI::handleSendCommandEndpoint(WiFiClient &client)
{
    if (emgSystem == nullptr)
    {
        const char *errorJson = "{\"error\":\"Internal Server Error\",\"message\":\"EMG system not available\"}";
        int contentLength = strlen(errorJson);

        client.println("HTTP/1.1 500 Internal Server Error");
        client.println("Content-Type: application/json");
        client.println("Access-Control-Allow-Origin: *");
        client.println("Connection: close");
        client.print("Content-Length: ");
        client.println(contentLength);
        client.println(); // Empty line to end headers

        client.print(errorJson);
        return;
    }

    // Check if EMG system is initialized
    if (!emgSystem->isInitialized())
    {
        const char *errorJson = "{\"error\":\"Service Unavailable\",\"message\":\"EMG system not initialized - client must be connected\"}";
        int contentLength = strlen(errorJson);

        client.println("HTTP/1.1 503 Service Unavailable");
        client.println("Content-Type: application/json");
        client.println("Access-Control-Allow-Origin: *");
        client.println("Connection: close");
        client.print("Content-Length: ");
        client.println(contentLength);
        client.println(); // Empty line to end headers

        client.print(errorJson);
        return;
    }

    // Get current command and send it
    int currentCommand = emgSystem->getCurrentCommand();
    const char *commandLabel = getCommandLabel(currentCommand);

    // Try to send the command via EMG system
    bool commandSent = emgSystem->sendCurrentCommand();

    if (commandSent)
    {
        printIfPinLow(F("API: Command sent successfully"), debugPin);
        char debugMsg[16];
        snprintf(debugMsg, sizeof(debugMsg), "API: Command %d", currentCommand);
        printIfPinLow(debugMsg, debugPin);

        char jsonResponse[300];
        snprintf(jsonResponse, sizeof(jsonResponse),
                 "{"
                 "\"status\":\"success\","
                 "\"message\":\"Command sent successfully\","
                 "\"command_sent\":{"
                 "\"code\":%d,"
                 "\"label\":\"%s\""
                 "},"
                 "\"timestamp\":%lu"
                 "}",
                 currentCommand,
                 commandLabel,
                 millis());

        sendJSONResponse(client, jsonResponse);
    }
    else
    {
        const char *errorJson = "{\"error\":\"Failed to Send\",\"message\":\"Command could not be sent - no TCP client connected or cooldown active\"}";
        int contentLength = strlen(errorJson);

        client.println("HTTP/1.1 400 Bad Request");
        client.println("Content-Type: application/json");
        client.println("Access-Control-Allow-Origin: *");
        client.println("Connection: close");
        client.print("Content-Length: ");
        client.println(contentLength);
        client.println(); // Empty line to end headers

        client.print(errorJson);
    }
}

/**
 * @brief Parse HTTP request line to extract method and path
 * @param requestLine HTTP request line
 * @param method Buffer to store HTTP method
 * @param path Buffer to store request path
 * @param methodSize Size of method buffer
 * @param pathSize Size of path buffer
 */
void RestAPI::parseRequest(const char *requestLine, char *method, char *path,
                           size_t methodSize, size_t pathSize)
{
    // Initialize buffers
    method[0] = '\0';
    path[0] = '\0';

    // Find first space (end of method)
    const char *methodEnd = strchr(requestLine, ' ');
    if (methodEnd == nullptr)
        return;

    // Copy method
    size_t methodLen = methodEnd - requestLine;
    if (methodLen >= methodSize)
        methodLen = methodSize - 1;
    strncpy(method, requestLine, methodLen);
    method[methodLen] = '\0';

    // Skip space and find path
    const char *pathStart = methodEnd + 1;
    const char *pathEnd = strchr(pathStart, ' ');
    if (pathEnd == nullptr)
        pathEnd = strchr(pathStart, '?'); // Handle query parameters
    if (pathEnd == nullptr)
        pathEnd = strchr(pathStart, '\r'); // Handle line ending
    if (pathEnd == nullptr)
        pathEnd = strchr(pathStart, '\n'); // Handle line ending
    if (pathEnd == nullptr)
        pathEnd = pathStart + strlen(pathStart); // End of string

    // Copy path
    size_t pathLen = pathEnd - pathStart;
    if (pathLen >= pathSize)
        pathLen = pathSize - 1;
    strncpy(path, pathStart, pathLen);
    path[pathLen] = '\0';
}
