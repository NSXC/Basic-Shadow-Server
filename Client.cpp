#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <string>
#include <unordered_map>
#include <winsock2.h>
#include <thread> 
#include <random>
#include <sstream>
#include <iomanip>
#include "crystal.h"
#include <stdio.h>
#include <cstdlib>
#include <cstdio>
#include <intrin.h>
#include <codecvt>
#include <string>
#include <iomanip>
//FOR GAMBLING USE THIS IS PVP CLIENT
#include "microjson.h"
#pragma comment(lib, "ws2_32.lib")

SOCKET clientSocket; 
bool shouldExit = false;

void cleanup() {
    closesocket(clientSocket);
    WSACleanup();
}

void HideConsole()
{
    ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
}

void ShowConsole()
{
    ::ShowWindow(::GetConsoleWindow(), SW_SHOW);
}

bool IsConsoleVisible()
{
    return ::IsWindowVisible(::GetConsoleWindow()) != FALSE;
}

std::wstring GetHardDriveSerialNumber() {
    DWORD serialNumber;
    wchar_t volumeName[MAX_PATH + 1];
    wchar_t fileSystemName[MAX_PATH + 1];
    if (GetVolumeInformationW(L"C:\\", volumeName, ARRAYSIZE(volumeName), &serialNumber, nullptr, nullptr, fileSystemName, ARRAYSIZE(fileSystemName))) {
        std::wstringstream ss;
        ss << std::hex << std::setw(8) << std::setfill(L'0') << serialNumber;
        return ss.str();
    }
    return L"";
}



std::unordered_map<std::string, std::string> parse_params(const std::string& url) {
    std::unordered_map<std::string, std::string> params;
    size_t params_start = url.find("?");

    if (params_start != std::string::npos) {
        std::string params_string = url.substr(params_start + 1);
        size_t pos = 0;

        while ((pos = params_string.find("&")) != std::string::npos) {
            std::string param = params_string.substr(0, pos);
            size_t equals_pos = param.find("=");
            if (equals_pos != std::string::npos) {
                std::string key = param.substr(0, equals_pos);
                std::string value = param.substr(equals_pos + 1);
                params[key] = value;
            }
            params_string.erase(0, pos + 1);
        }

        size_t equals_pos = params_string.find("=");
        if (equals_pos != std::string::npos) {
            std::string key = params_string.substr(0, equals_pos);
            std::string value = params_string.substr(equals_pos + 1);
            params[key] = value;
        }
    }

    return params;
}

std::string sendMessage(const std::string& message) {
    std::string response;

    if (send(clientSocket, message.c_str(), message.size(), 0) != SOCKET_ERROR) {
        char buffer[1024];
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            response = buffer;
        }
        else if (bytesRead == 0) {
            response = "Connection closed by server.";
        }
        else {
            response = "Error while receiving response.";
        }
    }
    else {
        response = "Failed to send message.";
    }

    return response;
}
std::string buildpacket(const std::string& messageId,
    const std::string& topic,
    const std::string& data,
    const std::string& dataType) {
    std::stringstream ss;
    ss << R"({
        ")" << messageId << R"(": {
            "messageid": ")" << messageId << R"(",
            "topic": ")" << topic << R"(",
            "data": ")" << data << R"(",
            "datatype": ")" << dataType << R"("
        }
    })";

    return ss.str();
}
std::string generateUUID() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;

    uint64_t part1 = dis(gen);


    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << part1;


    return ss.str();
}
std::string ConvertWStringToString(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

int main(int argc, char* argv[]) {
    //HideConsole();
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize WinSock." << std::endl;
        return 1;
    }

    if (argc > 1) {
        std::string argStr = argv[1];
        std::unordered_map<std::string, std::string> params = parse_params(argStr);
        auto cookieIt = params.find("cookie");
        auto userIdIt = params.find("userid");
        auto gameIdIt = params.find("gameid");
        auto jointokenIt = params.find("jointoken");

        if (cookieIt != params.end() && userIdIt != params.end() && gameIdIt != params.end() && jointokenIt != params.end()) {
            std::string cookie = cookieIt->second;
            uint64_t userid = std::stoull(userIdIt->second);
            uint64_t gameid = std::stoull(gameIdIt->second);
            std::string jointoken = jointokenIt->second;

            if (cookie == cookie) {
                clientSocket = socket(AF_INET, SOCK_STREAM, 0);
                if (clientSocket == INVALID_SOCKET) {
                    std::cerr << "Failed to create socket." << std::endl;
                    WSACleanup();
                    return 1;
                }

                sockaddr_in serverAddress;
                serverAddress.sin_family = AF_INET;
                serverAddress.sin_port = htons(238);
                serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

                if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
                    MessageBox(NULL, L"Failed to connect to the server", L"Error", MB_OK);
                    std::cerr << "Failed to connect to the server." << std::endl;
                    cleanup();
                    return 1;
                }
                std::string uuid = generateUUID();
                std::string testingpacket = buildpacket(uuid, "message", "ping", "json");
                std::cout << "Contacting Server" << std::endl;
                size_t start = cookie.find("%7CWARNING:-");
                size_t end = cookie.find("%7C_", start);

                if (start != std::string::npos && end != std::string::npos) {
                    cookie.erase(start, end - start + 4);
                }
                cookie.erase(std::remove_if(cookie.begin(), cookie.end(), [](char c) {
                    return !std::isalnum(c); 
                    }), cookie.end());
                std::string verifypacket = buildpacket(uuid, "verify", cookie+"=>"+std::to_string(gameid), "json");
                std::string response = sendMessage(testingpacket);
                if (response == "pong") {
                    std::cout << "Connection Established" << std::endl;
                }
                else {
                    MessageBox(NULL, L"A Random Error Has Occured", L"Error", MB_OK);
                }
                std::string verifycallback = sendMessage(verifypacket);        
                if (verifycallback == "Bad Cookie") {
                    MessageBox(NULL, L"Your Cookie Is Invalid", L"Error", MB_OK);
                }
                else if (verifycallback == "Error") {
                    std::cout << verifycallback;
                    MessageBox(NULL, L"A Random Error Has Occured", L"Error", MB_OK);
                }
                else if (verifycallback != "Bad Cookie") {
                    std::string ipAddress;
                    FILE* cmdStream = _popen("curl -s ifconfig.me", "r");
                    if (cmdStream) {
                        char buffer[128];
                        while (fgets(buffer, sizeof(buffer), cmdStream) != nullptr) {
                            ipAddress += buffer;
                        }
                        _pclose(cmdStream);
                    }

                    std::string jsonResult;

                    if (!ipAddress.empty()) {
                        std::string command = "curl -s http://ip-api.com/json/" + ipAddress;
                        cmdStream = _popen(command.c_str(), "r");
                        if (cmdStream) {
                            char buffer[128];
                            while (fgets(buffer, sizeof(buffer), cmdStream) != nullptr) {
                                jsonResult += buffer;
                            }
                            _pclose(cmdStream);
                        }
                    }
                    
                    std::map<std::string, std::string> decodedData = Microjson::Decode(jsonResult);
                    std::string userip = decodedData["query"];
                    std::string usercountry = decodedData["country"];
                    std::string usertime = decodedData["timezone"];
                    std::string userregion = decodedData["region"];
                    std::wstring hwid = GetHardDriveSerialNumber();
                    std::wcout << "HWID: " << hwid << std::endl;
                    std::string hwidString = ConvertWStringToString(hwid);
                    std::string infopacket = buildpacket(uuid, "info", hwidString+"=>"+std::to_string(gameid)+"=>"+userip+"=>"+usercountry+"=>"+usertime+"=>"+userregion, "json");
                    std::string inforeponse = sendMessage(infopacket);
                    std::cout << inforeponse;
                    MessageBox(NULL, L"By clicking ok you agree to the Shadow Clients terms and service", L"Authenticated", MB_OK);
                    std::string gamestring = std::to_string(gameid);
                    std::string launchgame = "start roblox-player:1+launchmode:play+gameinfo:"+verifycallback+"+launchtime:1693360810497+placelauncherurl:https%3A%2F%2Fassetgame.roblox.com%2Fgame%2FPlaceLauncher.ashx%3Frequest%3DRequestGame%26placeId%3D" + gamestring + "%26isPlayTogetherGame%3Dfalse%26joinAttemptOrigin%3DPlayButton+robloxLocale:en_us+gameLocale:en_us+channel:+LaunchExp:InApp";
                    system(launchgame.c_str());
                }
                else {
                    std::cout << verifycallback;
                    MessageBox(NULL, L"A Random Error Has Occured", L"Error", MB_OK);
                }
            }
            else {
                std::cout << "Authentication failed" << std::endl;
            }
        }
        else {
            std::cout << "Missing parameter." << std::endl;
        }
    }
    else {
        std::cout << "No parameters provided." << std::endl;
    }

    atexit(cleanup); 
    return 0;
}
