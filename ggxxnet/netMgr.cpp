/* for Visual Studio 8.0 */
#ifdef _MSC_VER
	#if (_MSC_VER >= 1400)
		#define POINTER_64 __ptr64
	#endif
#endif

//******************************************************************
// include
//******************************************************************
#include "netMgr.h"
#include "node.h"
#include "ggxxnet.h"
#include "denyList.h"
#include "zlib.h"
#include "util.h"

//******************************************************************
// macro
//******************************************************************
#define WSERR(api)\
{\
	char str[256];\
	int code = WSAGetLastError();\
	if (code == WSAEINTR)				sprintf(str, "%s = WSAEINTR\n", api);\
	else if (code == WSAEINVAL)			sprintf(str, "%s = WSAEINVAL\n", api);\
	else if (code == WSAEWOULDBLOCK)	sprintf(str, "%s = WSAEWOULDBLOCK\n", api);\
	else if (code == WSAETIMEDOUT)		sprintf(str, "%s = WSAETIMEDOUT\n", api);\
	else if (code == WSAECONNREFUSED)	sprintf(str, "%s = WSAECONNREFUSED\n", api);\
	else if (code == WSANO_DATA)		sprintf(str, "%s = WSANO_DATA\n", api);\
	else if (code == WSAECONNRESET)		sprintf(str, "%s = WSAECONNRESET\n", api);\
	else if (code == WSAENOTSOCK)		sprintf(str, "%s = WSAENOTSOCK\n", api);\
	else								sprintf(str, "%s = %08x\n", api, code);\
	DBGOUT_LOG(str);\
}

//******************************************************************
// global
//******************************************************************
CNetMgr* g_netMgr = NULL;

CNetMgr::CNetMgr(void)
{
	m_networkEnable		= false;
	m_quitApp			= false;
	m_recvThread_end	= true;
	m_lobbyThread_end	= true;
	m_recvThread		= NULL;
	m_lobbyThread		= NULL;

	m_udpSocket	= INVALID_SOCKET;
	m_tcpSocket	= INVALID_SOCKET;
	m_connect	= false;
	m_watch		= false;
	m_delay		= 0;
	m_queueSize	= 0;
	m_key		= NULL;
	m_syncChk	= NULL;
	m_playSide	= 1;
	m_time		= 0;
	m_suspend	= true;
	m_totalSlow	= 0;

	m_waitingConnectReply	= false;
	m_waitingSuspendReply	= false;
	m_waitingResumeReply	= false;
	m_waitingData			= false;
	m_waitingDataReply		= false;
	m_waitingWatchInReply	= false;
	m_waitingWatchInRootReply = EWIRReply_Idle;
	m_waitingBattleInfoRequestReply = false;

	m_waitingDataType		= -1;
	
	m_recvDataPtr	= NULL;
	m_recvDataSize	= 0;

	m_sendDataSeq = 0;

	setErrMsg("");

	m_remoteAddr_active = NULL_ADDR;
	m_remoteAddr_recv = NULL_ADDR;

	initWatchVars();

	InitializeCriticalSection(&m_csKey);
	InitializeCriticalSection(&m_csNode);
	InitializeCriticalSection(&m_csWatch);
}

CNetMgr::~CNetMgr(void)
{
	stopThread();

	if (m_udpSocket != INVALID_SOCKET) closesocket(m_udpSocket);
	if (m_tcpSocket != INVALID_SOCKET) closesocket(m_tcpSocket);
	WSACleanup();
	
	delete[] m_key;
	delete[] m_syncChk;
	delete[] m_watchRecvCompData;

	DeleteCriticalSection(&m_csKey);
	DeleteCriticalSection(&m_csNode);
	DeleteCriticalSection(&m_csWatch);
}

bool CNetMgr::init(int p_port, int p_delay, bool p_useLobby)
{
	try
	{
		m_delay		= p_delay;
		m_queueSize	= m_delay * 2;
		m_key		= new DWORD[m_queueSize];
		m_syncChk	= new WORD[m_queueSize];

		m_watchRecvCompData = new char[MAX_COMPWATCHDATASIZE];

		WSADATA wsadata;
		int err = WSAStartup(MAKEWORD(2, 0), &wsadata);
		if (err != 0)
		{
			DBGOUT_LOG("err WSAStartup\n");
			return false;
		}
		
		DBGOUT_LOG("WSAStartup ok\n");

		/* ���A�h���X */
		sockaddr_in	addr;
		addr = NULL_ADDR;
		addr.sin_port		= htons(p_port);
		addr.sin_family		= AF_INET;
		addr.sin_addr.s_addr= INADDR_ANY;

		// UDP Socket
		m_udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
		if (m_udpSocket == INVALID_SOCKET) throw "socket";

		DBGOUT_LOG("udp socket() ok\n");

		if (bind(m_udpSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) throw "bind";
		
		DBGOUT_LOG("udp bind() ok\n");

		/* UDP�̓m���u���b�L���O */
		unsigned long val = 1;
		if (ioctlsocket(m_udpSocket, FIONBIO, &val) == SOCKET_ERROR) throw "ioctlsocket";
		
		DBGOUT_LOG("udp ioctlsocket(FIONBIO) ok\n");

#if USE_TCP
		// TCP Socket
		m_tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (m_tcpSocket == INVALID_SOCKET) throw "socket";

		DBGOUT_LOG("tcp socket() ok\n");

		char option = 1;
		if (setsockopt(m_tcpSocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == SOCKET_ERROR)
		{
			throw "tcp setsockopt(SO_REUSEADDR) err";
		}
		
		DBGOUT_LOG("tcp setsockopt(SO_REUSEADDR) ok\n");

		if (bind(m_tcpSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) throw "bind";

		DBGOUT_LOG("tcp bind() ok\n");

		if (listen(m_tcpSocket, 1) == SOCKET_ERROR) throw "listen";

		DBGOUT_LOG("tcp listen() ok\n");
#endif

		DWORD tid;
		/* ��M�p�X���b�h���쐬 */
		m_recvThread = CreateThread(NULL, 0, _recvThreadProc, this, CREATE_SUSPENDED, &tid);

		//  1 = THREAD_PRIORITY_ABOVE_NORMAL
		//  0 = THREAD_PRIORITY_NORMAL
		// -1 = THREAD_PRIORITY_BELOW_NORMAL
		SetThreadPriority(m_recvThread, g_iniFileInfo.m_recvThreadPriority);

		if (m_recvThread == NULL) throw "err CreateThread _lobbyThreadProc\n";
		m_recvThread_end	= false;

		DBGOUT_LOG("recv thread ok\n");

		/* ���r�[�X���b�h���쐬 */
		if (p_useLobby)
		{
			m_lobbyThread = CreateThread(NULL, 0, _lobbyThreadProc, this, 0, &tid);
			//SetThreadPriority(m_lobbyThread, THREAD_PRIORITY_BELOW_NORMAL);
			if (m_lobbyThread == NULL) throw "err CreateThread _lobbyThreadProc\n";
			m_lobbyThread_end	= false;
		}

		DBGOUT_LOG("lobby thread ok\n");

		return true;
	}
	catch(char* msg)
	{
		if (WSAGetLastError() != 0)
		{
			WSERR(msg);
		}
		else
		{
			DBGOUT_LOG(msg);
		}
		if (m_udpSocket != INVALID_SOCKET) closesocket(m_udpSocket);
		if (m_tcpSocket != INVALID_SOCKET) closesocket(m_tcpSocket);
		WSACleanup();

		return false;
	}
}

void CNetMgr::initWatchVars(void)
{
	m_watch = false;
	m_1stCaster = false;
	m_watchRootAddr[0] = NULL_ADDR;
	m_watchRootAddr[1] = NULL_ADDR;
	__strncpy(m_watchRootName[0], "", 29);
	__strncpy(m_watchRootName[1], "", 29);
	m_watchRootGameCount[0] = 0;
	m_watchRootGameCount[1] = 0;
	m_watchRecvComplete = false;
	m_watchRecvSize = 0;

	for (int i = 0; i < WATCH_MAX_CHILD; i++) m_watcher[i].init();
	m_recvGalleryCount = 0;
	m_totalGalleryCount = 0;
	m_watchFailCount = 0;

	m_watchRecvCompSize = 0;

	DBGOUT_NET("---------------------initWatchVars\n");
}

void CNetMgr::startThread(void)
{
	ResumeThread(m_recvThread);
	ResumeThread(m_lobbyThread);
}

void CNetMgr::stopThread(void)
{
	m_quitApp = true;
	/* �X���b�h���I������܂őҋ@ */
	for (int i = 0; i < 100; i++)
	{
		if (m_recvThread_end && m_lobbyThread_end) break;
		Sleep(20); /* 2�b��timeout */
	}

	if (m_recvThread) CloseHandle(m_recvThread);
	if (m_lobbyThread) CloseHandle(m_lobbyThread);
}

void CNetMgr::connect(void)
{
	/* �����E����̓��͂��N���A */
	for (int i = 0; i < m_queueSize; i++)
	{
		m_key[i] = 0x00000000;
	}
	/* �������N���A */
	for (int i = 0; i < m_queueSize; i++) m_syncChk[i] = 0;
	/* ����̏��N���A */
	g_enemyInfo.clear();

	m_connect = true;
	m_watch   = false;
	m_1stCaster = true;
	m_suspend = true;
	m_suspendFrame = 0;
	m_vsloadFrame = -1;	/* -1��vsload���łȂ����Ƃ�\�� */
	m_recvSuspend = true;
	m_recvDataSeq = -1;  

	m_enMaxPacketSize = 256;

	DBGOUT_NET("connect\n");
}

void CNetMgr::disconnect(char* p_cause)
{
	if (strcmp(p_cause, "SYNC ERROR!!") == 0)
	{
		g_replay.m_data.inputData[g_replay.m_frameCount++] = INPUT_SYNCERROR;
	}
	else if (strcmp(p_cause, "endbattle") == 0)
	{
		g_replay.m_data.inputData[g_replay.m_frameCount++] = INPUT_COMPLETE;
	}
	else
	{
		g_replay.m_data.inputData[g_replay.m_frameCount++] = INPUT_DISCONNECT;
	}

	// �Ō�ɗ]�T�������đ��鎞�Ԃ�^����
	ENTERCS(&g_netMgr->m_csWatch);
	for (int i = 0; i < 3; i++)
	{
		for (int i = 0; i < g_setting.watchMaxNodes + (m_watch ? WATCH_MAX_CHILD_INC : 0); i++)
		{
			if (g_netMgr->m_watcher[i].isActive())
			{
				// ���M���ォ��P�b�҂��Ă��ԓ����Ȃ�
				if (g_netMgr->m_watcher[i].m_sendTime != 0xffffffff &&
					timeGetTime() - g_netMgr->m_watcher[i].m_sendTime > TIMEOUT_WATCHDATAREPLY)
				{
					g_netMgr->m_watcher[i].init();
				}
				else
				{
					g_netMgr->send_watchData(i);
				}
			}
		}
		Sleep(10);
	}
	LEAVECS(&g_netMgr->m_csWatch);

#if !TESTER
	// ���v���C�̕ۑ��Aggxx�ϐ��̏��������Ȃ�
	onDisconnect(p_cause);
#endif

	if (m_connect)
	{
		m_connect = false;
		DBGOUT_NET("************************* disconnect %s *************************\n", p_cause);
		
		ENTERCS(&g_netMgr->m_csNode);

		char addrstr[32];
		int idx = g_nodeMgr->findNodeIdx_address(getStringFromAddr(&m_remoteAddr_active, addrstr));
		if (idx != -1)
		{
			char* id = g_nodeMgr->getNode(idx)->m_id;

			DBGOUT_LOG("disconnect (%s) %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\t#%s\n", p_cause,
				(BYTE)id[0], (BYTE)id[1], (BYTE)id[2], (BYTE)id[3], (BYTE)id[4], (BYTE)id[5],
				(BYTE)id[6], (BYTE)id[7], (BYTE)id[8], (BYTE)id[9], g_nodeMgr->getNode(idx)->m_name);
		}
		LEAVECS(&g_netMgr->m_csNode);
	}

	if (m_watch)
	{
		DBGOUT_NET("************************* disconnect (watch) %s *************************\n", p_cause);
		
		ENTERCS(&g_netMgr->m_csNode);
		for (int i = 0; i < 2; i++)
		{
			char addrstr[32];
			int idx = g_nodeMgr->findNodeIdx_address(getStringFromAddr(&m_watchRootAddr[i], addrstr));
			if (idx != -1)
			{
				char* id = g_nodeMgr->getNode(idx)->m_id;

				DBGOUT_LOG("disconnect watch P%d : %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\t#%s\n", i + 1,
					(BYTE)id[0], (BYTE)id[1], (BYTE)id[2], (BYTE)id[3], (BYTE)id[4], (BYTE)id[5],
					(BYTE)id[6], (BYTE)id[7], (BYTE)id[8], (BYTE)id[9], g_nodeMgr->getNode(idx)->m_name);
			}
		}
		LEAVECS(&g_netMgr->m_csNode);

		g_replay.m_playing = false;
	}

	initWatchVars();

	m_remoteAddr_active = NULL_ADDR;

	m_lobbyFrame = -1;
}

void CNetMgr::resume(void)
{
	if (m_connect && send_resume() == false)
	{
		disconnect("resume");
		return;
	}

	m_suspend = false;
	m_time = 0;
	m_totalSlow = 0;
	
	/* ����̓��͂��N���A */
	for (int i = 0; i < m_queueSize; i++)
	{
		//m_key[i] |= (m_playSide == 1) ? 0xffff0000 : 0x0000ffff;
		m_key[i] = 0;
	}
	/* �������N���A */
	for (int i = 0; i < m_queueSize; i++) m_syncChk[i] = 0;

	DBGOUT_NET("resume\n");
}

void CNetMgr::suspend(void)
{
	if (m_connect && send_suspend() == false)
	{
		disconnect("suspend");
		return;
	}

	m_suspend = true;
	m_suspendFrame = 0;
	DBGOUT_NET("suspend\n");
}

void CNetMgr::setErrMsg(char* p_msg)
{
	if (p_msg[0] == '\0' || m_errMsg[0] == '\0')
	{
		m_errMsgTime = 0;
		strcpy(m_errMsg, p_msg);
	}
}

char* CNetMgr::getStringFromAddr(sockaddr_in* p_addr, char* p_output)
{
	/* sockaddr_in���當����(ip1.ip2.ip3.ip4:port)�̌`���ɕϊ����� */
	DWORD	ip = p_addr->sin_addr.S_un.S_addr;
	WORD	port = p_addr->sin_port;
	port = ((port << 8) & 0xFF00) | ((port >> 8) & 0xFF);
	sprintf(p_output, "%d.%d.%d.%d:%d", ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF, port);

	return p_output;
}

sockaddr_in CNetMgr::getAddrFromString(char* p_str)
{
	/* ������(ip1.ip2.ip3.ip4:port)����sockaddr_in�̌`���ɕϊ����� */
	sockaddr_in addr;

	int		port;
	char	ip[32];
	CNodeMgr::getNodeInfoFromString(p_str, ip, &port);

	addr = NULL_ADDR;
	addr.sin_port			= htons(port);
	addr.sin_family			= AF_INET;
	addr.sin_addr.s_addr	= inet_addr(ip);
	return addr;
}

bool CNetMgr::watch(char* p_targetName, sockaddr_in* p_targetAddr, int p_targetGameCount, bool /*p_blockingMode*/)
{
	// watchIn���ɍs���̂ŁA�ǂ��炩�̃��[�g�m�[�h�ɏW�������\���������B
	// �K�w���d�˂邱�Ƃɂ��x����100ms���|����Ȃ����ߓ��ɖ��͂Ȃ��B
	// watch in root�͈����Ŏw�肳�ꂽ�m�[�h�ɒ��ڊϐ�v���𓊂���B
	// �󂯓����f��ꂽ�ꍇ�A����̃A�h���X�������Ă���̂ŁA
	// ������ɍēxwatch in root�𓊂���B
	// �y���O�����z
	// �E�|�[�g�O�Ńc���[�����ߐs�������ƍ��������ƂɂȂ�
	// �E�ł���΃c���[���ϓ��ɍ\�z�ł���Ε���Ȃ��̂����c

	// �e�̃A�h���X�A���k�f�[�^�͍Đڑ��̍ۂɔj�������
	m_watchParentAddr = NULL_ADDR;
	m_watchRecvCompSize = 0;

	// watch in
	bool success = false;
	bool result = send_watchIn(p_targetName, p_targetAddr, p_targetGameCount);
	if (result) { DBGOUT_NET("watchin succeed!!\n"); return true; }
	DBGOUT_NET("watchin failed!!\n");

	/* �ϐ�҂����Ȃ��悤�Ȃ̂Œ��ڔz�M���ւȂ� */

	// watch in root[0]
	//m_remoteAddr_active = *p_targetAddr;
	result = send_watchInRoot(p_targetAddr, p_targetGameCount, success);
	if (result && success) { DBGOUT_NET("watchin root[0] succeed!!\n"); return true; }
	if (result == false)   { DBGOUT_NET("watchin root[0] failed!!\n"); goto FAILED; }

	/* ����I�[�o�[�Ȃ�󂯎�����ΐ푊��̃A�h���X�ɍđ����� */

	// watch in root[1]
	//m_remoteAddr_active = m_watchRootAddr[1];
	result = send_watchInRoot(&m_watchRootAddr[1], m_watchRootGameCount[1], success);
	if (result && success) { DBGOUT_NET("watchin root[1] succeed!!\n"); return true; }
	if (result == false)   { DBGOUT_NET("watchin root[1] failed!!\n"); goto FAILED; }

FAILED:
	// �Đڑ��Ɏ��s������watch���[�h���~
	DBGOUT_NET("failed to request watch\n");
	return false;
}

int CNetMgr::findFreeWatchEntry(sockaddr_in* p_addr)
{
	// ���ɂ���Ȃ炻���Ԃ�
	for (int i = 0; i < g_setting.watchMaxNodes + (m_watch ? WATCH_MAX_CHILD_INC : 0); i++)
	{
		if (m_watcher[i].m_remoteAddr.sin_addr.S_un.S_addr == p_addr->sin_addr.S_un.S_addr &&
			m_watcher[i].m_remoteAddr.sin_port == p_addr->sin_port) return i;
	}
	// ���Ȃ���΋󔒂�T��
	for (int i = 0; i < g_setting.watchMaxNodes + (m_watch ? WATCH_MAX_CHILD_INC : 0); i++)
	{
		if (m_watcher[i].isActive() == false) return i;
	}
	return -1; // �󖳂�
}

int CNetMgr::getChildWatcherCount(void)
{
	int count = 0;
	for (int i = 0; i < WATCH_MAX_CHILD; i++)
	{
		if (m_watcher[i].isActive()) count += m_watcher[i].m_childCount;
	}
	return count;
}

bool CNetMgr::send_connect(sockaddr_in* p_addr)
{
	SPacket_Connect	data;
	data.packetType		= Packet_Connect;
	data.cid			= CONNECTION_ID;
	data.maxPacketSize	= g_iniFileInfo.m_maxPacketSize;
	udpsend(p_addr, (char*)&data, sizeof(data));

	m_waitingConnectReply = true;
	for (int i = 0; i * 10 < TIMEOUT_REPLY; i++)
	{
		Sleep(10);
		if (!m_waitingConnectReply)
		{
			DBGOUT_NET("send_connect success\n");
			return true;
		}
		/* �đ� */
		if (i % 5 == 0) udpsend(p_addr, (char*)&data, sizeof(data));
	}
	DBGOUT_NET("send_connect timeout\n");
	m_waitingConnectReply = false;
	return false;
}

void CNetMgr::send_connectReply(void)
{
	SPacket_Connect	data;
	data.packetType		= Packet_ConnectReply;
	data.cid			= CONNECTION_ID;
	data.maxPacketSize	= g_iniFileInfo.m_maxPacketSize;
	udpsend(&m_remoteAddr_recv, (char*)&data, sizeof(data));
}

void CNetMgr::send_key(int p_time)
{
	SPacket_Key data;
	data.packetType = Packet_Key;
	data.time		= p_time;
	
	for (int i = 0; i < m_queueSize; i++)
	{
		if (m_playSide == 1)
		{
			data.cell[i].key = (WORD)m_key[i];
			data.cell[i].syncChk = (BYTE)m_syncChk[i];
		}
		else if (m_playSide == 2)
		{
			data.cell[i].key = (WORD)(m_key[i] >> 16);
			data.cell[i].syncChk = (BYTE)(m_syncChk[i] >> 8);
		}
	}

	udpsend(&m_remoteAddr_active, (char*)&data, 5 + m_queueSize * 3);
}

bool CNetMgr::send_watchInRoot(sockaddr_in* p_addr, int p_targetGameCount, bool& p_success)
{
	SPacket_WatchInRoot	data;
	data.packetType		= Packet_WatchInRoot;
	data.cid			= CONNECTION_ID;
	data.dataOffset		= m_watchRecvSize;
	data.targetGameCount= p_targetGameCount;
	data.format			= 1;	// ���k�T�|�[�g
	data.maxPacketSize	= g_iniFileInfo.m_maxPacketSize;

	m_waitingWatchInRootReply = EWIRReply_Wait;
	for (int i = 0; i < 5; i++)
	{	
		/* �đ� */
		udpsend(p_addr, (char*)&data, sizeof(data));
		Sleep(10);
		if (m_waitingWatchInRootReply == EWIRReply_Success ||
			m_waitingWatchInRootReply == EWIRReply_Fail)
		{
			p_success = m_waitingWatchInRootReply == EWIRReply_Success;
			DBGOUT_NET("send_watchinroot %s\n", p_success ? "success" : "fail");
			return true;
		}
	}
	DBGOUT_NET("send_watchinroot timeout\n");
	m_waitingWatchInRootReply = EWIRReply_Idle;
	p_success = false;
	return false;
}

void CNetMgr::send_watchInRootReply(bool p_accept)
{
	SPacket_WatchInRootReply	data;
	data.packetType	= Packet_WatchInRootReply;
	data.cid		= CONNECTION_ID;
	if (p_accept)
	{
		// format1���g�p
		data.accept = 1;
		getNameTrip(data.extra.format1.myName);
		__strncpy(data.extra.format1.enemyName, g_enemyInfo.m_name, 29);
		data.extra.format1.myGameCount = g_setting.totalBattle;
		data.extra.format1.enemyGameCount = g_enemyInfo.m_gameCount;
		memcpy(&data.enemyAddr, &m_remoteAddr_active, sizeof(m_remoteAddr_active));
		udpsend(&m_remoteAddr_recv, (char*)&data, sizeof(data));
	}
	else
	{
		// format2���g�p
		// �����͂Ȃ��Ȃ��̂ő���̃A�h���X�𑗐M����
		// �ϐ�v���҂͑���ɐڑ������݂�
		data.accept = 0;
		data.extra.format2.myGameCount = g_setting.totalBattle;
		data.extra.format2.enemyGameCount = g_enemyInfo.m_gameCount;
		memcpy(&data.enemyAddr, &m_remoteAddr_active, sizeof(m_remoteAddr_active));
		udpsend(&m_remoteAddr_recv, (char*)&data, SPacket_WatchInRootReply::SIZE120_2_FMT2);
	}
}

bool CNetMgr::send_watchIn(char* p_targetName, sockaddr_in* p_targetIP, int p_targetGameCount)
{
	SPacket_WatchIn	data;
	data.packetType		= Packet_WatchIn;
	data.cid			= CONNECTION_ID;
	data.targetIP		= p_targetIP->sin_addr;
	__strncpy(data.targetName, p_targetName, 29);
	data.targetGameCount= p_targetGameCount;
	data.dataOffset		= m_watchRecvSize;
	data.format			= 1;	// ���k�T�|�[�g
	data.maxPacketSize	= g_iniFileInfo.m_maxPacketSize;

	m_waitingWatchInReply = true;
	for (int i = 0; i < 5; i++)
	{	
		/* �đ� */
		for (int j = 0; j < g_nodeMgr->getNodeCount(); j++)
		{
			CNode* node = g_nodeMgr->getNode(j);
			if (node->m_state == State_Watch || node->m_state == State_Watch_Playable)
			{
				sockaddr_in addr = g_netMgr->getAddrFromString(node->m_addr);
				udpsend(&addr, (char*)&data, sizeof(data));
			}
		}
		Sleep(10);
		if (!m_waitingWatchInReply)
		{
			DBGOUT_NET("send_watchin success\n");
			return true;
		}
	}
	DBGOUT_NET("send_watchin timeout\n");
	m_waitingWatchInReply = false;
	return false;
}

void CNetMgr::send_watchInReply(void)
{
	SPacket_WatchInReply	data;
	data.packetType	= Packet_WatchInReply;
	data.cid		= CONNECTION_ID;
	__strncpy(data.rootName[0], m_watchRootName[0], 29);
	__strncpy(data.rootName[1], m_watchRootName[1], 29);
	data.rootIP[0] = m_watchRootAddr[0];
	data.rootIP[1] = m_watchRootAddr[1];
	data.rootGameCount[0] = m_watchRootGameCount[0];
	data.rootGameCount[1] = m_watchRootGameCount[1];
	udpsend(&m_remoteAddr_recv, (char*)&data, sizeof(data));
}

void CNetMgr::send_watchData(int p_idx)
{
	// �O��watch�֘A�̔r�����邱��

	// ���݂̋L�^�ς̃��v�T�C�Y���擾
	// �ϐ��t���ꂽ���_�Ńw�b�_�[�����͊��ɑ��݂��Ă���͂�
	// �ϐ펞�͖��Đ��ł���M���𑗂��
	// �c��̃f�[�^�T�C�Y�����Ĉ��k�f�[�^�𑗂邩�ǂ������f����
	DWORD size = 0;
	if (m_1stCaster)
	{
		if (g_replay.m_frameCount > 1) size = REPLAY_HEADER_SIZE + sizeof(DWORD) * g_replay.m_frameCount;
	}
	else
	{
		size = m_watchRecvSize;
	}

	WRITE_REPLAY_RAWDATA(size);

	// ���k���T�|�[�g���Ă���A�T�C�Y��128byte�ȏ�œ]�����̈��k�f�[�^��������΍쐬
	if (m_watcher[p_idx].m_supportedFormat > 0 && (int)size - (int)m_watcher[p_idx].m_sendSize > 128 && m_watcher[p_idx].m_compSize == 0)
	{
		m_watcher[p_idx].m_compOffset = m_watcher[p_idx].m_sendSize;
		m_watcher[p_idx].m_compSize = zmwrite((char*)&g_replay.m_data + m_watcher[p_idx].m_sendSize, size - m_watcher[p_idx].m_sendSize, m_watcher[p_idx].m_compData, MAX_COMPWATCHDATASIZE);
		m_watcher[p_idx].m_compSendSize = 0;
		DBGOUT_NET("data compression org:%d comp:%d\n", size - m_watcher[p_idx].m_sendSize, m_watcher[p_idx].m_compSize);
	}

	if (m_watcher[p_idx].m_compSendSize < m_watcher[p_idx].m_compSize)
	{
		DBGOUT_NET("compdata %d/%d\n", m_watcher[p_idx].m_compSendSize, m_watcher[p_idx].m_compSize);

		// ���k�f�[�^������΂���𑗂�
		SPacket_CompWatchData data;
		data.packetType			= Packet_CompWatchData;
		data.compblock_offset	= m_watcher[p_idx].m_compSendSize;
		data.compall_offset		= m_watcher[p_idx].m_compOffset;
		data.compall_size		= m_watcher[p_idx].m_compSize;
		data.galleryCount		= m_totalGalleryCount;
		data.compblock_size		= m_watcher[p_idx].m_compSize - m_watcher[p_idx].m_compSendSize;
		
		// �����Ƒ���̍ő�p�P�b�g�̏��Ȃ����ŃT�C�Y����
		if (data.compblock_size > m_watcher[p_idx].m_maxPacketSize)	data.compblock_size = m_watcher[p_idx].m_maxPacketSize;
		if (data.compblock_size > g_iniFileInfo.m_maxPacketSize)	data.compblock_size = g_iniFileInfo.m_maxPacketSize;
		memcpy(data.data, m_watcher[p_idx].m_compData + data.compblock_offset, data.compblock_size);

		DBGOUT_NET("send_compWatchData %d(%d) last4 = %08x\n", data.compblock_offset, data.compblock_size, *((DWORD*)(m_watcher[p_idx].m_compData + data.compblock_offset + data.compblock_size - 4)));

		udpsend(&m_watcher[p_idx].m_remoteAddr, (char*)&data, data.compblock_size + SPacket_CompWatchData::PACKET_HEADER_SIZE);
	}
	else
	{
		SPacket_WatchData data;
		data.packetType = Packet_WatchData;
		data.offset		= m_watcher[p_idx].m_sendSize;
		data.galleryCount = m_totalGalleryCount;
		//DBGOUT_LOG("xxx send_watchData() %d\n", m_totalGalleryCount);

		if (m_watcher[p_idx].m_sendSize < size)
		{
			data.size = (WORD)(size - m_watcher[p_idx].m_sendSize);

			// dest�T�C�Y����
			if (data.size > m_watcher[p_idx].m_maxPacketSize)	data.size = m_watcher[p_idx].m_maxPacketSize;
			if (data.size > g_iniFileInfo.m_maxPacketSize)		data.size = g_iniFileInfo.m_maxPacketSize;
	
			// src�T�C�Y����
			if (m_watcher[p_idx].m_sendSize + data.size > sizeof(ReplayFile)) data.size = (WORD)(m_watcher[p_idx].m_sendSize + data.size - sizeof(ReplayFile));
			memcpy(data.data, (char*)&g_replay.m_data + m_watcher[p_idx].m_sendSize, data.size);
		}
		else data.size = 0;

		DBGOUT_NET("send_watchData %d(%d)\n", data.offset, data.size);

		// ���ɑ��t�ς݂ł����邱�ƂŐڑ����p�����Ă��邱�Ƃ��m�F����
		udpsend(&m_watcher[p_idx].m_remoteAddr, (char*)&data, data.size + SPacket_WatchData::PACKET_HEADER_SIZE);
	}
	if (m_watcher[p_idx].m_sendTime == 0xffffffff) m_watcher[p_idx].m_sendTime = timeGetTime();
}

void CNetMgr::send_watchDataReply(int p_size)
{
	SPacket_WatchDataReply	data;
	data.packetType	= Packet_WatchDataReply;
	data.reserved	= 0;
	data.size		= p_size;
	data.childCount	= 1 + getChildWatcherCount();
	udpsend(&m_remoteAddr_recv, (char*)&data, sizeof(data));
}

void CNetMgr::send_compWatchDataReply(int p_compsize, int p_rawsize)
{
	SPacket_CompWatchDataReply	data;
	data.packetType	= Packet_CompWatchDataReply;
	data.reserved	= 0;
	data.compsize	= p_compsize;
	data.rawsize	= p_rawsize;
	data.childCount	= 1 + getChildWatcherCount();
	udpsend(&m_remoteAddr_recv, (char*)&data, sizeof(data));
}

bool CNetMgr::send_ping(sockaddr_in* p_addr, int p_selNodeIdx)
{
	CNode* node = g_nodeMgr->getNode(p_selNodeIdx);

	node->m_sendpingtime = timeGetTime();

	SPacket_Ping data;
	data.packetType	= Packet_Ping;
	data.cid		= CONNECTION_ID;

	data.scriptCode	= g_scriptCode;
	getNameTrip(data.name);
	__strncpy(data.ver, GGNVERSTR, 9);
	memcpy(data.mac, g_machineID, 6);

	data.delay		= g_setting.delay;
	data.ex			= g_setting.useEx;
	data.wins		= g_setting.wins;
	data.rank		= g_setting.rank;
	data.notready	= g_vsnet.m_menu_visible || (m_lobbyFrame < g_setting.wait * 60 + 30);
	data.ignoreSlow	= g_setting.ignoreSlow;
	data.round		= g_setting.rounds;
	data.deny		= (g_denyListMgr->find(node->m_id) >= 0);	// ���ۂ��Ă���
	data.needDetail = node->m_needDetail;
	data.gamecount	= g_setting.totalBattle;
	memcpy(data.hdid, &g_machineID[6], 4);
	data.watchFlags	= (m_watch ? WF_WATCH : 0) | (g_setting.watchIntrusion ? WF_INTRUSION : 0);
	data.watchMaxNode = g_setting.watchBroadcast ? g_setting.watchMaxNodes : -16;
	udpsend(p_addr, (char*)&data, sizeof(data));
	
	return true;
}

void CNetMgr::send_pingReply120(bool p_needDetail, bool p_deny, bool p_underV113)
{
	/* ping�𑗂��Ă����A�h���X�ɕԐM */
	/* ver1.20�ȑO�Ƃ̌݊��̂��߂����ɑ��� */
	if (p_needDetail && !m_connect && !m_watch)
	{
		if (p_underV113)
		{
			SPacket_PingReply112_3 data;
			data.packetType	= Packet_PingReply112_3;
			data.cid		= CONNECTION_ID;
			data.delay		= g_setting.delay;
			data.ex			= g_setting.useEx;
			data.wins		= g_setting.wins;
			data.rank		= g_setting.rank;
			data.notready	= g_vsnet.m_menu_visible || (m_lobbyFrame < g_setting.wait * 60 + 30);
			data.ignoreSlow	= g_setting.ignoreSlow;
			data.round		= g_setting.rounds;
			data.deny		= p_deny;
			data.gamecount	= g_setting.totalBattle;
			__strncpy(data.msg, g_setting.msg, 255);
			udpsend(&m_remoteAddr_recv, (char*)&data, sizeof(data));
		}
		else	// ver1.13�ȏ�
		{
			SPacket_PingReply data;
			data.packetType	= Packet_PingReply;
			data.cid		= CONNECTION_ID;
			data.delay		= g_setting.delay;
			data.ex			= g_setting.useEx;
			data.wins		= g_setting.wins;
			data.rank		= g_setting.rank;
			data.notready	= g_vsnet.m_menu_visible || (m_lobbyFrame < g_setting.wait * 60 + 30);
			data.ignoreSlow	= g_setting.ignoreSlow;
			data.round		= g_setting.rounds;
			data.deny		= p_deny;
			data.gamecount	= g_setting.totalBattle;
			__strncpy(data.msg, g_setting.msg, 255);
			memcpy(data.id, g_machineID, 10);
			udpsend(&m_remoteAddr_recv, (char*)&data, sizeof(data));
		}
	}
	else
	{
		/* ���Ƀ��b�Z�[�W���擾���Ă���^�ΐ�A�ϐ풆�͏ڍוs�v */
		SPacket_PingReplyLite120 data;
		data.packetType	= Packet_PingReplyLite120;
		data.cid		= CONNECTION_ID;
		data.delay		= g_setting.delay;
		data.ex			= g_setting.useEx;
		data.wins		= g_setting.wins;
		data.rank		= g_setting.rank;
		data.round		= g_setting.rounds;
		data.deny		= p_deny;
		data.gamecount	= g_setting.totalBattle;

		// �ΐ풆�Ȃ瑊��E�L�������𑗂�
		if (m_connect && g_replay.m_repRecording)
		{
			__strncpy(data.name[0], g_replay.m_data.name1P, 29);
			__strncpy(data.name[1], g_replay.m_data.name2P, 29);
			data.chara[0] = (char)g_replay.m_data.chara1P;
			data.chara[1] = (char)g_replay.m_data.chara2P;
		}
		else
		{
			data.name[0][0] = '\0';
			data.name[1][0] = '\0';
			data.chara[0] = CID_SOL;
			data.chara[1] = CID_SOL;
		}

		memcpy(data.id, g_machineID, 10);
		__strncpy(data.ver, GGNVERSTR, 9);

		data.watchFlags		= (m_watch ? WF_WATCH : 0) | (g_setting.watchIntrusion ? WF_INTRUSION : 0);
		data.watchMaxNode	= g_setting.watchBroadcast ? g_setting.watchMaxNodes : -16;

		// ����Ƃ̍��v��t�m�[�h����1�ȏ�Ȃ�ϐ�\�i���ۂȂ�-16���Z�b�g����Ă���j
		bool casting = m_connect;
		casting &= (g_setting.watchBroadcast ? g_setting.watchMaxNodes : -16) + g_enemyInfo.m_watchMaxNode > 0;

		// g_enemyInfo.m_rank == -1�̂Ƃ�Busy�ɂ͂��Ȃ�
		// g_enemyInfo�̏�񂪎󂯎��Ă��Ȃ��܂܁ABusy�ő����Ă��܂���
		// ���̌�Ping�𑗂�Ȃ��̂ŁA���肪Casting=On�������ꍇ�ɍX�V�������Ȃ��Ȃ�
		if (m_watch == false)
		{
			if (g_enemyInfo.m_rank == -1)			data.busy = BF_IDLE;		// idle[��]
			else if (casting && getGGXXMODE2() != 6)data.busy = BF_BUSY_CAST_NG;// busy(casting)[�D] ��ver�ł�busy���������悤��
			else if (casting && getGGXXMODE2() == 6)data.busy = BF_BUSY_CAST_OK;// busy(casting)[��] ��ver�ł�busy���������悤��
			else if (m_connect)						data.busy = BF_BUSY;		// busy[�D]
			else									data.busy = BF_IDLE;		// idle[��]
		}
		else data.busy = BF_BUSY;	// busy[�D](���o�[�W�����ł�Watch����Busy�ɂȂ��Ăق���)

		udpsend(&m_remoteAddr_recv, (char*)&data, sizeof(data));
	}
	//DBGOUT_NET("send_pingReply120\n");
}

void CNetMgr::send_pingReply(bool p_deny)
{
	/* ping�𑗂��Ă����A�h���X�ɕԐM */

	// g_enemyInfo.m_rank == -1�̂Ƃ�Busy�Ƃ��Ĉ���Ȃ�
	// g_enemyInfo�̏�񂪎󂯎��Ă��Ȃ��܂܁ABusy�ő����Ă��܂���
	// ���̌�Ping�𑗂�Ȃ��̂ŁA���肪Casting=On�������ꍇ�ɍX�V�������Ȃ��Ȃ�
	bool idle = (m_connect && g_enemyInfo.m_rank == -1);

	if (!m_connect && !m_watch || idle)	// idle
	{
		SPacket_PingReply_Idle data;
		data.packetType	= Packet_PingReply_Idle;
		data.cid		= CONNECTION_ID;

		data.notready = g_vsnet.m_menu_visible || (m_lobbyFrame < g_setting.wait * 60 + 30);

		__strncpy(data.ver, GGNVERSTR, 9);
		memcpy(data.id, g_machineID, 10);
		
		data.delay		= g_setting.delay;
		data.ex			= g_setting.useEx;
		data.wins		= g_setting.wins;
		data.rank		= g_setting.rank;
		data.round		= g_setting.rounds;
		data.ignoreSlow	= g_setting.ignoreSlow;
		data.deny		= p_deny;
		data.gamecount	= g_setting.totalBattle;
		
		data.allowIntrusion = g_setting.watchIntrusion != 0;
		data.watchMaxNode	= g_setting.watchBroadcast ? g_setting.watchMaxNodes : -16;

		udpsend(&m_remoteAddr_recv, (char*)&data, sizeof(data));
	}
	else if (m_watch)	// watch
	{
		SPacket_PingReply_Watch data;
		data.packetType	= Packet_PingReply_Watch;
		data.cid		= CONNECTION_ID;

		__strncpy(data.ver, GGNVERSTR, 9);
		memcpy(data.id, g_machineID, 10);

		data.delay		= g_setting.delay;
		data.ex			= g_setting.useEx;
		data.wins		= g_setting.wins;
		data.rank		= g_setting.rank;
		data.round		= g_setting.rounds;
		data.ignoreSlow	= g_setting.ignoreSlow;
		data.deny		= p_deny;
		data.gamecount	= g_setting.totalBattle;
		
		data.allowIntrusion = g_setting.watchIntrusion != 0;
		data.watchMaxNode	= g_setting.watchBroadcast ? g_setting.watchMaxNodes : -16;

		udpsend(&m_remoteAddr_recv, (char*)&data, sizeof(data));
	}
	else if (m_connect)
	{
		// ����Ƃ̍��v��t�m�[�h����1�ȏ�Ȃ�ϐ�\�i���ۂȂ�-16���Z�b�g����Ă���j
		bool casting = (g_setting.watchBroadcast ? g_setting.watchMaxNodes : -16) + g_enemyInfo.m_watchMaxNode > 0;
		
		if (casting)	// busy casting
		{
			SPacket_PingReply_BusyCasting data;
			data.packetType	= Packet_PingReply_BusyCasting;
			data.cid		= CONNECTION_ID;

#if TESTER
			data.casting = 1;
#else
			data.casting = getGGXXMODE2() == 6; // �ΐ풆�Ȃ�OK
#endif
			data.wins		= g_setting.wins;
			data.rank		= g_setting.rank;
			data.gamecount	= g_setting.totalBattle;

			udpsend(&m_remoteAddr_recv, (char*)&data, sizeof(data));
		}
		else			// busy
		{
			SPacket_PingReply_Busy data;
			data.packetType	= Packet_PingReply_Busy;
			data.cid		= CONNECTION_ID;

			udpsend(&m_remoteAddr_recv, (char*)&data, sizeof(data));
		}
	}
	//DBGOUT_NET("send_pingReply\n");
}

void CNetMgr::send_comment(void)
{
	SPacket_Comment data;
	data.packetType	= Packet_Comment;
	data.size = strnlen(g_setting.msg, 256);
	__strncpy(data.msg, g_setting.msg, data.size);
	udpsend(&m_remoteAddr_recv, (char*)&data, sizeof(char) * 3 + data.size);
}

void CNetMgr::send_vsLoadCompleted(void)
{
	SPacket_VSLoadCompleted data;
	data.packetType	= Packet_VSLoadCompleted;
	data.cid		= CONNECTION_ID;
	udpsend(&m_remoteAddr_active, (char*)&data, sizeof(data));
}

bool CNetMgr::send_suspend(void)
{
	SPacket_Suspend data;
	data.packetType	= Packet_Suspend;
	data.cid		= CONNECTION_ID;
	udpsend(&m_remoteAddr_active, (char*)&data, sizeof(data));

	m_waitingSuspendReply = true;
	for (int i = 0; i * 10 < TIMEOUT_REPLY; i++)
	{
		Sleep(10);
		if (!m_waitingSuspendReply)
		{
			DBGOUT_NET("send_suspend success\n");
			return true;
		}
		/* �đ� */
		if (i % 5 == 0) udpsend(&m_remoteAddr_active, (char*)&data, sizeof(data));
	}
	DBGOUT_NET("send_suspend timeout\n");
	m_waitingSuspendReply = false;
	return false;
}

void CNetMgr::send_suspendReply(void)
{
	SPacket_SuspendReply data;
	data.packetType	= Packet_SuspendReply;
	data.cid		= CONNECTION_ID;
	udpsend(&m_remoteAddr_recv, (char*)&data, sizeof(data));
}

bool CNetMgr::send_resume(void)
{
	SPacket_Resume data;
	data.packetType	= Packet_Resume;
	data.cid		= CONNECTION_ID;
	udpsend(&m_remoteAddr_active, (char*)&data, sizeof(data));

	m_waitingResumeReply = true;
	for (int i = 0; i * 10 < TIMEOUT_REPLY; i++)
	{
		Sleep(10);
		if (!m_waitingResumeReply)
		{
			DBGOUT_NET("send_resume success\n");
			return true;
		}
		/* �đ� */
		if (i % 5 == 0) udpsend(&m_remoteAddr_active, (char*)&data, sizeof(data));
	}
	DBGOUT_NET("send_resume timeout\n");
	m_waitingResumeReply = false;
	return false;
}

void CNetMgr::send_resumeReply(void)
{
	SPacket_ResumeReply data;
	data.packetType	= Packet_ResumeReply;
	data.cid		= CONNECTION_ID;
	udpsend(&m_remoteAddr_recv, (char*)&data, sizeof(data));
}

void CNetMgr::send_dataReply(int p_seq)
{
	SPacket_DataReply data;
	data.packetType	= Packet_DataReply;
	data.seq		= p_seq;
	udpsend(&m_remoteAddr_active, (char*)&data, sizeof(data));
}

void CNetMgr::send_nodeaddr115_3(sockaddr_in* p_addr, CNode* p_node)
{
	if (p_node->m_scriptCode != 0)
	{
		SPacket_NodeAddr data;
		data.packetType = Packet_NodeAddr;
		data.scriptCode = p_node->m_scriptCode;
		memcpy(data.name, p_node->m_name, 30);
		memcpy(data.addr, p_node->m_addr, 32);
		udpsend(p_addr, (char*)&data, sizeof(data));
	}
}

void CNetMgr::send_galleryCount(void)
{
	SPacket_GalleryCount data;
	data.packetType		= Packet_GalleryCount;
	data.galleryCount	= getChildWatcherCount();
	udpsend(&m_remoteAddr_active, (char*)&data, sizeof(data));
}

void CNetMgr::send_galleryCountForLobby(void)
{
	if (m_watchRecvSize > REPLAY_HEADER_SIZE)
	{
		SPacket_GalleryCountForLobby data;
		data.packetType		= Packet_GalleryCountForLobby;
		data.galleryCount	= m_totalGalleryCount;
		__strncpy(data.name[0], m_watchRootName[0], 29);
		__strncpy(data.name[1], m_watchRootName[1], 29);
		data.ip[0] = m_watchRootAddr[0].sin_addr.S_un.S_addr;
		data.ip[1] = m_watchRootAddr[1].sin_addr.S_un.S_addr;
		data.gameCount[0] = m_watchRootGameCount[0];
		data.gameCount[1] = m_watchRootGameCount[1];
		udpsend(&m_remoteAddr_recv, (char*)&data, sizeof(data));
	}
}

void CNetMgr::send_nameRequest(sockaddr_in* p_addr)
{
	SPacket_NameRequest data;
	data.packetType		= Packet_NameRequest;
	data.cid			= CONNECTION_ID;
	udpsend(p_addr, (char*)&data, sizeof(data));
}

void CNetMgr::send_debugInfo(void)
{
	SPacket_DebugInfo data;
	data.packetType	= Packet_DebugInfo;
	memcpy(data.cpu_name, g_cpuid.getCpuName(), 49);
	data.cpu_eax	= g_cpuid.getEAX();
	data.cpu_ecx	= g_cpuid.getECX();
	data.cpu_edx	= g_cpuid.getEDX();
	data.fcw		= GETFCW();
	data.analog[0]	= *((char*)0x602910);
	data.analog[1]	= *((char*)0x602944);
	data.ggmode		= *((char*)0x602962);

	for (int i = 0; i < 5; i++)
	{
		udpsend(&m_remoteAddr_active, (char*)&data, sizeof(data));
		Sleep(10);
	}
}

bool CNetMgr::send_battleInfoRequest(char* p_targetName, sockaddr_in* p_targetIP, int p_targetGameCount)
{
	SPacket_BattleInfoRequest	data;
	data.packetType	= Packet_BattleInfoRequest;
	data.targetIP	= p_targetIP->sin_addr;
	__strncpy(data.targetName, p_targetName, 29);
	data.targetGameCount = p_targetGameCount;

	m_waitingBattleInfoRequestReply = true;
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < g_nodeMgr->getNodeCount(); j++)
		{
			CNode* node = g_nodeMgr->getNode(j);
			if (node->m_state == State_Watch || node->m_state == State_Watch_Playable ||
				node->m_state == State_Idle || node->m_state == State_NotReady)
			{
				sockaddr_in addr = g_netMgr->getAddrFromString(node->m_addr);
				udpsend(&addr, (char*)&data, sizeof(data));
			}
		}
		Sleep(10);
		if (!m_waitingBattleInfoRequestReply)
		{
			DBGOUT_NET("send_batinf_request success\n");
			return true;
		}
	}
	// �N����������������̂Œ��ڌq��
	for (int i = 0; i < 5; i++)
	{
		/* �đ� */
		udpsend(p_targetIP, (char*)&data, sizeof(data));
		Sleep(10);
		if (!m_waitingBattleInfoRequestReply)
		{
			DBGOUT_NET("send_batinf_request(root) success\n");
			return true;
		}
	}
	DBGOUT_NET("send_batinf_request timeout\n");
	m_waitingBattleInfoRequestReply = false;
	return false;
}

void CNetMgr::send_battleInfo(char* p_name1, char* p_name2, DWORD p_ip1, DWORD p_ip2, DWORD p_gamecount1, DWORD p_gamecount2, char p_chara1, char p_chara2)
{
	SPacket_BattleInfo data;
	data.packetType	= Packet_BattleInfo;
	__strncpy(data.name[0], p_name1, 29);
	__strncpy(data.name[1], p_name2, 29);
	data.ip[0].S_un.S_addr = p_ip1;
	data.ip[1].S_un.S_addr = p_ip2;
	data.gamecount[0] = p_gamecount1;
	data.gamecount[1] = p_gamecount2;
	data.chara[0] = p_chara1;
	data.chara[1] = p_chara2;
	udpsend(&m_remoteAddr_recv, (char*)&data, sizeof(data));
}

bool CNetMgr::sendDataBlock(char p_type, char* p_data, int p_dataSize, int p_timeout)
{
#if USE_TCP
	char* data = new char[p_dataSize + 1];
	data[0] = p_type;
	if (p_data) memcpy(&data[1], p_data, p_dataSize);
	bool result = (g_netMgr->tcpsend(data, p_dataSize + 1, p_timeout) > 0);
	
	delete data;
	return result;
#else
	/* udp���g�p���đ�e�ʂ̃f�[�^���m���ɑ��� */

	/* �����Ƒ���ŏ��������ɍ��킹�� */
	int maxDataSize = g_iniFileInfo.m_maxPacketSize;
	if (maxDataSize > m_enMaxPacketSize) maxDataSize = m_enMaxPacketSize;
	for (int i = 0; i * maxDataSize < p_dataSize; i++)
	{
		int size = p_dataSize - i * maxDataSize;
		if (size > maxDataSize) size = maxDataSize;
		
		/* ���M�\�ȃT�C�Y�ɕ��� */
		SPacket_Data	data;
		data.packetType	= Packet_Data;
		data.type		= p_type;
		data.seq		= m_sendDataSeq;
		data.dataSize	= size;
		data.dataOffset	= maxDataSize * i;
		memcpy(data.data, &p_data[maxDataSize * i], size);
		
		bool success = false;
		m_waitingDataReply = true;
		for (int j = 0; j * 10 < p_timeout; j++)
		{
			if (!m_waitingDataReply)
			{
				DBGOUT_NET("send_data %d success\n", i);
				success = true;
				break;
			}
			/* �đ� */
			udpsend(&m_remoteAddr_active, (char*)&data, g_iniFileInfo.m_maxPacketSize + SPacket_Data::PACKET_HEADER_SIZE);
			Sleep(10);
		}
		if (success == false)
		{
			m_waitingDataReply = false;
			return false;
		}
		m_sendDataSeq++;
	}
	return true;
#endif
}

bool CNetMgr::recvDataBlock(char p_type, char* p_data, int p_dataSize, int p_timeout)
{
#if USE_TCP
	char* data = new char[p_dataSize + 1];
	
	int readsize = 0;
	while (readsize == 0)
	{
		readsize = g_netMgr->tcprecv(data, p_dataSize + 1, p_timeout);

		if (readsize == 0) break;

		if (data[0] == p_type)
		{
			memcpy(p_data, &data[1], readsize > p_dataSize ? p_dataSize : readsize);
			readsize -= 1; /* �^�C�v������ */
		}
		else readsize = 0;
	}
	delete data;
	return (readsize > 0);
#else
	/* udp���g�p���đ�e�ʂ̃f�[�^���m���Ɏ󂯎�� */
	m_recvDataPtr  = p_data;
	m_recvDataSize = p_dataSize;

	m_waitingDataType = p_type;
	m_waitingData = true;
	for (int i = 0; i * 10 < p_timeout; i++)
	{
		Sleep(10);
		if (!m_waitingData)
		{
			DBGOUT_NET("recvdata success\n");
			m_recvDataPtr = NULL;
			m_recvDataSize = 0;
			return true;
		}
	}
	DBGOUT_NET("recvdata timeout\n");
	m_waitingData = false;
	m_waitingDataType = -1;
	return false;
#endif
}

bool CNetMgr::talking(void)
{
	if (m_networkEnable == false) return false; 

	int packetSize = udprecv((char*)&m_buf, PACKETMAX_SIZE);
	if (packetSize <= 0) return false;
	
//#if DEBUG_OUTPUT_NET
//	char packetStr[256] = "";
//	if (m_buf.packetType == Packet_Connect)				strcpy(packetStr, "Packet_Connect");
//	else if (m_buf.packetType == Packet_ConnectReply)	strcpy(packetStr, "Packet_ConnectReply");
//	else if (m_buf.packetType == Packet_Resume)			strcpy(packetStr, "Packet_Resume");
//	else if (m_buf.packetType == Packet_ResumeReply)	strcpy(packetStr, "Packet_ResumeReply");
//	else if (m_buf.packetType == Packet_Suspend)		strcpy(packetStr, "Packet_Suspend");
//	else if (m_buf.packetType == Packet_SuspendReply)	strcpy(packetStr, "Packet_SuspendReply");
//	else if (m_buf.packetType == Packet_Data)			strcpy(packetStr, "Packet_Data");
//	else if (m_buf.packetType == Packet_DataReply)		strcpy(packetStr, "Packet_DataReply");
//	else if (m_buf.packetType == Packet_WatchIn)		strcpy(packetStr, "Packet_WatchIn");
//	else if (m_buf.packetType == Packet_WatchInRoot)	strcpy(packetStr, "Packet_WatchInRoot");
//	else if (m_buf.packetType == Packet_WatchData)		strcpy(packetStr, "Packet_WatchData");
//	else if (m_buf.packetType == Packet_WatchDataReply)	strcpy(packetStr, "Packet_WatchDataReply");
//
//	if (strcmp(packetStr, "") != 0)
//	{
//		DBGOUT_NET("recv %s from %d.%d.%d.%d\n", packetStr,
//				m_remoteAddr_recv.sin_addr.S_un.S_un_b.s_b1,
//				m_remoteAddr_recv.sin_addr.S_un.S_un_b.s_b2,
//				m_remoteAddr_recv.sin_addr.S_un.S_un_b.s_b3,
//				m_remoteAddr_recv.sin_addr.S_un.S_un_b.s_b4);
//	}
//#endif

	switch (m_buf.packetType)
	{
	case Packet_Ping:
		{
			SPacket_Ping *data = (SPacket_Ping*)&m_buf;
			
			/* server���Ⴄ�ꍇ�͖��� */
			if (useLobbyServer() && data->scriptCode != g_scriptCode) break;

			ENTERCS(&g_netMgr->m_csNode);
			
			char addrstr[32];
			getStringFromAddr(&m_remoteAddr_recv, addrstr);
			int idx = g_nodeMgr->findNodeIdx_address(addrstr);
			if (idx == -1)
			{
				if (useLobbyServer())
				{
					// �m��Ȃ��m�[�h����ping������o�^����
					// ver1.15-3����findNodeIdx_address�̔��f����ω������̂ŁA
					// NAT�ɂ��|�[�g���ω����Ă��܂��ꍇ�͂����ւ͂��Ȃ�
					idx = g_nodeMgr->addNode(addrstr, UNKNOWN_NAME, true, true);
					if (idx == -1)
					{
						LEAVECS(&g_netMgr->m_csNode);
						break;
					}
				}
				else
				{
					/* ping�ɂ��������Ȃ� */
					LEAVECS(&g_netMgr->m_csNode);
					break;
				}
			}

			CNode* node = g_nodeMgr->getNode(idx);

			// �L���ȃA�h���X�ōX�V
			__strncpy(node->m_addr, addrstr, 31);

			/* VersionError�`�F�b�N */
			if (data->cid != CONNECTION_ID)
			{
				node->m_state = State_VersionError;
				LEAVECS(&g_netMgr->m_csNode);
				break;
			}

			/* ����̏����X�V */
			node->m_delay		= data->delay;
			node->m_ex			= data->ex;
			node->m_rank		= data->rank;
			node->m_win			= data->wins;
			node->m_ignoreSlow	= data->ignoreSlow;
			node->m_round		= data->round;
			node->m_deny		= data->deny;
			node->m_gamecount	= data->gamecount;
			node->m_scriptCode	= data->scriptCode;
			__strncpy(node->m_name, data->name, 29);
			__strncpy(node->m_ver, data->ver, 9);

			node->clearBattleInfo();

			// �s���S�R�[�h�e�X�g
			memcpy(node->m_id, data->mac, 6);
			if (packetSize >= SPacket_Ping::SIZE120)
			{
				// 1.20���ping��10byte�̋��ۃR�[�h���擾�\
				memcpy(&node->m_id[6], data->hdid, 4);
			}

			/* ping�������Ƃ������Ƃ�busy�Ano response�ł͂Ȃ� */
			node->m_state = State_Idle;
			if (data->notready == 1)		node->m_state = State_NotReady;
			
			// 1.20���ϐ��Ԃ��T�|�[�g
			if (packetSize >= SPacket_Ping::SIZE120)
			{
				node->m_validInfo |= data->VF120;
				node->m_allowIntrusion  = (data->watchFlags & WF_INTRUSION) != 0;
				node->m_watchMaxNode	= data->watchMaxNode;

				if (data->watchFlags & WF_WATCH) node->m_state = State_Watch;
				if (node->m_state == State_Watch && node->m_allowIntrusion) node->m_state = State_Watch_Playable;
			}
			else
			{
				node->m_validInfo |= data->VF115;
				node->m_watchMaxNode = -16;
			}

			bool netSpeedGood = node->isNetSpeedGood();
			bool ignoreSlow = g_setting.ignoreSlow || node->m_ignoreSlow;
			if (!netSpeedGood && ignoreSlow)		node->m_state = State_PingOver;

			bool underV113 = strcmp(node->m_ver, "1.13") < 0;

			if (node->m_delay != g_setting.delay)			node->m_state = State_Mismatch;
			if (underV113 && node->m_ex != g_setting.useEx)	node->m_state = State_Mismatch;

			/* �ԐM��ł� */
			if (strcmp(node->m_ver, "1.20") <= 0)
			{
				send_pingReply120(data->needDetail, g_denyListMgr->find(node->m_id) >= 0, underV113);
			}
			else
			{
				send_pingReply(g_denyListMgr->find(node->m_id) >= 0);

				if (data->needDetail && !m_connect)
				{
					send_comment();
				}
			}

			/* �ϐ�̏����ԐM */
			if (m_watch)
			{
				if (strcmp(node->m_ver, "1.20") >= 0) send_galleryCountForLobby();
			}

			LEAVECS(&g_netMgr->m_csNode);
		}
		break;
	case Packet_PingReply112_3:
		{
			SPacket_PingReply112_3 *data = (SPacket_PingReply112_3*)&m_buf;

			ENTERCS(&g_netMgr->m_csNode);
			
			char addrstr[32];
			getStringFromAddr(&m_remoteAddr_recv, addrstr);
			int idx = g_nodeMgr->findNodeIdx_address(addrstr);
			if (idx != -1)
			{
				CNode* node = g_nodeMgr->getNode(idx);

				/* ping�l�v�� */
				node->recordPing();

				// �L���ȃA�h���X�ōX�V
				__strncpy(node->m_addr, addrstr, 31);

				/* VersionError�`�F�b�N */
				if (data->cid != CONNECTION_ID)
				{
					node->m_state = State_VersionError;
					LEAVECS(&g_netMgr->m_csNode);
					break;
				}

				/* ����̏����X�V */
				node->m_validInfo |= data->VF;
				node->m_delay		= data->delay;
				node->m_ex			= data->ex;
				node->m_rank		= data->rank;
				node->m_win			= data->wins;
				node->m_ignoreSlow	= data->ignoreSlow;
				node->m_round		= data->round;
				node->m_deny		= data->deny;
				node->m_gamecount	= data->gamecount;
				node->m_needDetail	= false;
				__strncpy(node->m_msg, data->msg, 255);

				node->m_watchMaxNode = -16;

				/* pingReply(�ڍ�)�������Ƃ������Ƃ�busy, no response�ł͂Ȃ� */
				node->m_state = State_Idle;
				if (data->notready == 1)				node->m_state = State_NotReady;

				bool netSpeedGood = node->isNetSpeedGood();
				bool ignoreSlow = g_setting.ignoreSlow || node->m_ignoreSlow;
				if (!netSpeedGood && ignoreSlow) node->m_state = State_PingOver;

				if (node->m_delay != g_setting.delay)									node->m_state = State_Mismatch;
				if (strcmp(node->m_ver, "1.13") < 0 && node->m_ex != g_setting.useEx)	node->m_state = State_Mismatch;
			}
			LEAVECS(&g_netMgr->m_csNode);
		}
		break;
	case Packet_PingReply:
		{
			SPacket_PingReply *data = (SPacket_PingReply*)&m_buf;

			ENTERCS(&g_netMgr->m_csNode);
			
			char addrstr[32];
			getStringFromAddr(&m_remoteAddr_recv, addrstr);
			int idx = g_nodeMgr->findNodeIdx_address(addrstr);
			if (idx != -1)
			{
				CNode* node = g_nodeMgr->getNode(idx);

				/* ping�l�v�� */
				node->recordPing();

				// �L���ȃA�h���X�ōX�V
				__strncpy(node->m_addr, addrstr, 31);

				/* VersionError�`�F�b�N */
				if (data->cid != CONNECTION_ID)
				{
					node->m_state = State_VersionError;
					LEAVECS(&g_netMgr->m_csNode);
					break;
				}

				/* ����̏����X�V */
				node->m_delay		= data->delay;
				node->m_ex			= data->ex;
				node->m_rank		= data->rank;
				node->m_win			= data->wins;
				node->m_ignoreSlow	= data->ignoreSlow;
				node->m_round		= data->round;
				node->m_deny		= data->deny;
				node->m_needDetail	= false;
				__strncpy(node->m_msg, data->msg, 255);

				if (packetSize >= SPacket_PingReply::SIZE120)
				{
					node->m_validInfo |= data->VF115;
					node->m_gamecount = data->gamecount;
				}
				else
				{
					node->m_validInfo |= data->VF120;
				}

				memcpy(node->m_id, data->id, 10);

				/* pingReply(�ڍ�)�������Ƃ������Ƃ�busy, no response�ł͂Ȃ� */
				node->m_state = State_Idle;
				if (data->notready == 1)				node->m_state = State_NotReady;

				bool netSpeedGood = node->isNetSpeedGood();
				bool ignoreSlow = g_setting.ignoreSlow || node->m_ignoreSlow;
				if (!netSpeedGood && ignoreSlow)		node->m_state = State_PingOver;

				if (node->m_delay != g_setting.delay)	node->m_state = State_Mismatch;
			}
			LEAVECS(&g_netMgr->m_csNode);
		}
		break;
	case Packet_PingReplyLite120:
		{
			SPacket_PingReplyLite120 *data = (SPacket_PingReplyLite120*)&m_buf;

			ENTERCS(&g_netMgr->m_csNode);
			
			char addrstr[32];
			getStringFromAddr(&m_remoteAddr_recv, addrstr);
			int idx = g_nodeMgr->findNodeIdx_address(addrstr);
			if (idx != -1)
			{
				CNode* node = g_nodeMgr->getNode(idx);
				
				/* ping�l�v�� */
				node->recordPing();

				// �L���ȃA�h���X�ōX�V
				__strncpy(node->m_addr, addrstr, 31);

				/* VersionError�`�F�b�N */
				if (data->cid != CONNECTION_ID)
				{
					node->m_state = State_VersionError;
					LEAVECS(&g_netMgr->m_csNode);
					break;
				}

				// 1.20���ϐ��Ԃ��T�|�[�g
				// ��������\�������邽�߂�deny�������邱�ƂɂȂ���
				if (packetSize >= SPacket_PingReplyLite120::SIZE120)
				{
					node->m_validInfo |= data->VF120;

					node->m_delay		= data->delay;
					node->m_ex			= data->ex;
					node->m_rank		= data->rank;
					node->m_win			= data->wins;
					//node->m_ignoreSlow	= 1;
					node->m_round		= data->round;
					node->m_deny		= data->deny;
					node->m_gamecount	= data->gamecount;

					// �ꉞ�����̏��Ƃ��Ă͏\���Ȃ̂ŗ��p����
					if (data->chara[0]-1 >= 0 && data->chara[0]-1 < CHARACOUNT &&
						data->chara[1]-1 >= 0 && data->chara[1]-1 < CHARACOUNT)
					{
						__strncpy(node->m_battleInfoName[0], data->name[0], 29);
						__strncpy(node->m_battleInfoName[1], data->name[1], 29);
						node->m_battleInfoIP[0] = 0;
						node->m_battleInfoIP[1] = 0;
						node->m_battleInfoGameCount[0] = -1;
						node->m_battleInfoGameCount[1] = -1;
						node->m_battleInfoChara[0] = data->chara[0];
						node->m_battleInfoChara[1] = data->chara[1];
					}

					memcpy(node->m_id, data->id, 10);
					__strncpy(node->m_ver, data->ver, 9);

					node->m_allowIntrusion  = (data->watchFlags & WF_INTRUSION) != 0;
					node->m_watchMaxNode	= data->watchMaxNode;

					if (data->watchFlags & WF_WATCH)
					{
						node->m_state = node->m_allowIntrusion ? State_Watch_Playable : State_Watch;
						
						bool netSpeedGood = node->isNetSpeedGood();
						bool ignoreSlow = g_setting.ignoreSlow || node->m_ignoreSlow;
						if (!netSpeedGood && ignoreSlow) node->m_state = State_PingOver;

						if (node->m_delay != g_setting.delay) node->m_state = State_Mismatch;
					}
					else
					{
						bool netSpeedGood = node->isNetSpeedGood();
						bool ignoreSlow = g_setting.ignoreSlow || node->m_ignoreSlow;
						if (!netSpeedGood && ignoreSlow) node->m_state = State_PingOver;

						/* busy�Ȃ炱��ȏ㑗�炸�A���肩�瑗���Ă���̂�҂� */
						switch (data->busy)
						{
						case BF_IDLE:			break;
						case BF_BUSY:			node->m_state = State_Busy; break;
						case BF_BUSY_CAST_OK:	node->m_state = State_Busy_Casting; break;
						case BF_BUSY_CAST_NG:	node->m_state = State_Busy_Casting_NG; break;
						default:				node->m_state = State_Busy; break;
						}

						if (node->m_state == State_Busy_Casting ||
							node->m_state == State_Busy_Casting_NG) 
						{
							if (node->m_delay != g_setting.delay) node->m_state = State_Mismatch;
						}
					}
				}
				else
				{
					node->m_validInfo |= data->VF115;

					if (data->busy != BF_IDLE) node->m_state = State_Busy;
					node->m_watchMaxNode = -16;
				}
			}
			LEAVECS(&g_netMgr->m_csNode);
		}
		break;
	case Packet_PingReply_BusyCasting:
		{
			SPacket_PingReply_BusyCasting *data = (SPacket_PingReply_BusyCasting*)&m_buf;

			ENTERCS(&g_netMgr->m_csNode);
			
			char addrstr[32];
			getStringFromAddr(&m_remoteAddr_recv, addrstr);
			int idx = g_nodeMgr->findNodeIdx_address(addrstr);
			if (idx != -1)
			{
				CNode* node = g_nodeMgr->getNode(idx);

				/* �L���ȃA�h���X�ōX�V */
				__strncpy(node->m_addr, addrstr, 31);

				/* VersionError�`�F�b�N */
				if (data->cid != CONNECTION_ID)
				{
					node->m_state = State_VersionError;
					LEAVECS(&g_netMgr->m_csNode);
					break;
				}

				/* ping�l�v�� */
				node->recordPing();

				/* ����̏����X�V */
				node->m_validInfo |= data->VF;
				node->m_win			= data->wins;
				node->m_rank		= data->rank;
				node->m_gamecount	= data->gamecount;

				/* State�̍X�V */
				node->m_state = data->casting ? State_Busy_Casting : State_Busy_Casting_NG;
			}
			LEAVECS(&g_netMgr->m_csNode);
		}
		break;
	case Packet_PingReply_Busy:
		{
			SPacket_PingReply_Busy *data = (SPacket_PingReply_Busy*)&m_buf;

			ENTERCS(&g_netMgr->m_csNode);
			
			char addrstr[32];
			getStringFromAddr(&m_remoteAddr_recv, addrstr);
			int idx = g_nodeMgr->findNodeIdx_address(addrstr);
			if (idx != -1)
			{
				CNode* node = g_nodeMgr->getNode(idx);
				
				node->clearBattleInfo();

				/* �L���ȃA�h���X�ōX�V */
				__strncpy(node->m_addr, addrstr, 31);

				/* VersionError�`�F�b�N */
				if (data->cid != CONNECTION_ID)
				{
					node->m_state = State_VersionError;
					LEAVECS(&g_netMgr->m_csNode);
					break;
				}

				/* ping�l�v�� */
				node->recordPing();
				
				/* ����̏����X�V */
				node->m_validInfo |= data->VF;

				/* State�̍X�V */
				node->m_state = State_Busy;
			}
			LEAVECS(&g_netMgr->m_csNode);
		}
		break;
	case Packet_PingReply_Watch:
		{
			SPacket_PingReply_Watch *data = (SPacket_PingReply_Watch*)&m_buf;

			ENTERCS(&g_netMgr->m_csNode);
			
			char addrstr[32];
			getStringFromAddr(&m_remoteAddr_recv, addrstr);
			int idx = g_nodeMgr->findNodeIdx_address(addrstr);
			if (idx != -1)
			{
				CNode* node = g_nodeMgr->getNode(idx);
				
				node->clearBattleInfo();

				/* �L���ȃA�h���X�ōX�V */
				__strncpy(node->m_addr, addrstr, 31);

				/* VersionError�`�F�b�N */
				if (data->cid != CONNECTION_ID)
				{
					node->m_state = State_VersionError;
					LEAVECS(&g_netMgr->m_csNode);
					break;
				}

				/* ping�l�v�� */
				node->recordPing();
				
				/* ����̏����X�V */
				node->m_validInfo |= data->VF;
				__strncpy(node->m_ver, data->ver, 9);
				memcpy(node->m_id, data->id, 10);

				node->m_delay		= data->delay;
				node->m_ex			= data->ex;
				node->m_win			= data->wins;
				node->m_rank		= data->rank;
				node->m_round		= data->round;
				node->m_ignoreSlow	= data->ignoreSlow;
				node->m_deny		= data->deny;
				node->m_gamecount	= data->gamecount;
				
				node->m_allowIntrusion = data->allowIntrusion;
				node->m_watchMaxNode = data->watchMaxNode;

				/* State�̍X�V */
				node->m_state = node->m_allowIntrusion ? State_Watch_Playable : State_Watch;

				if (node->m_state == State_Watch_Playable)
				{
					// �����̏ꍇ����Ping/DelayCheck����
					bool netSpeedGood = node->isNetSpeedGood();
					bool ignoreSlow = g_setting.ignoreSlow || node->m_ignoreSlow;
					if (!netSpeedGood && ignoreSlow) node->m_state = State_PingOver;

					if (node->m_delay != g_setting.delay) node->m_state = State_Mismatch;
				}
			}
			LEAVECS(&g_netMgr->m_csNode);
		}
		break;
	case Packet_PingReply_Idle:
		{
			SPacket_PingReply_Idle *data = (SPacket_PingReply_Idle*)&m_buf;

			ENTERCS(&g_netMgr->m_csNode);
			
			char addrstr[32];
			getStringFromAddr(&m_remoteAddr_recv, addrstr);
			int idx = g_nodeMgr->findNodeIdx_address(addrstr);
			if (idx != -1)
			{
				CNode* node = g_nodeMgr->getNode(idx);
				
				node->clearBattleInfo();

				/* �L���ȃA�h���X�ōX�V */
				__strncpy(node->m_addr, addrstr, 31);

				/* VersionError�`�F�b�N */
				if (data->cid != CONNECTION_ID)
				{
					node->m_state = State_VersionError;
					LEAVECS(&g_netMgr->m_csNode);
					break;
				}

				/* ping�l�v�� */
				node->recordPing();
				
				/* ����̏����X�V */
				node->m_validInfo |= data->VF;
				__strncpy(node->m_ver, data->ver, 9);
				memcpy(node->m_id, data->id, 10);

				node->m_delay		= data->delay;
				node->m_ex			= data->ex;
				node->m_win			= data->wins;
				node->m_rank		= data->rank;
				node->m_round		= data->round;
				node->m_ignoreSlow	= data->ignoreSlow;
				node->m_deny		= data->deny;
				node->m_gamecount	= data->gamecount;
				
				node->m_allowIntrusion = data->allowIntrusion;
				node->m_watchMaxNode = data->watchMaxNode;

				/* State�̍X�V */
				node->m_state = data->notready ? State_NotReady : State_Idle;

				bool netSpeedGood = node->isNetSpeedGood();
				bool ignoreSlow = g_setting.ignoreSlow || node->m_ignoreSlow;
				if (!netSpeedGood && ignoreSlow) node->m_state = State_PingOver;

				if (node->m_delay != g_setting.delay) node->m_state = State_Mismatch;
			}
			LEAVECS(&g_netMgr->m_csNode);
		}
		break;
	case Packet_Comment:
		{
			SPacket_Comment *data = (SPacket_Comment*)&m_buf;
			
			ENTERCS(&g_netMgr->m_csNode);
			
			char addrstr[32];
			getStringFromAddr(&m_remoteAddr_recv, addrstr);
			int idx = g_nodeMgr->findNodeIdx_address(addrstr);
			if (idx != -1)
			{
				CNode* node = g_nodeMgr->getNode(idx);
				node->m_needDetail = false;
				__strncpy(node->m_msg, data->msg, data->size);
				node->m_msg[data->size] = '\0';
				node->m_validInfo |= data->VF;
			}
			LEAVECS(&g_netMgr->m_csNode);
		}
		break;
	case Packet_Connect:
		/*
		�������ڑ��v�����o���Ă���Ԃ͑��̃m�[�h����̐ڑ������ۂ���
		Auto Connect Wait���I�����Ă�����O������ڑ��\
		�ؒf���シ���ɓ����m�[�h�֐ڑ�����Ƃ��������Ȃ邩������Ȃ����A
		timeout�Őؒf�����܂ł��̃m�[�h��busy�ƂȂ�͂��Ȃ̂�connect���M�ł��Ȃ��͂�
		������connect��Ԃł��A�����connectReply���͂��Ă��Ȃ��\��������̂�m_connect�̓`�F�b�N���Ȃ�
		*/
		// lobbyFrame�`�F�b�N�͊ϐ풆�ɏ�Q���o��̂ł�߂�B�ڑ�����NotReady�ɂ͂Ȃ�͂��Ȃ̂ő傫�Ȗ��͂Ȃ�
		if ((!m_watch || g_setting.watchIntrusion) && m_waitingConnectReply == false)
		{
			SPacket_Connect *data = (SPacket_Connect*)&m_buf;
			
			if (data->cid == CONNECTION_ID)
			{
				bool connectok = true;

				ENTERCS(&g_netMgr->m_csNode);

				char addrstr[32];
				getStringFromAddr(&m_remoteAddr_recv, addrstr);
				int idx = g_nodeMgr->findNodeIdx_address(addrstr);
				if (idx != -1)
				{
					// 08/02/04 �ϐ�̗����ɂ�ping�v���Ă���ɂ��Ȃ�
					// �v��������Ƃ������Ƃ͑��肩�猩����PingOver�ł͂Ȃ��̂Ńm�[�`�F�b�N�łȂ��Ă��܂����Ƃɂ���
					// 08/02/18 ���ȏ��ping�l���󂯂Ȃ��悤�ɂ��A1��1��ping�l�ɂ܂Ƃ��Ȓl�����҂ł���
					// watch��Ԃ𒲂ׂ�̂ɂP���ping�𑗂邽�߁A���̎���ping�l�Ń`�F�b�N����

					/* �ᑬ������ۂ̏ꍇ�Aping�l�𔻒f */
					/* ����PC�̕`�搫�\���ǂ����Ă��Ȃ��ꍇ�A�����ł͔��f�ł��Ȃ� */
					if (g_setting.ignoreSlow && g_nodeMgr->getNode(idx)->isNetSpeedGood() == false)
					{
						/* 60fps������肻���Ȃ̂ŋ��� */
						connectok = false;
						DBGOUT_NET("slow connection was ignored.\n");
					}
					else if (g_denyListMgr->find(g_nodeMgr->getNode(idx)->m_id) != -1)
					{
						/* ���ۂ��Ă��鑊�� */
						connectok = false;
						DBGOUT_NET("denied player was ignored.\n");
					}
				}
				else
				{
					/* �m�[�h��������Ȃ���΋��� */
					connectok = false;
					DBGOUT_NET("node not found.\n");
				}
				
				LEAVECS(&g_netMgr->m_csNode);

				if (connectok)
				{
					/* ������connect��Ԃł��A�����connectReply���͂��Ă��Ȃ��\��������̂ōđ����K�v */
					/* ���ڑ������ɐڑ���������ւ̍đ��ł����Reply���đ�����B */
					if (m_remoteAddr_active.sin_port == 0 || memcmp(&m_remoteAddr_active, &m_remoteAddr_recv, sizeof(m_remoteAddr_recv)) == 0)
					{
						/* �ϐ풆�Ȃ�ؒf */
						if (m_watch) disconnect("intrusion!!");

						/* �ڑ�������ʒm */
						g_netMgr->send_connectReply();
						connect();
						m_playSide = 2;

						/* �ő�p�P�b�g�T�C�Y���w�肳��Ă���ΐݒ� */
						if (packetSize >= SPacket_Connect::SIZE120_2)
						{
							m_enMaxPacketSize = data->maxPacketSize;
						}

						/* �����Őڑ�����̃A�h���X���i�[ */
						m_remoteAddr_active = m_remoteAddr_recv;
						DBGOUT_NET("active remote address updated!!\n");
					}
				}
			}
		}
		break;
	case Packet_ConnectReply:
		/* �O�̂��ߎ������ڑ��v�����o���Ă���Ԃ����ڑ����󗝂��� */
		if (!m_connect && !m_watch && m_waitingConnectReply)
		{
			SPacket_ConnectReply *data = (SPacket_ConnectReply*)&m_buf;

			if (data->cid == CONNECTION_ID)
			{
				connect();
				m_playSide = 1;

				/* �ő�p�P�b�g�T�C�Y���w�肳��Ă���ΐݒ� */
				if (packetSize >= SPacket_ConnectReply::SIZE120_2)
				{
					m_enMaxPacketSize = data->maxPacketSize;
				}

				/* ���M���� */
				m_waitingConnectReply = false;
			}
		}
		else
		{
			DBGOUT_NET("connection was duplicated!!\n");
		}
		break;
	case Packet_Key:
		if (m_connect)
		{
			SPacket_Key *data = (SPacket_Key*)&m_buf;

			ENTERCS(&m_csKey);
			
			int diff = m_time - data->time;
			
			/* �x����\�� */
			//DBGOUT_NET("%d:%d\n", m_time, diff);

			for (int i = 0; i < m_queueSize; i++)
			{
				int idx1 = (diff > 0) ? (i + diff) : i;
				int idx2 = (diff < 0) ? (i - diff) : i;

				if (idx1 >= m_queueSize || idx2 >= m_queueSize) break;

				if (m_playSide == 1)
				{
					m_key[idx1]		= (m_key[idx1] & 0x0000FFFF) | (data->cell[idx2].key << 16);
					m_syncChk[idx1] = (m_syncChk[idx1] & 0x00FF) | (data->cell[idx2].syncChk << 8);
				}
				if (m_playSide == 2)
				{
					m_key[idx1]		= (m_key[idx1] & 0xFFFF0000) | data->cell[idx2].key;
					m_syncChk[idx1]	= (m_syncChk[idx1] & 0xFF00) | data->cell[idx2].syncChk;
				}
			}
			LEAVECS(&m_csKey);
		}
		break;
	case Packet_VSLoadCompleted:
		if (m_connect && g_netMgr->m_recvVSLoadCompleted == false)
		{
			SPacket_VSLoadCompleted *data = (SPacket_VSLoadCompleted*)&m_buf;

			if (data->cid == CONNECTION_ID)
			{
				g_netMgr->m_recvVSLoadCompleted = true;
				DBGOUT_NET("VsLoadCompleted!!\n");
			}
		}
		break;
	case Packet_Suspend:
		/* m_recvSuspend == false�Ŗ����Ă��������s�� */
		/* suspendReply������ɓ͂��Ă��Ȃ��\�������邽�� */
		if (m_connect)
		{
			SPacket_Suspend *data = (SPacket_Suspend*)&m_buf;
			if (data->cid == CONNECTION_ID)
			{
				/* ��M������ʒm */
				send_suspendReply();
				m_recvSuspend = true;
			}
		}
		break;
	case Packet_SuspendReply:
		if (m_connect && m_waitingSuspendReply)
		{
			SPacket_SuspendReply *data = (SPacket_SuspendReply*)&m_buf;
			if (data->cid == CONNECTION_ID)
			{
				/* ���M���� */
				m_waitingSuspendReply = false;
			}
		}
		break;
	case Packet_Resume:
		/* m_recvSuspend == true�Ŗ����Ă��������s�� */
		/* resumeReply������ɓ͂��Ă��Ȃ��\�������邽�� */
		if (m_connect)
		{
			SPacket_Resume *data = (SPacket_Resume*)&m_buf;
			if (data->cid == CONNECTION_ID)
			{
				/* ��M������ʒm */
				send_resumeReply();
				m_recvSuspend = false;
			}
		}
		break;
	case Packet_ResumeReply:
		if (m_connect && m_waitingResumeReply)
		{
			SPacket_ResumeReply *data = (SPacket_ResumeReply*)&m_buf;
			if (data->cid == CONNECTION_ID)
			{
				/* ���M���� */
				m_waitingResumeReply = false;
			}
		}
		break;
	case Packet_Data:
		if (m_connect)
		{
			SPacket_Data *data = (SPacket_Data*)&m_buf;

			if (data->type == m_waitingDataType)
			{
				if (m_waitingData && m_recvDataSeq < data->seq)
				{
					if (m_recvDataSize >= data->dataOffset + data->dataSize)
					{
						memcpy(&m_recvDataPtr[data->dataOffset], data->data, data->dataSize);
					}
					else
					{
						DBGOUT_NET("m_recvDataPtr overflow!! type = %d\n", data->type);
					}
					DBGOUT_NET("testdata seq %3d,offset=%08x\n", data->seq, data->dataOffset);
					if (data->dataOffset + data->dataSize == m_recvDataSize) m_waitingData = false; /* �f�[�^��M���� */
					m_recvDataSeq = data->seq;
				}
				/* ��M������ʒm�i�đ��̂��ߌÂ��V�[�P���X�ł����M�j */
				if (m_recvDataSeq >= data->seq)
				{
					send_dataReply(data->seq);
					DBGOUT_NET("send testdatareply seq %3d, wait=%d, recvseq=%d\n", data->seq, m_waitingData, m_recvDataSeq);
				}
			}
		}
		break;
	case Packet_DataReply:
		if (m_connect && m_waitingDataReply)
		{
			SPacket_DataReply *data = (SPacket_DataReply*)&m_buf;
			if (data->seq == m_sendDataSeq)
			{
				/* ���M���� */
				m_waitingDataReply = false;
			}
		}
		DBGOUT_NET("testdatareply seq %3d, %d%d\n", m_sendDataSeq, m_connect, m_waitingDataReply);
		break;
	case Packet_WatchInRoot:
		if (m_connect && g_replay.m_repRecording)
		{
			SPacket_WatchInRoot	*data = (SPacket_WatchInRoot*)&m_buf;
			
			if (data->cid != CONNECTION_ID) break;

			short	maxPacketSize = 256;
			char	format = 0;
			int  targetGameCount = -1;
			if (packetSize >= SPacket_WatchInRoot::SIZE120_2)
			{
				targetGameCount = data->targetGameCount;
				format = data->format;
				maxPacketSize = data->maxPacketSize;
			}

			// �����J�E���g����v���Ă��邩�H
			if (targetGameCount == -1 || targetGameCount == g_setting.totalBattle)
			{
				int idx = findFreeWatchEntry(&m_remoteAddr_recv);
#if _DEBUG
				// watchin���󂯕t���Ȃ�
				if (g_ignoreWatchIn) idx = -1;
#endif
				if (idx != -1)
				{
					/* �����Ŋϐ�҂̃A�h���X���i�[ */
					ENTERCS(&m_csWatch);
					m_watcher[idx].m_remoteAddr = m_remoteAddr_recv;
					m_watcher[idx].m_sendSize = data->dataOffset;
					m_watcher[idx].m_childCount = 0;
					m_watcher[idx].m_sendTime = 0xffffffff;
					m_watcher[idx].m_supportedFormat = format;
					m_watcher[idx].m_compOffset = 0;
					m_watcher[idx].m_compSize = 0;
					m_watcher[idx].m_compSendSize = 0;
					m_watcher[idx].m_maxPacketSize = maxPacketSize;
					LEAVECS(&m_csWatch);

					DBGOUT_NET("watchinroot %d.%d.%d.%d:%d = %s\n",
						m_remoteAddr_recv.sin_addr.S_un.S_un_b.s_b1,
						m_remoteAddr_recv.sin_addr.S_un.S_un_b.s_b2,
						m_remoteAddr_recv.sin_addr.S_un.S_un_b.s_b3,
						m_remoteAddr_recv.sin_addr.S_un.S_un_b.s_b4,
						ntohs(m_remoteAddr_recv.sin_port),
						format == 0 ? "rawdata only" : "compression ok");
					send_watchInRootReply(1);
				}
				else
				{
					// �󂫂��Ȃ�
					send_watchInRootReply(0);
				}
			}
		}
		break;
	case Packet_WatchInRootReply:
		if (m_waitingWatchInRootReply == EWIRReply_Wait)
		{
			SPacket_WatchInRootReply *data = (SPacket_WatchInRootReply*)&m_buf;

			if (data->cid != CONNECTION_ID) break;
			
			if (data->accept)
			{
				// Format1
				m_watch = true;
				m_1stCaster = false;
				m_lastWatchDataTime = timeGetTime();

				// �z�M���̖��O�A����ID�A�A�h���X��ۑ�
				__strncpy(m_watchRootName[0], data->extra.format1.myName, 29);
				memcpy(&m_watchRootAddr[0], &m_remoteAddr_recv, sizeof(m_remoteAddr_recv));
				m_watchRootGameCount[0] = data->extra.format1.myGameCount;
				__strncpy(m_watchRootName[1], data->extra.format1.enemyName, 29);
				memcpy(&m_watchRootAddr[1], &data->enemyAddr, sizeof(data->enemyAddr));
				m_watchRootGameCount[1] = data->extra.format1.enemyGameCount;

				// ���ڒʐM����e�̃A�h���X��ۑ�
				m_watchParentAddr = m_remoteAddr_recv;

				/* ���M����(�ڑ���) */
				m_waitingWatchInRootReply = EWIRReply_Success;
				DBGOUT_NET("recv Packet_WatchInRootReply(success)\n");
			}
			else
			{
				// Format2
				// �ϐ�ł��Ȃ���΁A�A�h���X�������擾����B
				// Root���͂Q���z�M���s�����߂̊ϐ��ƍ��Ɏg�p���Ă��邾���Ȃ̂ŁA�ϐ킵�Ȃ�����Ӗ��͂Ȃ�
				memcpy(&m_watchRootAddr[0], &m_remoteAddr_recv, sizeof(m_remoteAddr_recv));
				memcpy(&m_watchRootAddr[1], &data->enemyAddr, sizeof(data->enemyAddr));

				// 1.20-2
				if (packetSize >= SPacket_WatchInRootReply::SIZE120_2_FMT2)
				{
					m_watchRootGameCount[0] = data->extra.format2.myGameCount;
					m_watchRootGameCount[1] = data->extra.format2.enemyGameCount;
				}
				/* ���M����(�ڑ��s��) */
				m_waitingWatchInRootReply = EWIRReply_Fail;
				DBGOUT_NET("recv Packet_WatchInRootReply(failed)\n");
			}
		}
		break;
	case Packet_WatchIn:
#if _DEBUG
		// watchin���󂯕t���Ȃ�
		if (g_ignoreWatchIn) break;
#endif
		if (m_watch)
		{
			SPacket_WatchIn	*data = (SPacket_WatchIn*)&m_buf;

			if (data->cid != CONNECTION_ID) break;
			
			short maxPacketSize = 256;
			char  format = 0;
			if (packetSize >= SPacket_WatchIn::SIZE120_2)
			{
				format = data->format;
				maxPacketSize = data->maxPacketSize;
			}

			for (int i = 0; i < 2; i++)
			{
				// �����J�E���g��IP����v���Ă��邩�H
				if (data->targetGameCount != m_watchRootGameCount[i]) continue;
				if (memcmp(&data->targetIP, &m_watchRootAddr[i].sin_addr, sizeof(in_addr)) != 0) continue;
				
				int idx;
				char addrstr[32];
				idx = g_nodeMgr->findNodeIdx_address(g_netMgr->getStringFromAddr(&m_watchRootAddr[i], addrstr));
				if (idx < 0) continue;

				// ���O����v���Ă��邩�H
				if (strncmp(data->targetName, g_nodeMgr->getNode(idx)->m_name, 29) != 0) continue;
				
				idx = findFreeWatchEntry(&m_remoteAddr_recv);
				if (idx != -1)
				{
					/* �����Ŋϐ�҂̃A�h���X���i�[ */
					ENTERCS(&m_csWatch);
					m_watcher[idx].m_remoteAddr = m_remoteAddr_recv;
					m_watcher[idx].m_sendSize = data->dataOffset;
					m_watcher[idx].m_childCount = 0;
					m_watcher[idx].m_sendTime = 0xffffffff;
					m_watcher[idx].m_supportedFormat = format;
					m_watcher[idx].m_compOffset = 0;
					m_watcher[idx].m_compSize = 0;
					m_watcher[idx].m_compSendSize = 0;
					m_watcher[idx].m_maxPacketSize = maxPacketSize;
					LEAVECS(&m_csWatch);
					DBGOUT_NET("watchin %d.%d.%d.%d:%d = %s\n",
						m_remoteAddr_recv.sin_addr.S_un.S_un_b.s_b1,
						m_remoteAddr_recv.sin_addr.S_un.S_un_b.s_b2,
						m_remoteAddr_recv.sin_addr.S_un.S_un_b.s_b3,
						m_remoteAddr_recv.sin_addr.S_un.S_un_b.s_b4,
						ntohs(m_remoteAddr_recv.sin_port),
						format == 0 ? "rawdata only" : "compression ok");
					send_watchInReply();
					break;
				}
			}
		}
		break;
	case Packet_WatchInReply:
		if (m_waitingWatchInReply)
		{
			SPacket_WatchInReply *data = (SPacket_WatchInReply*)&m_buf;

			if (data->cid != CONNECTION_ID) break;
			
			m_watch = true;
			m_1stCaster = false;
			m_lastWatchDataTime = timeGetTime();
			// �z�M�����O���󂯎��
			__strncpy(m_watchRootName[0], data->rootName[0], 29);
			__strncpy(m_watchRootName[1], data->rootName[1], 29);
			// �z�M���A�h���X���󂯎��
			m_watchRootAddr[0] = data->rootIP[0];
			m_watchRootAddr[1] = data->rootIP[1];
			m_watchRootGameCount[0] = data->rootGameCount[0];
			m_watchRootGameCount[1] = data->rootGameCount[1];

			// ���ڒʐM����e�̃A�h���X��ۑ�
			m_watchParentAddr = m_remoteAddr_recv;

			/* ���M���� */
			m_waitingWatchInReply = false;
		}
		break;
	case Packet_WatchData:
		if (m_watch)
		{
			// �e�ƔF�����Ă���m�[�h�ȊO����̒ʐM�͎󂯂Ȃ�
			if (m_watchParentAddr.sin_addr.S_un.S_addr != m_remoteAddr_recv.sin_addr.S_un.S_addr ||
				m_watchParentAddr.sin_port != m_remoteAddr_recv.sin_port)
			{
				break;
			}

			ENTERCS(&m_csWatch);

			SPacket_WatchData *data = (SPacket_WatchData*)&m_buf;
			
			// ��M�σT�C�Y�ƃI�t�Z�b�g����v���Ă���Ύ�M��t�ƕԐM���s��
			// �ԐM�ɂ͎�M�T�C�Y���܂߂đ��M����̂Ŏ��̎�M�v���Ƃ�������
			// ��M�f�[�^���w�b�_���{�}�[�W�������܂�����Đ��J�n����
			if (data->offset == m_watchRecvSize && data->offset + data->size <= sizeof(ReplayFile))
			{
				// ���ϐ�҂��󂯎��
				m_totalGalleryCount = data->galleryCount;
				//DBGOUT_LOG("xxx recv Packet_WatchData %d\n", data->galleryCount);

				if (data->size > 0)
				{
					m_watchRecvSize += data->size;
					memcpy((char*)&g_replay.m_data + data->offset, data->data, data->size);

					if (m_watchRecvSize > REPLAY_HEADER_SIZE)
					{
						int frameCount = (g_netMgr->m_watchRecvSize - REPLAY_HEADER_SIZE) / sizeof(DWORD);
						if (g_replay.m_data.inputData[frameCount - 1] == INPUT_SYNCERROR ||
							g_replay.m_data.inputData[frameCount - 1] == INPUT_DISCONNECT ||
							g_replay.m_data.inputData[frameCount - 1] == INPUT_COMPLETE)
						{
							// ��M���������̂Ŋϐ�v���͏o���Ȃ�
							m_watchRecvComplete = true;
						}
					}
					WRITE_REPLAY_RAWDATA(g_netMgr->m_watchRecvSize);
				}
				DBGOUT_NET("recv watchData %d(%d)\n", data->offset, data->size);

				// �Ō�Ƀf�[�^���󂯎�������Ԃ��L�^
				// �I�t�Z�b�g����v���Ă��Ȃ��ꍇ�͍X�V���Ȃ�
				m_lastWatchDataTime = timeGetTime();
			}
			else DBGOUT_NET("watchData error (need:%d recv:%d)\n", m_watchRecvSize, data->offset);

			// watchDataReply�����X�g���邩�����Ă��邩������Ȃ��̂ŁA�I�t�Z�b�g�̈�v�Ɋւ�炸�ԐM����B
			if (m_watchRecvComplete == false)
			{
				send_watchDataReply(g_netMgr->m_watchRecvSize);
			}

			LEAVECS(&m_csWatch);
		}
		break;
	case Packet_CompWatchData:
		if (m_watch)
		{
			// �e�ƔF�����Ă���m�[�h�ȊO����̒ʐM�͎󂯂Ȃ�
			if (m_watchParentAddr.sin_addr.S_un.S_addr != m_remoteAddr_recv.sin_addr.S_un.S_addr ||
				m_watchParentAddr.sin_port != m_remoteAddr_recv.sin_port)
			{
				break;
			}

			ENTERCS(&m_csWatch);

			SPacket_CompWatchData *data = (SPacket_CompWatchData*)&m_buf;
			
			// ��M�σT�C�Y�ƃI�t�Z�b�g����v���Ă���Ύ�M��t�ƕԐM���s��
			// �ԐM�ɂ͎�M�T�C�Y���܂߂đ��M����̂Ŏ��̎�M�v���Ƃ�������
			// ��M�f�[�^���w�b�_���{�}�[�W�������܂�����Đ��J�n����
			if (data->compblock_offset == m_watchRecvCompSize && data->compblock_offset + data->compblock_size <= data->compall_size)
			{
				// ���ϐ�҂��󂯎��
				m_totalGalleryCount = data->galleryCount;
				//DBGOUT_LOG("xxx recv Packet_WatchData %d\n", data->galleryCount);

				if (data->compblock_size > 0)
				{
					m_watchRecvCompSize += data->compblock_size;
					memcpy(m_watchRecvCompData + data->compblock_offset, data->data, data->compblock_size);

					// ���ݎ�M�ς̈��k�f�[�^��W�J
					int actualsize = zmread(m_watchRecvCompData, m_watchRecvCompSize, (char*)&g_replay.m_data + data->compall_offset, sizeof(ReplayFile) - m_watchRecvSize);
					DBGOUT_NET("comp=%d raw=%d rawoffset=%d last4=%08x\n", m_watchRecvCompSize, actualsize, data->compall_offset, *(DWORD*)(m_watchRecvCompData + m_watchRecvCompSize - 4));

					// ���̓f�[�^(1F������4�o�C�g)�����[�ɂȂ�Ȃ��悤�ɗ]�����f�[�^�͐؂�̂ĂĎ�M�ϐ��f�[�^���X�V
					int inputframe = ((int)data->compall_offset + actualsize - (int)REPLAY_HEADER_SIZE) / (int)sizeof(DWORD);
					if (inputframe > 0)
					{
						m_watchRecvSize = REPLAY_HEADER_SIZE + inputframe * sizeof(DWORD);
					}
					else
					{
						m_watchRecvSize = data->compall_offset + actualsize;
					}

					if (m_watchRecvSize > REPLAY_HEADER_SIZE)
					{
						int frameCount = (g_netMgr->m_watchRecvSize - REPLAY_HEADER_SIZE) / sizeof(DWORD);
						if (g_replay.m_data.inputData[frameCount - 1] == INPUT_SYNCERROR ||
							g_replay.m_data.inputData[frameCount - 1] == INPUT_DISCONNECT ||
							g_replay.m_data.inputData[frameCount - 1] == INPUT_COMPLETE)
						{
							// ��M���������̂Ŋϐ�v���͏o���Ȃ�
							m_watchRecvComplete = true;
						}
					}
					WRITE_REPLAY_RAWDATA(g_netMgr->m_watchRecvSize);
				}

				// ���k�f�[�^�����ׂĎ�M����
				if (m_watchRecvCompSize == data->compall_size) DBGOUT_NET("compdata complete!!\n");

				DBGOUT_NET("recv compWatchData %d(%d) of %d\n", data->compblock_offset, data->compblock_size, data->compall_size);
			
				// �Ō�Ƀf�[�^���󂯎�������Ԃ��L�^
				// �I�t�Z�b�g����v���Ă��Ȃ��ꍇ�͍X�V���Ȃ�
				m_lastWatchDataTime = timeGetTime();
			}
			else DBGOUT_NET("compWatchData error (need:%d recv:%d) of %d\n", m_watchRecvCompSize, data->compblock_offset, data->compall_size);

			// compWatchDataReply�����X�g���邩�����Ă��邩������Ȃ��̂ŁA�I�t�Z�b�g�̈�v�Ɋւ�炸�ԐM����B
			if (m_watchRecvComplete == false)
			{
				send_compWatchDataReply(m_watchRecvCompSize, m_watchRecvSize);
			}

			LEAVECS(&m_csWatch);
		}
		break;
	case Packet_WatchDataReply:
		if (m_connect || m_watch)
		{
			ENTERCS(&m_csWatch);

			SPacket_WatchDataReply *data = (SPacket_WatchDataReply*)&m_buf;

			for (int i = 0; i < WATCH_MAX_CHILD; i++)
			{
				if (m_watcher[i].m_remoteAddr.sin_addr.S_un.S_addr == m_remoteAddr_recv.sin_addr.S_un.S_addr &&
					m_watcher[i].m_remoteAddr.sin_port == m_remoteAddr_recv.sin_port)
				{
					// �q�ϐ�҂��󂯎��
					m_watcher[i].m_childCount = data->childCount;

					// ���M�σf�[�^�������Ă�����X�V���A���̑��M�f�[�^�̃I�t�Z�b�g�Ƃ���
					if (data->size > m_watcher[i].m_sendSize) m_watcher[i].m_sendSize = data->size;
					m_watcher[i].m_sendTime = 0xffffffff;
					break;
				}
			}
			LEAVECS(&m_csWatch);
		}
		break;
	case Packet_CompWatchDataReply:
		if (m_connect || m_watch)
		{
			ENTERCS(&m_csWatch);

			SPacket_CompWatchDataReply *data = (SPacket_CompWatchDataReply*)&m_buf;

			for (int i = 0; i < WATCH_MAX_CHILD; i++)
			{
				if (m_watcher[i].m_remoteAddr.sin_addr.S_un.S_addr == m_remoteAddr_recv.sin_addr.S_un.S_addr &&
					m_watcher[i].m_remoteAddr.sin_port == m_remoteAddr_recv.sin_port)
				{
					// �q�ϐ�҂��󂯎��
					m_watcher[i].m_childCount = data->childCount;
					
					if (data->compsize > m_watcher[i].m_compSendSize)
					{
						m_watcher[i].m_compSendSize = data->compsize;
						if (m_watcher[i].m_compSendSize == m_watcher[i].m_compSize)
						{
							// ���k�f�[�^�̎�M������
							DBGOUT_NET("compWatchData complete %d bytes\n", m_watcher[i].m_compSize);
							m_watcher[i].m_compOffset = 0;
							m_watcher[i].m_compSendSize = 0;
							m_watcher[i].m_compSize = 0;
						}
					}
					if (data->rawsize > m_watcher[i].m_sendSize) m_watcher[i].m_sendSize = data->rawsize;
					m_watcher[i].m_sendTime = 0xffffffff;
					break;
				}
			}
			LEAVECS(&m_csWatch);
		}
		break;
	case Packet_NodeAddr115_2:
		//{
		//	SPacket_NodeAddr115_2 *data = (SPacket_NodeAddr115_2*)&m_buf;
		//	ENTERCS(&g_netMgr->m_csNode);
		//	g_nodeMgr->addNode(data->addr, data->name, false, false);
		//	LEAVECS(&g_netMgr->m_csNode);
		//	DBGOUT_NET("Packet_NodeAddr115_2 %s %s\n", data->name, data->addr);
		//}
		break;
	case Packet_NodeAddr:
		{
			SPacket_NodeAddr *data = (SPacket_NodeAddr*)&m_buf;
			
			if (data->scriptCode == g_scriptCode)
			{
				ENTERCS(&g_netMgr->m_csNode);
				g_nodeMgr->addNode(data->addr, data->name, false, false);
				LEAVECS(&g_netMgr->m_csNode);

				//DBGOUT_NET("Packet_NodeAddr %s %s\n", data->name, data->addr);
			}
		}
		break;
	case Packet_GalleryCount:
		{
			SPacket_GalleryCount *data = (SPacket_GalleryCount*)&m_buf;
			m_recvGalleryCount  = data->galleryCount;
		}
		break;
	case Packet_GalleryCountForLobby:
		{
			SPacket_GalleryCountForLobby *data = (SPacket_GalleryCountForLobby*)&m_buf;

			if (packetSize >= SPacket_GalleryCountForLobby::SIZE120_2)
			{
				// ��Ver�̏��͑O�̎����̃M�������[����������Ȃ��̂Ŗ���
				ENTERCS(&g_netMgr->m_csNode);

				int idx1 = g_nodeMgr->findNodeIdx_name_ip(data->name[0], data->ip[0]);
				if (idx1 >= 0 && g_nodeMgr->getNode(idx1)->m_gamecount == data->gameCount[0])
				{
					g_nodeMgr->getNode(idx1)->m_galleryCount = data->galleryCount;
					g_nodeMgr->getNode(idx1)->m_galleryUpdateTime = timeGetTime();
				}
				int idx2 = g_nodeMgr->findNodeIdx_name_ip(data->name[1], data->ip[1]);
				if (idx2 >= 0 && g_nodeMgr->getNode(idx2)->m_gamecount == data->gameCount[1])
				{
					g_nodeMgr->getNode(idx2)->m_galleryCount = data->galleryCount;
					g_nodeMgr->getNode(idx2)->m_galleryUpdateTime = timeGetTime();
				}

				LEAVECS(&g_netMgr->m_csNode);
			}
		}
		break;
	case Packet_NameRequest:
		{
			ENTERCS(&g_netMgr->m_csNode);

			char addrstr[32];
			int idx = g_nodeMgr->findNodeIdx_address(g_netMgr->getStringFromAddr(&m_remoteAddr_recv, addrstr));
			if (idx >= 0) send_ping(&m_remoteAddr_recv, idx);

			LEAVECS(&g_netMgr->m_csNode);
		}
		break;
	case Packet_DebugInfo:
#if _DEBUG
		{
			SPacket_DebugInfo *data = (SPacket_DebugInfo*)&m_buf;
			data->cpu_name[49] = '\0';

			DBGOUT_NET("======= Sync Error Debug Info=======\n");
			DBGOUT_NET("name=%s eax=%p ecx=%p edx=%p\n", data->cpu_name, data->cpu_eax, data->cpu_ecx, data->cpu_edx);
			DBGOUT_NET("fcw=%04x analog=%d-%d ggmode=%d\n", data->fcw, data->analog[0], data->analog[1], data->ggmode);
			DBGOUT_NET("========================\n");
		}
#endif
		break;
	case Packet_BattleInfoRequest:
		{
			SPacket_BattleInfoRequest *data = (SPacket_BattleInfoRequest*)&m_buf;

			char  name[2][30];
			DWORD ip[2];
			int   gamecount[2];
			char  chara[2];
			if (m_watch && g_replay.m_playing)
			{
				// �v�����ꂽ�m�[�h���ϐ풆�̂��̂ƈ�v�����瑗�M
				bool hit = false;
				hit |= data->targetGameCount == m_watchRootGameCount[0] &&
					strcmp(data->targetName, m_watchRootName[0]) == 0 &&
					data->targetIP.S_un.S_addr == m_watchRootAddr[0].sin_addr.S_un.S_addr;

				hit |= data->targetGameCount == m_watchRootGameCount[1] &&
					strcmp(data->targetName, m_watchRootName[1]) == 0 &&
					data->targetIP.S_un.S_addr == m_watchRootAddr[1].sin_addr.S_un.S_addr;

				if (!hit) break;
				
				__strncpy(name[0], g_replay.m_data.name1P, 29);
				__strncpy(name[1], g_replay.m_data.name2P, 29);
				chara[0] = (char)g_replay.m_data.chara1P;
				chara[1] = (char)g_replay.m_data.chara2P;

				if (strcmp(g_replay.m_data.name1P, m_watchRootName[0]) == 0)
				{
					ip[0] = m_watchRootAddr[0].sin_addr.S_un.S_addr;
					ip[1] = m_watchRootAddr[1].sin_addr.S_un.S_addr;
					gamecount[0] = m_watchRootGameCount[0];
					gamecount[1] = m_watchRootGameCount[1];
				}
				else
				{
					ip[0] = m_watchRootAddr[1].sin_addr.S_un.S_addr;
					ip[1] = m_watchRootAddr[0].sin_addr.S_un.S_addr;
					gamecount[0] = m_watchRootGameCount[1];
					gamecount[1] = m_watchRootGameCount[0];
				}
				DBGOUT_NET("Battle Info Request Watch\n");
			}
			else if (m_connect && g_replay.m_repRecording)
			{
				// ���ڗv���������疳�����ő��M
				__strncpy(name[0], g_replay.m_data.name1P, 29);
				__strncpy(name[1], g_replay.m_data.name2P, 29);
				chara[0] = (char)g_replay.m_data.chara1P;
				chara[1] = (char)g_replay.m_data.chara2P;

				sockaddr_in myaddr = getAddrFromString(g_nodeMgr->getOwnNode());
				if (g_netMgr->m_playSide == 1)
				{
					ip[0] = myaddr.sin_addr.S_un.S_addr;
					ip[1] = m_remoteAddr_active.sin_addr.S_un.S_addr;
					gamecount[0] = g_setting.totalBattle;
					gamecount[1] = g_enemyInfo.m_gameCount;
				}
				else
				{
					ip[0] = m_remoteAddr_active.sin_addr.S_un.S_addr;
					ip[1] = myaddr.sin_addr.S_un.S_addr;
					gamecount[0] = g_enemyInfo.m_gameCount;
					gamecount[1] = g_setting.totalBattle;
				}
				DBGOUT_NET("Battle Info Request Busy\n");
			}
			else if (!m_connect && !m_watch)
			{
				// �m�[�h���X�g�Ɉ�v�����񂪂���Α��M
				CNode* node = NULL;
				bool hit = false;
				for (int i = 0; i < g_nodeMgr->getNodeCount(); i++)
				{
					node = g_nodeMgr->getNode(i);

					hit |= data->targetGameCount == node->m_battleInfoGameCount[0] &&
						strcmp(data->targetName, node->m_battleInfoName[0]) == 0 &&
						data->targetIP.S_un.S_addr == node->m_battleInfoIP[0];

					hit |= data->targetGameCount == node->m_battleInfoGameCount[1] &&
						strcmp(data->targetName, node->m_battleInfoName[1]) == 0 &&
						data->targetIP.S_un.S_addr == node->m_battleInfoIP[1];
					if (hit) break;
				}
				if (!hit) break;
				__strncpy(name[0], node->m_battleInfoName[0], 29);
				__strncpy(name[1], node->m_battleInfoName[1], 29);
				chara[0] = node->m_battleInfoChara[0];
				chara[1] = node->m_battleInfoChara[1];
				ip[0] = node->m_battleInfoIP[0];
				ip[1] = node->m_battleInfoIP[1];
				gamecount[0] = node->m_battleInfoGameCount[0];
				gamecount[1] = node->m_battleInfoGameCount[1];

				DBGOUT_NET("Battle Info Request Idle\n");
			}
			send_battleInfo(name[0], name[1], ip[0], ip[1], gamecount[0], gamecount[1], chara[0], chara[1]);
		}
		break;
	case Packet_BattleInfo:
		if (!m_connect && !m_watch && m_waitingBattleInfoRequestReply)
		{
			SPacket_BattleInfo *data = (SPacket_BattleInfo*)&m_buf;
			
			if (data->chara[0]-1 >= 0 && data->chara[0]-1 < CHARACOUNT &&
				data->chara[1]-1 >= 0 && data->chara[1]-1 < CHARACOUNT)
			{
				ENTERCS(&g_netMgr->m_csNode);

				for (int i = 0; i < 2; i++)
				{
					int idx = g_nodeMgr->findNodeIdx_name_ip(data->name[i], data->ip[i].S_un.S_addr);
					if (idx >= 0)
					{
						CNode* node = g_nodeMgr->getNode(idx);
						__strncpy(node->m_battleInfoName[0], data->name[0], 29);
						__strncpy(node->m_battleInfoName[1], data->name[1], 29);
						node->m_battleInfoIP[0] = data->ip[0].S_un.S_addr;
						node->m_battleInfoIP[1] = data->ip[1].S_un.S_addr;
						node->m_battleInfoChara[0] = data->chara[0];
						node->m_battleInfoChara[1] = data->chara[1];
						node->m_battleInfoGameCount[0] = data->gamecount[0];
						node->m_battleInfoGameCount[1] = data->gamecount[1];
					}
				}
				LEAVECS(&g_netMgr->m_csNode);
			}

			/* ���M���� */
			m_waitingBattleInfoRequestReply = false;
		}
		break;
	}

	return true;
}

int CNetMgr::udpsend(sockaddr_in* p_addr, char* p_data, int p_dataSize)
{
	return sendto(m_udpSocket, p_data, p_dataSize, 0, (sockaddr*)p_addr, sizeof(sockaddr_in));
}

int CNetMgr::udprecv(char* p_buf, int p_bufSize)
{
	int	raLen = sizeof(m_remoteAddr_recv);
	return recvfrom(m_udpSocket, p_buf, p_bufSize, 0, (sockaddr*)&m_remoteAddr_recv, &raLen);
}

#if USE_TCP
int CNetMgr::tcpsend(char* p_data, int p_dataSize, int p_timeout)
{
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		WSERR("socket");
		return 0;
	}

	int stTime = timeGetTime();
	int sendsize = 0;
	while ((int)timeGetTime() - stTime < p_timeout)
	{
		if (::connect(sock, (sockaddr*)&m_remoteAddr_active, sizeof(m_remoteAddr_active)) != SOCKET_ERROR)
		{
			sendsize = send(sock, p_data, p_dataSize, 0);
			if (sendsize == -1) WSERR("SEND");
			break;
		}
	}
	shutdown(sock, SD_BOTH);
	closesocket(sock);

	return sendsize;
}

int CNetMgr::tcprecv(char* p_buf, int p_bufSize, int p_timeout)
{
	timeval tv = { 0, p_timeout * 1000 };
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(m_tcpSocket, &fds);
	if (select(0, &fds, NULL, NULL, &tv) == 0)
	{
		WSERR("select(timeout)");
		return 0;
	}

	int readSize = 0;
	sockaddr_in addr2;
	int addrlen = sizeof(addr2);
	SOCKET sock = accept(m_tcpSocket, (sockaddr*)&addr2, &addrlen);
	if (sock == INVALID_SOCKET)
	{
		WSERR("accept");
		return 0;
	}
	for (int i = 0;; i++)
	{
		int size = recv(sock, &p_buf[readSize], p_bufSize - readSize, 0);
		if (size == -1) { WSERR("recv"); continue; }
		if (size == 0) break;
		readSize += size;
	}
	shutdown(sock, SD_BOTH);
	closesocket(sock);
	
	return readSize;
}
#endif
