#include "x2powercontrol.h"

X2PowerControl::X2PowerControl(const char* pszDisplayName,
										const int& nInstanceIndex,
										SerXInterface						* pSerXIn,
										TheSkyXFacadeForDriversInterface	* pTheSkyXIn,
										SleeperInterface					* pSleeperIn,
										BasicIniUtilInterface				* pIniUtilIn,
										LoggerInterface						* pLoggerIn,
										MutexInterface						* pIOMutexIn,
										TickCountInterface					* pTickCountIn):m_bLinked(0)
{
    char portName[255];
    char szIpAddress[255];
	std::string sLabel;
    int i;

	m_pTheSkyXForMounts = pTheSkyXIn;
	m_pIniUtil = pIniUtilIn;
	m_pIOMutex = pIOMutexIn;
	m_pTickCount = pTickCountIn;

	m_nISIndex = nInstanceIndex;
    
    if (m_pIniUtil) {
		// load port names
		for(i=0; i<NB_PORTS; i++) {
            sLabel = "Relay " + std::to_string(i+1);
            m_pIniUtil->readString(PARENT_KEY, m_IniPortKey[i].c_str(), sLabel.c_str(), portName, 255);
            m_sPortNames.push_back(std::string(portName));

            m_pIniUtil->readString(PARENT_KEY, CHILD_KEY_IP, "192.168.1.123", szIpAddress, 255);
            m_sIpAddress.assign(szIpAddress);

            m_bRelay1Used = (m_pIniUtil->readInt(PARENT_KEY, CHILD_KEY_RELAY1_USED, 0) == 0?false:true);

        }

	}
}

X2PowerControl::~X2PowerControl()
{
	//Delete objects used through composition
	if (GetTheSkyXFacadeForDrivers())
		delete GetTheSkyXFacadeForDrivers();
	if (GetSleeper())
		delete GetSleeper();
	if (GetSimpleIniUtil())
		delete GetSimpleIniUtil();
	if (GetMutex())
		delete GetMutex();
}

int X2PowerControl::establishLink(void)
{
	int nErr = SB_OK;

    nErr = m_PowerPorts.Connect(m_sIpAddress);
    if(nErr)
        m_bLinked = false;
    else
        m_bLinked = true;

	return nErr;
}

int X2PowerControl::terminateLink(void)
{
	m_bLinked = false;
    m_PowerPorts.Disconnect();
    return SB_OK;
}

bool X2PowerControl::isLinked() const
{
	return m_bLinked;
}


void X2PowerControl::deviceInfoNameShort(BasicStringInterface& str) const
{
    str = "Dragonfly controller";
}
void X2PowerControl::deviceInfoNameLong(BasicStringInterface& str) const
{
    str = "Dragonfly controller";
}
void X2PowerControl::deviceInfoDetailedDescription(BasicStringInterface& str) const
{
    str = "Lunatico Astonomy Dragonfly controller";
}
void X2PowerControl::deviceInfoFirmwareVersion(BasicStringInterface& str)
{
    if(m_bLinked) {
        std::string sFirmware;
        X2MutexLocker ml(GetMutex());
        m_PowerPorts.getFirmwareVersion(sFirmware);
        str = sFirmware.c_str();
    }
    else
        str = "N/A";
}
void X2PowerControl::deviceInfoModel(BasicStringInterface& str)
{
    str = "Dragonfly controller";;
}

void X2PowerControl::driverInfoDetailedInfo(BasicStringInterface& str) const
{
	str = "Lunatico Astonomy Dragonfly controller X2 plugin by Rodolphe Pineau";
}

double X2PowerControl::driverInfoVersion(void) const
{
	return PLUGIN_VERSION;
}

int X2PowerControl::queryAbstraction(const char* pszName, void** ppVal)
{
	*ppVal = NULL;

    if (!strcmp(pszName, ModalSettingsDialogInterface_Name))
        *ppVal = dynamic_cast<ModalSettingsDialogInterface*>(this);
    else if (!strcmp(pszName, X2GUIEventInterface_Name))
        *ppVal = dynamic_cast<X2GUIEventInterface*>(this);
    else if (!strcmp(pszName, CircuitLabelsInterface_Name))
        *ppVal = dynamic_cast<CircuitLabelsInterface*>(this);
    else if (!strcmp(pszName, SetCircuitLabelsInterface_Name))
        *ppVal = dynamic_cast<SetCircuitLabelsInterface*>(this);

	return 0;
}

#pragma mark - UI binding

int X2PowerControl::execModalSettingsDialog()
{
    int nErr = SB_OK;
    X2ModalUIUtil uiutil(this, GetTheSkyXFacadeForDrivers());
    X2GUIInterface*                    ui = uiutil.X2UI();
    X2GUIExchangeInterface*            dx = NULL;//Comes after ui is loaded
    bool bPressedOK = false;
    char szIpAddress[255];

    if (NULL == ui)
        return ERR_POINTER;

    nErr = ui->loadUserInterface("DragonflyPower.ui", deviceType(), m_nISIndex);
    if (nErr)
        return nErr;

    if (NULL == (dx = uiutil.X2DX()))
        return ERR_POINTER;

    X2MutexLocker ml(GetMutex());

    if(m_bLinked) {
        dx->setEnabled("IPAddress",false); // can't modify ip address when connected
    }
    else {
        dx->setEnabled("IPAddress",true);
    }
    dx->setText("IPAddress", m_sIpAddress.c_str());
    dx->setChecked("relay1RoR", (m_bRelay1Used==true?1:0));

    //Display the user interface
    if ((nErr = ui->exec(bPressedOK)))
        return nErr;

    //Retreive values from the user interface
    if (bPressedOK) {
        m_bRelay1Used = (dx->isChecked("relay1RoR")!=0);
        if(!m_bLinked) {
            dx->propertyString("IPAddress", "text", szIpAddress, 255);
            m_sIpAddress.assign(szIpAddress);
            m_pIniUtil->writeString(PARENT_KEY, CHILD_KEY_IP, szIpAddress);
        }
        m_pIniUtil->writeInt(PARENT_KEY, CHILD_KEY_RELAY1_USED, m_bRelay1Used?1:0);
    }
    return nErr;

}

void X2PowerControl::uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent)
{

    if (!strcmp(pszEvent, "on_timer")) {
    }
}

int X2PowerControl::numberOfCircuits(int& nNumber)
{
	nNumber = m_PowerPorts.getPortCount();
	return 0;
}

int X2PowerControl::circuitState(const int& nIndex, bool& bZeroForOffOneForOn)
{
	int nErr = SB_OK;

	if(!m_bLinked)
        return ERR_NOLINK;

    if (nIndex >= 0 && nIndex<m_PowerPorts.getPortCount())
        bZeroForOffOneForOn = m_PowerPorts.getPortStatus(nIndex);
	else
		nErr = ERR_INDEX_OUT_OF_RANGE;

    return nErr;
}

int X2PowerControl::setCircuitState(const int& nIndex, const bool& bZeroForOffOneForOn)
{
	int nErr = SB_OK;

	if(!m_bLinked)
        return ERR_NOLINK;

    if (nIndex >= 0 && nIndex < m_PowerPorts.getPortCount()) {
        if(nIndex == 0 && m_bRelay1Used) {
            return ERR_INDEX_OUT_OF_RANGE;
        }
        nErr = m_PowerPorts.setPort(nIndex, bZeroForOffOneForOn);
    }
	else
		nErr = ERR_INDEX_OUT_OF_RANGE;
	return nErr;
}

int X2PowerControl::circuitLabel(const int &nZeroBasedIndex, BasicStringInterface &str)
{
    int nErr = SB_OK;
    std::string sLabel;
    if(m_sPortNames.size() >= nZeroBasedIndex+1) {
        if(m_bRelay1Used && nZeroBasedIndex==0) {
            str = "Reserved for RoR";
        }
        else
            str = m_sPortNames[nZeroBasedIndex].c_str();
    }
    else {
        if(m_bRelay1Used && nZeroBasedIndex==0) {
            sLabel = "Reserved for RoR";
        }
        else {
            sLabel = "Relay " + std::to_string(nZeroBasedIndex+1);
        }
        str = sLabel.c_str();
    }

    return nErr;
}

int X2PowerControl::setCircuitLabel(const int &nZeroBasedIndex, const char *str)
{
    int nErr = SB_OK;

    if(m_sPortNames.size() >= nZeroBasedIndex+1) {
        m_sPortNames[nZeroBasedIndex] = str;
        m_pIniUtil->writeString(PARENT_KEY, m_IniPortKey[nZeroBasedIndex].c_str(), str);
    }
    else {
        nErr = ERR_CMDFAILED;
    }
    return nErr;
}
