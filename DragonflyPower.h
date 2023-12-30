//
//
//  Created by Rodolphe Pineau on 3/11/2020.


#ifndef __DragonflyPower_C__
#define __DragonflyPower_C__
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <memory.h>

#ifdef SB_WIN_BUILD
#include <time.h>
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#endif

// C++ includes
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <thread>
#include <ctime>


#include "../../licensedinterfaces/sberrorx.h"
#include "../../licensedinterfaces/serxinterface.h"

#define PLUGIN_VERSION  1.0

// #define PLUGIN_DEBUG 3

#define BUFFER_SIZE 4096
#define MAX_TIMEOUT 500

enum DragonflyErrors {PLUGIN_OK=0, NOT_CONNECTED, CANT_CONNECT, BAD_CMD_RESPONSE, COMMAND_FAILED, NO_DATA_TIMEOUT, ERR_PARSE};


#define PLUGIN_VERSION  1.0

#define NB_PORTS 8


class CDragonflyPower
{
public:
    CDragonflyPower();
    ~CDragonflyPower();

    int         Connect(std::string sIpAddress);
    void        Disconnect(void);
    bool        IsConnected(void) { return m_bIsConnected; };

    int         getState();
    int         getFirmwareVersion(std::string &sVersion);

    // getter and setter
    bool        getPortStatus(const int nPortNumber);
    int         setPort(const int nPortNumber, const bool bON);
    int         getPortCount();
    
protected:
    bool            m_bIsConnected;
    std::string     m_sVersion;


    int             readResponse(std::string &sResp, int nTimeout = MAX_TIMEOUT, char cEndOfResponse = '#');
    int             deviceCommand(std::string sCmd, std::string &sResp, int nTimeout = MAX_TIMEOUT);
    int             parseFields(const std::string sIn, std::vector<std::string> &svFields, char cSeparator);

    std::string&    trim(std::string &str, const std::string &filter );
    std::string&    ltrim(std::string &str, const std::string &filter);
    std::string&    rtrim(std::string &str, const std::string &filter);
    std::string     findField(std::vector<std::string> &svFields, const std::string& token);

    // network
#ifdef WIN32
    WSADATA m_WSAData;
    SOCKET m_iSockfd;
#else
    int m_iSockfd;
#endif
    int m_nServerlen;  // Length of server address
    struct sockaddr_in m_Serveraddr; // Struct for server address
    //#endif

#ifdef PLUGIN_DEBUG
    // timestamp for logs
    const std::string getTimeStamp();
    std::ofstream m_sLogFile;
    std::string m_sLogfilePath;
#endif


};

#endif //__DragonflyPower_C__
