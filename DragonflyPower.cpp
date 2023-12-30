//
//  Pegasus pocket power box X2 plugin
//
//  Created by Rodolphe Pineau on 3/11/2020.


#include "DragonflyPower.h"


CDragonflyPower::CDragonflyPower()
{

    // set some sane values
    m_bIsConnected = false;

#ifdef PLUGIN_DEBUG
#if defined(WIN32)
    m_sLogfilePath = getenv("HOMEDRIVE");
    m_sLogfilePath += getenv("HOMEPATH");
    m_sLogfilePath += "\\DragonflyPower-Log.txt";
#else
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/DragonflyPower-Log.txt";
#endif
    m_sLogFile.open(m_sLogfilePath, std::ios::out |std::ios::trunc);
#endif

#if defined PLUGIN_DEBUG
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [CDragonflyPower] Version " << std::fixed << std::setprecision(2) << PLUGIN_VERSION << " build " << __DATE__ << " " << __TIME__ << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [CDragonflyPower] Constructor Called." << std::endl;
    m_sLogFile.flush();
#endif

}

CDragonflyPower::~CDragonflyPower()
{
#ifdef	PLUGIN_DEBUG
    // Close LogFile
    if(m_sLogFile.is_open())
        m_sLogFile.close();
#endif
}


int CDragonflyPower::deviceCommand(std::string sCmd, std::string &sResp, int nTimeout)
{
    int nErr = PLUGIN_OK;
    unsigned long  ulBytesWrite = 0;
#ifdef WIN32
    typedef int socklen_t;
    DWORD tvwin;
#endif

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [deviceCommand] sending : '" << sCmd << "' " << std::endl;
    m_sLogFile.flush();
#endif


#if defined SB_LINUX_BUILD || defined SB_MAC_BUILD
    // Mac and Unix use a timeval struct to set the timout
    // Set timeout to MAX_TIMEOUT (multiply by 1000 as MAX_TIMEOUT is in ms)
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = MAX_TIMEOUT*1000;

    nErr = setsockopt(m_iSockfd, SOL_SOCKET, SO_RCVTIMEO,(const char *) &tv,sizeof(tv));
    if (nErr) {
        return COMMAND_FAILED;
    }
    nErr = setsockopt(m_iSockfd, SOL_SOCKET, SO_SNDTIMEO,(const char *) &tv,sizeof(tv));
    if (nErr) {
        return COMMAND_FAILED;
    }
#endif

#ifdef WIN32
    // Timeout measured in milliseconds
    // Set timeout to MAX_TIMEOUT (MAX_TIMEOUT is in ms)
    tvwin = MAX_TIMEOUT;
    nErr = setsockopt(m_iSockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tvwin, sizeof(tvwin));
    if (nErr) {
        return COMMAND_FAILED;
    }
    nErr = setsockopt(m_iSockfd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tvwin, sizeof(tvwin));
    if (nErr) {
        return COMMAND_FAILED;
    }
#endif

    // Send command to the Dragonfly
    ulBytesWrite = sendto(m_iSockfd, sCmd.c_str(), sCmd.size(), 0,  (const sockaddr*) &m_Serveraddr,  m_nServerlen);
    if (ulBytesWrite < 0) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [deviceCommand] error sending command." << std::endl;
        m_sLogFile.flush();
#endif
        return ERR_TXTIMEOUT;
    }
    nErr = readResponse(sResp);
    if(nErr) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [deviceCommand] error reading response, nErr : '" << nErr << "' " << std::endl;
        m_sLogFile.flush();
#endif

        return nErr;
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [deviceCommand] response : '" << sResp << "' " << std::endl;
    m_sLogFile.flush();
#endif
    if(sResp.size()==0)
        return ERR_CMDFAILED;
    return nErr;

}

int CDragonflyPower::readResponse(std::string &sResp, int nTimeout, char cEndOfResponse)
{
    int nErr = PLUGIN_OK;
    long lBytesRead = 0;
    long lTotalBytesRead = 0;
    char szRespBuffer[BUFFER_SIZE];
    char *pszBufPtr;
    int nBufferLen = BUFFER_SIZE -1;
    sockaddr retserver;

#ifdef WIN32
    typedef int socklen_t;
#endif

    socklen_t lenretserver = sizeof(retserver);

    memset(szRespBuffer, 0, BUFFER_SIZE);
    sResp.clear();
    pszBufPtr = szRespBuffer;

    do {
        // Read response from socket
        lBytesRead = recvfrom(m_iSockfd, pszBufPtr, nBufferLen - lTotalBytesRead , 0, &retserver, &lenretserver);
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [readResponse] lBytesRead : " << lBytesRead << std::endl;
        m_sLogFile.flush();
#endif
        if(lBytesRead == -1) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 3
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [readResponse] socket read error." << std::endl;
            m_sLogFile.flush();
#endif
            return BAD_CMD_RESPONSE;
        }
        lTotalBytesRead += lBytesRead;
        pszBufPtr+=lBytesRead;
    } while (lTotalBytesRead < BUFFER_SIZE  && *(pszBufPtr-1) != cEndOfResponse);


#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [readResponse] response in szRespBuffer : '" << szRespBuffer << "' " << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [readResponse] lTotalBytesRead         : " << lTotalBytesRead << std::endl;
    m_sLogFile.flush();
#endif

    if(lTotalBytesRead>1) {
        *(pszBufPtr-1) = 0; //remove the end of response character
        sResp.assign(szRespBuffer);
    }
    return nErr;
}


int CDragonflyPower::Connect(std::string sIpAddress)
{
    int nErr = PLUGIN_OK;
    std::string sResp;
    struct hostent *server;
    struct linger l;
    int rc;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Called." << std::endl;
    m_sLogFile.flush();
#endif

    m_iSockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    server = gethostbyname(sIpAddress.c_str());

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] m_Sockfd : " << m_iSockfd << std::endl;
    m_sLogFile.flush();
#endif

    if (m_iSockfd < 0 || !server) {
        m_bIsConnected = false;
        return ERR_COMMOPENING;
    }

    l.l_onoff  = 0;
    l.l_linger = 0;
#ifdef WIN32
    rc = setsockopt(m_iSockfd, SOL_SOCKET, SO_LINGER, (char *)&l, sizeof(l));
#else
    rc = setsockopt(m_iSockfd, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
#endif

    memset(&m_Serveraddr, 0, sizeof(m_Serveraddr));

    m_Serveraddr.sin_family = AF_INET;
    memcpy(&m_Serveraddr.sin_addr.s_addr, server->h_addr, server->h_length);
    m_Serveraddr.sin_port = htons(10000); // default Dragonfly port
    m_nServerlen = sizeof(m_Serveraddr);

    m_bIsConnected = true;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] connected to " << sIpAddress << std::endl;
    m_sLogFile.flush();
#endif

    nErr = getFirmwareVersion(m_sVersion);
    if(nErr) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] no response from device at " << sIpAddress << std::endl;
        m_sLogFile.flush();
#endif
        m_bIsConnected = false;
        return ERR_NORESPONSE;
    }

    return nErr;
}


void CDragonflyPower::Disconnect()
{
    if(m_bIsConnected) {
#ifdef WIN32
        if (m_iSockfd != -1) closesocket(m_iSockfd);
#else
        if (m_iSockfd != -1) close(m_iSockfd);
#endif
        m_iSockfd = -1;
    }
    m_bIsConnected = false;
}

#pragma mark getters and setters

int CDragonflyPower::getFirmwareVersion(std::string &sVersion)
{
    int nErr = PLUGIN_OK;
    std::string sResp;
    std::vector<std::string> respFields;

    nErr = deviceCommand("!seletek version#", sResp); // pulse relay 1 for 1 second
    if(nErr) {
        return nErr;
    }
    nErr = parseFields(sResp, respFields, ':');
    if(nErr) {
        return nErr;
    }

    if(respFields.size()>1) {
        if(respFields[1].find("error")!=-1) {
            return ERR_CMDFAILED;
        }
        sVersion = respFields[1];
    }
    else {
        sVersion.clear();
        nErr = ERR_CMDFAILED;
    }

    return nErr;
}


bool CDragonflyPower::getPortStatus(const int nPortNumber)
{
    int nErr = PLUGIN_OK;
    std::string sResp;
    std::stringstream ssCmd;
    std::vector<std::string> vFields;
    int nPortstate;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getPortStatus] Getting port " << nPortNumber <<" state." << std::endl;
    m_sLogFile.flush();
#endif

    ssCmd << "!relio rldgrd 0 " << nPortNumber << "#";
    nErr = deviceCommand(ssCmd.str(), sResp);
    if(nErr)
        return nErr;

    nErr = parseFields(sResp, vFields, ':');
    if(nErr) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getPortStatus] Error parsing response : " << sResp << std::endl;
        m_sLogFile.flush();
#endif
        return nErr;
    }

    nPortstate=0;
    if(vFields.size()>1) {
        if(vFields[1].find("error")!=-1) {
            return ERR_CMDFAILED;
        }
        nPortstate = std::stoi(vFields[1]);
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getPortStatus] state : " << nPortstate << std::endl;
        m_sLogFile.flush();
#endif
    }

    return nPortstate==0?false:true;
}

int CDragonflyPower::setPort(const int nPortNumber, const bool bOn)
{
    int nErr = PLUGIN_OK;
    std::string sResp;
    std::stringstream ssCmd;
    std::vector<std::string> vFields;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setPort] setting port " << nPortNumber <<" state to " << (bOn?"On":"Off")<<"." << std::endl;
    m_sLogFile.flush();
#endif

    ssCmd << "!relio rlset 0 " << nPortNumber << " " << (bOn?1:0)<<"#";
    nErr = deviceCommand(ssCmd.str(), sResp);
    if(nErr && sResp.find("error")!=-1) {
        return ERR_CMDFAILED;
    }
    return PLUGIN_OK;
}

int CDragonflyPower::getPortCount()
{
    return NB_PORTS;
}

#pragma mark command and response functions
int CDragonflyPower::parseFields(const std::string sIn, std::vector<std::string> &svFields, char cSeparator)
{
    int nErr = PLUGIN_OK;
    std::string sSegment;
    std::stringstream ssTmp(sIn);

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [parseFields] Called." << std::endl;
    m_sLogFile.flush();
#endif
    if(sIn.size() == 0)
        return ERR_PARSE;

    svFields.clear();
    // split the string into vector elements
    while(std::getline(ssTmp, sSegment, cSeparator))
    {
        svFields.push_back(sSegment);
    }

    if(svFields.size()==0) {
        nErr = ERR_PARSE;
    }
    return nErr;
}

std::string& CDragonflyPower::trim(std::string &str, const std::string& filter )
{
    return ltrim(rtrim(str, filter), filter);
}

std::string& CDragonflyPower::ltrim(std::string& str, const std::string& filter)
{
    str.erase(0, str.find_first_not_of(filter));
    return str;
}

std::string& CDragonflyPower::rtrim(std::string& str, const std::string& filter)
{
    str.erase(str.find_last_not_of(filter) + 1);
    return str;
}

std::string CDragonflyPower::findField(std::vector<std::string> &svFields, const std::string& token)
{
    for(int i=0; i<svFields.size(); i++){
        if(svFields[i].find(token)!= -1) {
            return svFields[i];
        }
    }
    return std::string();
}


#ifdef PLUGIN_DEBUG
const std::string CDragonflyPower::getTimeStamp()
{
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    std::strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}
#endif
