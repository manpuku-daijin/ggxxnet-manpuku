#pragma once

//******************************************************************
// libs
//******************************************************************
#pragma comment(lib, "ws2_32.lib")

//******************************************************************
// includes
//******************************************************************
#include <winsock2.h>
#include <ws2tcpip.h>
#include "res/resource.h"

//******************************************************************
// defines
//******************************************************************
#define USE_TCP				FALSE

/* ggxxnet�R�l�N�V����ID�E��v���Ă����Ver�Ⴂ�ł��ʐM�\ */
/* ver1.11�ȍ~�Aver1.12�n���O�Ƃ��Ă��ׂ�12�ŌŒ肵�Ă��� */
#define CONNECTION_ID		12

#define	LOBBY_VER			6		/* ���r�[�o�[�W���� */
#define REPLAY_VER			3		/* Replay�o�[�W���� */
// 3 = 1.10-
// 2 = 1.09
// 1 = 1.06-1.08b
// 0 = 1.00-1.05
#define	PACKETMAX_SIZE		1024
#define LOG_SIZE			(1024*1024)	// 1MB

#define MAX_COMPWATCHDATASIZE	30000	// ���k�ςȂ̂�30kb����Η]�T

#define WATCH_MAX_CHILD_BASE	3
#define WATCH_MAX_CHILD_INC		2
#define WATCH_MAX_CHILD			(WATCH_MAX_CHILD_BASE + WATCH_MAX_CHILD_INC)

#define MSPF_FLOAT	16.66666667f
#define MSPF_INT	17

/* �e�^�C���A�E�g���ԁi�b�j */
#define TIMEOUT_VSLOAD	5000	/* VS��ʂ��I����Ă���̑҂����� */
#define TIMEOUT_SUSPEND	5000	/* �ڑ��`CS�J�n�ACS�I���`VS�J�n�܂ł�Suspend�^�C���A�E�g �ُ�P�[�X�Ȃ̂Œ��߂ŗǂ� */
#define TIMEOUT_KEY		1000	/* �L�[���� */
#define TIMEOUT_KEY2	3000	/* �L�[����(�t���[�Y�΍�) */
#define TIMEOUT_REPLY	3000	/* Suspend, Resume, Connect�AData�̕ԐM */
#define TIMEOUT_PING	500		/* ping�҂� */
#define TIMEOUT_BLOCK	3000	/* �f�[�^�u���b�N�]�� */

#define TIMEOUT_WATCHDATA		1000	/* ���܂ł��ϐ�f�[�^�����Ȃ� */
#define TIMEOUT_WATCHDATAREPLY	1000	/* ���܂ł��ϐ�f�[�^�ԐM�����Ȃ� */

#define TIMEOUT_WATCHDATAWAIT	4000	/* �ϐ�f�[�^�҂��ɂ���~���Ԍ��E */

#define WATCH_RESUME_INTERVAL	2000	/* �ϐ�ڑ����؂ꂽ���̍ėv���Ԋu */

const sockaddr_in NULL_ADDR = { 0, 0,						// family, port
								0, 0, 0, 0,					// addr
								0, 0, 0, 0, 0, 0, 0, 0 };	// zero

enum EPacketDataType
{
	Packet_Connect = 0,		/* �ڑ��v�� */
	Packet_ConnectReply,
	Packet_Ping,
	Packet_PingReply112_3,
	Packet_PingReplyLite120,

	Packet_Key,
	Packet_VSLoadCompleted,
	Packet_Suspend,
	Packet_SuspendReply,
	Packet_Resume,
	Packet_ResumeReply,

	Packet_Data,
	Packet_DataReply,

	Packet_WatchIn,			/* �ϐ�v�� */
	Packet_WatchInReply,
	Packet_WatchInRoot,		/* �z�M���ɑ΂��钼�ڊϐ�v�� */
	Packet_WatchInRootReply,
	Packet_WatchData,		/* �ϐ�ׂ̈̒f�ГI�ȃ��v�f�[�^�B�đ��͂��Ȃ������X�g�����ꍇ�A���񑗐M���ɏ�悹����� */
							/* ���܂�ɂ��ԐM�����Ȃ��ꍇ�͐ؒf�������̂Ƃ݂Ȃ��A�Ȍ�]�����Ȃ� */
	Packet_WatchDataReply,	/* �ϐ�f�[�^�̕ԐM�B�đ��͂��Ȃ� */
	Packet_PingReply,
	Packet_NodeAddr115_2,	/* �ꕔ�̃o�[�W�����Ŗ����Ɏg�p����Ă���̂ōė��p�֎~�I�I */
	Packet_NodeAddr,

	Packet_GalleryCount,		/* �ΐ�ғ��m�Ŋϐ�Ґ��𑗐M���� */
	Packet_GalleryCountForLobby,/* ���r�[�ɂ���m�[�h�Ɋϐ�̏���� */
	Packet_NameRequest,			/* UNKNOWN_NAME�ɑ΂��Ĕ��s�B���O�𓾂邽��ping���b�Z�[�W��v�� */
	Packet_DebugInfo,			/* SyncError�̌��������p */

	Packet_PingReply_BusyCasting,	/* 1.20-2����pingrep�̌y�ʉ� */
	Packet_PingReply_Busy,			/* 1.20-2����pingrep�̌y�ʉ� */
	Packet_PingReply_Watch,			/* 1.20-2����pingrep�̌y�ʉ� */
	Packet_PingReply_Idle,			/* 1.20-2����pingrep�̌y�ʉ� */
	Packet_Comment,					/* 1.20-2����R�����g��PingReply���番�� */

	Packet_CompWatchData,		/* ���k�ϐ�f�[�^ */
	Packet_CompWatchDataReply,	/* ���k�ϐ�f�[�^�̕ԐM */

	Packet_BattleInfoRequest,	/* �ΐ풆�̏��v�� */
	Packet_BattleInfo,			/* �ΐ풆�̏�� */
};

enum EBlockDataType
{
	Block_RandomTable = 0,
	Block_PlayerInfo,
	Block_KeySetting,
	Block_Palette,

	Block_NetLog = 124,
	Block_KeyLog = 125,
	Block_RndLog = 126,

	Block_TestData = 127,
};

enum EStateType
{
	State_Idle = 0,
	State_Busy,				// �ΐ풆
	State_NoResponse,
	State_Mismatch,
	State_VersionError,
	State_NotReady,
	State_Unknown,
	State_PingOver,			// �ϐ풆�ł�Ping����ȏ�Ȃ�PingOver�Ƃ���
	
	State_Watch,			// �ϐ풆
	State_Watch_Playable,	// �ϐ풆�����ΐ�v���ɉ�����
	State_Busy_Casting,		// �ΐ풆�Ŋϐ�\
	State_Busy_Casting_NG,	// �ΐ풆�Ŋϐ�\�����A�܂��L�����Z����
};

enum EValidFlag{
	VF_RANK		= 0x00000001,
	VF_WINS		= 0x00000002,
	VF_EX		= 0x00000004,
	VF_ROUND	= 0x00000008,
	VF_COUNT	= 0x00000010,
	VF_VERSION	= 0x00000020,
	VF_COMMENT	= 0x00000040,
	VF_CAST		= 0x00000080,
	VF_ID		= 0x00000100,
	VF_DENY		= 0x00000200,
};

#pragma pack(push)
#pragma pack(1)

typedef struct
{
	char	packetType;
	char	data[PACKETMAX_SIZE-1];
}SPacket_Unknown;

enum { BF_IDLE=0, BF_BUSY=1, BF_BUSY_CAST_OK=2, BF_BUSY_CAST_NG=3 };
enum { WF_WATCH=0x80, WF_INTRUSION=0x40 };

typedef struct
{
	enum { SIZE115=66, SIZE120=72, };
	enum {
		VF115 = VF_DENY | VF_RANK | VF_WINS | VF_EX | VF_ROUND | VF_COUNT | VF_VERSION | VF_CAST, // -16�ŌŒ�Ȃ̂�VF_CAST���L���ɂ��Ă���
		VF120 = VF_DENY | VF_RANK | VF_WINS | VF_EX | VF_ROUND | VF_COUNT | VF_VERSION | VF_CAST | VF_ID,
	};

	char	packetType;
	char	cid;

	DWORD	scriptCode;
	char	name[30];
	char	ver[10];
	char	mac[6];

	char	delay;
	char	ex;
	short	wins;
	char	rank;
	char	notready;
	char	ignoreSlow;
	char	round;
	bool	deny;
	bool	needDetail;
	int		gamecount;
	// 1.20
	char	hdid[4];		// mac�̑���
	BYTE	watchFlags;		// 7bit : watch
							// 6bit : allow intrusion (Setting)
	char	watchMaxNode;
}SPacket_Ping;

typedef struct
{
	enum { VF = VF_DENY | VF_RANK | VF_WINS | VF_EX | VF_ROUND | VF_COUNT | VF_COMMENT | VF_CAST }; // -16�ŌŒ�Ȃ̂�VF_CAST���L���ɂ��Ă���

	// 1.12-3�܂ł͂�������g�p
	char	packetType;
	char	cid;

	char	msg[256];

	char	delay;
	char	ex;
	short	wins;
	char	rank;
	char	notready;
	char	ignoreSlow;
	char	round;
	bool	deny;
	int		gamecount;
}SPacket_PingReply112_3;

typedef struct
{
	enum { SIZE115=281, SIZE120=285, };
	enum {
		VF115 = VF_DENY | VF_RANK | VF_WINS | VF_EX | VF_ROUND | VF_COMMENT | VF_ID,
		VF120 = VF_DENY | VF_RANK | VF_WINS | VF_EX | VF_ROUND | VF_COUNT | VF_COMMENT | VF_ID,
	};

	char	packetType;
	char	cid;

	// ver1.13���MAC�A�h���X�̏d����h������HD�̃V���A�����t������
	// ���o�[�W�����Ƃ̌݊�����邽�߁A����̃o�[�W�����𒲂ׂ����10byte���̃R�[�h�𑗂�
	// �R�[�h�̔�r�͏��Ȃ��ق��ɂ��킹��(HD�̃V���A����ffffffff�Ȃ烏�C���h�J�[�h�Ƃ���)
	char	id[10];
	char	msg[256];

	char	delay;
	char	ex;
	short	wins;
	char	rank;
	char	notready;
	char	ignoreSlow;
	char	round;
	bool	deny;
	// 1.20
	int		gamecount;
}SPacket_PingReply;

/* �l�b�g���[�N���ׂ��y�����邽�߁ASPacket_Ping::needDetail���I�t�̂Ƃ��͂�������g�p */
/* �ǂ���������ping����Ƃ��͈�ʂ�̏��𑗂��Ă��� */
typedef struct
{
	enum { SIZE115=3, SIZE120=98, };
	enum {
		VF115 = 0x00000000,
		VF120 = VF_DENY | VF_RANK | VF_WINS | VF_EX | VF_ROUND | VF_VERSION | VF_COUNT | VF_ID,
	};

	char	packetType;
	char	cid;
	char	busy;
	// 1.20
	char	name[2][30];	// �ΐ푊��̏��
	char	chara[2];		// �ΐ�L�����̏��

	char	id[10];
	char	ver[10];

	char	delay;
	char	ex;
	short	wins;
	char	rank;
	char	round;
	bool	deny;
	int		gamecount;	// �ϐ�ׂ̈̎������ʂɎg�p

	BYTE	watchFlags;	// 7bit : watch
						// 6bit : allow intrusion (Setting)
	char	watchMaxNode;
}SPacket_PingReplyLite120;

// 120-2����󋵕ʂɈقȂ郁�b�Z�[�W��Ԃ�
typedef struct
{
	enum { VF = VF_RANK | VF_WINS | VF_COUNT };

	char	packetType;
	char	cid;
	bool	casting;
	short	wins;
	char	rank;
	int		gamecount;		// �ϐ�ׂ̈̎������ʂɎg�p
}SPacket_PingReply_BusyCasting;

typedef struct
{
	enum { VF = 0x00000000 };

	char	packetType;
	char	cid;
}SPacket_PingReply_Busy;

typedef struct
{
	enum { VF = VF_DENY | VF_RANK | VF_WINS | VF_EX | VF_ROUND | VF_VERSION | VF_COUNT | VF_CAST | VF_ID };

	char	packetType;
	char	cid;

	char	id[10];
	char	ver[10];

	char	delay;
	char	ex;
	short	wins;
	char	rank;
	char	round;
	char	ignoreSlow;
	bool	deny;
	int		gamecount;		// �ϐ�ׂ̈̎������ʂɎg�p

	bool	allowIntrusion;
	char	watchMaxNode;
}SPacket_PingReply_Watch;

typedef struct
{
	enum { VF = VF_COMMENT };

	// ping��needDetail=true�̎��A
	// pingrep�Ɠ����^�C�~���O�ő���
	char	packetType;
	BYTE	size;
	char	msg[256];
}SPacket_Comment;

typedef struct
{
	enum { VF = VF_DENY | VF_RANK | VF_WINS | VF_EX | VF_ROUND | VF_VERSION | VF_COUNT | VF_CAST | VF_ID };

	char	packetType;
	char	cid;
	bool	notready;

	char	ver[10];
	char	id[10];

	char	delay;
	char	ex;
	short	wins;
	char	rank;
	char	round;
	char	ignoreSlow;
	bool	deny;
	int		gamecount;	// �ϐ�ׂ̈̎������ʂɎg�p
	
	bool	allowIntrusion;
	char	watchMaxNode;
}SPacket_PingReply_Idle;

typedef struct
{
	char	packetType;
	DWORD	time;
	struct
	{
		WORD	key;
		BYTE	syncChk;
	}cell[32];
}SPacket_Key;

typedef struct
{
	enum { SIZE120=2, SIZE120_2=4, };

	char	packetType;
	char	cid;
	// 1.20-2
	short	maxPacketSize;	// �u���b�N�]���ł̍ő�p�P�b�g�T�C�Y
}SPacket_Connect, SPacket_ConnectReply;

typedef struct
{
	char	packetType;
	char	cid;
}SPacket_VSLoadCompleted, SPacket_Suspend, SPacket_SuspendReply, SPacket_Resume, SPacket_ResumeReply, SPacket_NameRequest;

typedef struct
{
	enum { PACKET_HEADER_SIZE = 12, };

	char	packetType;
	char	type;
	int		seq;
	WORD	dataSize;
	DWORD	dataOffset;
	char	data[PACKETMAX_SIZE - PACKET_HEADER_SIZE];
}SPacket_Data;

typedef struct
{
	char	packetType;
	DWORD	seq;
}SPacket_DataReply;

// �z�M���֒��ڐڑ�����
typedef struct
{
	enum { SIZE120=6, SIZE120_2=13, };

	char	packetType;
	char	cid;
	int		dataOffset;		// �v���f�[�^�̐擪�o�C�g�i�r������v�������ꍇ������̂Łj
	// 1.20-2
	int		targetGameCount;// �z�M�����J�E���g
	char	format;			// �Ή��t�H�[�}�b�g(0|����`=RawData�̂݁A1=���k���T�|�[�g)
	short	maxPacketSize;	// �ő�p�P�b�g�T�C�Y
}SPacket_WatchInRoot;

typedef struct
{
	enum { SIZE120_FMT1=87, SIZE120_2_FMT1=87 };
	enum { SIZE120_FMT2=19, SIZE120_2_FMT2=27 };
	
	char		packetType;
	char		cid;
	char		accept;
	//sockaddr_in	myAddr;		// �����̃A�h���X�͒m���Ă���͂��Ȃ̂ő���Ȃ�
	sockaddr_in	enemyAddr;		// �q�ɑ���̃A�h���X��������

	union
	{
		struct
		{
			// accept == 1�̂Ƃ�
			char myName[30];		// �q�Ɏ����̖��O��������
			char enemyName[30];		// �q�ɑ���̖��O��������
			int  myGameCount;		// �q�Ɏ����̎���ID��������
			int  enemyGameCount;	// �q�ɑ���̎���ID��������
		}format1;
		struct
		{
			// 1.20-2�ȍ~��accept == 0�̂Ƃ�
			int  myGameCount;		// �q�Ɏ����̎���ID��������
			int  enemyGameCount;	// �q�ɑ���̎���ID��������
		}format2;
	}extra;
}SPacket_WatchInRootReply;

typedef struct
{
	enum { SIZE120=44, SIZE120_2=47, };

	char	packetType;
	char	cid;
	char	targetName[30];
	in_addr	targetIP;		// �z�M��IP �����������p����̂����烋�[�g��NAT���ł��ڑ��ł���悤�ɂ�����
	int		targetGameCount;// �z�M�����J�E���g
	int		dataOffset;		// �v���f�[�^�̐擪�o�C�g�i�r������v�������ꍇ������̂Łj
	// 1.20-2
	char	format;			// �Ή��t�H�[�}�b�g(0|����`=RawData�̂݁A1=���k���T�|�[�g)
	short	maxPacketSize;	// �ő�p�P�b�g�T�C�Y
}SPacket_WatchIn;

typedef struct
{
	char		packetType;
	char		cid;
	char		rootName[2][30];	// �q�ɔz�M���̖��O��������
	sockaddr_in	rootIP[2];			// �z�M���A�h���X��������
	int			rootGameCount[2];	// �z�M�����J�E���g
}SPacket_WatchInReply;

typedef struct
{
	enum {
		MINBUFFERSIZE = 64,
		MAXBUFFERSIZE = 512,
		PACKET_HEADER_SIZE = 8,// sizeof(packetType) + sizeof(offset) + sizeof(size) + sizeof(galleryCount)
	};
	char	packetType;
	DWORD	offset;
	WORD	size;
	BYTE	galleryCount;			/* ���̑ΐ�̑��ϐ�Ґ���z�M�҂���ʒm */
	char	data[MAXBUFFERSIZE];	/* ���ۂɂ͕K�v�ȕ��������� */
}SPacket_WatchData;

typedef struct
{
	char	packetType;
	BYTE	reserved;			/* ���g�p�i=0�j */
	DWORD	size;
	BYTE	childCount;			/* �����̎q�ϐ�Ґ���e�֕� */
}SPacket_WatchDataReply;

typedef struct
{
	enum {
		MINBUFFERSIZE = 64,
		MAXBUFFERSIZE = 512,
		PACKET_HEADER_SIZE = 12,
	};
	char	packetType;
	WORD	compblock_offset;		/* ���k�f�[�^�u���b�N�̃I�t�Z�b�g�i����]�������f�[�^�̈��k�f�[�^��̎n�_�j */
	WORD	compblock_size;			/* ���k�f�[�^�u���b�N�̃T�C�Y */
	DWORD	compall_offset;			/* ���k�f�[�^�̑S�̂̃I�t�Z�b�g�i���f�[�^�̉������爳�k�����̂��j */
	WORD	compall_size;			/* ���k�f�[�^�̑S�̂̃T�C�Y */
	BYTE	galleryCount;			/* ���̑ΐ�̑��ϐ�Ґ���z�M�҂���ʒm */
	char	data[MAXBUFFERSIZE];	/* ���ۂɂ͕K�v�ȕ��������� */
}SPacket_CompWatchData;

typedef struct
{
	char	packetType;
	BYTE	reserved;			/* ���g�p�i=0�j */
	WORD	compsize;			/* ��M�ψ��k�f�[�^�T�C�Y */
	DWORD	rawsize;			/* ���f�[�^�Ƃ��ēW�J�ł����T�C�Y��� */
	BYTE	childCount;			/* �����̎q�ϐ�Ґ���e�֕� */
}SPacket_CompWatchDataReply;

//typedef struct
//{
//	char	packetType;
//	char	name[30];
//	char	addr[32];
//}SPacket_NodeAddr115_2;

// �ʐM�\��IP:Port�𑼂̃m�[�h�ɓ`����
typedef struct
{
	char	packetType;
	DWORD	scriptCode;
	char	name[30];
	char	addr[32];
}SPacket_NodeAddr;

// �ΐ풆�̃m�[�h�����҂̊ϐ�Ґ��𓊂�����
typedef struct
{
	char	packetType;
	int		galleryCount;
}SPacket_GalleryCount;

// �ϐ�҂����r�[�����Ping�ɑ΂���ԐM�Ƃ��đΐ�̊ϐ�Ґ���񍐂���
typedef struct
{
	enum { SIZE120=73, SIZE120_2=81, };

	char	packetType;
	char	name[2][30];
	DWORD	ip[2];
	int		galleryCount;
	// 1.20-2
	int		gameCount[2];	// �������ʗp�ɒǉ�
}SPacket_GalleryCountForLobby;

typedef struct
{
	char	packetType;
	char	cpu_name[49];
	DWORD	cpu_eax;
	DWORD	cpu_edx;
	DWORD	cpu_ecx;
	WORD	fcw;
	char	analog[2];
	char	ggmode;
}SPacket_DebugInfo;

typedef struct
{
	char	packetType;
	char	targetName[30];	// �z�M�Җ�
	in_addr	targetIP;		// �z�M��IP
	int		targetGameCount;// �z�M�����J�E���g
}SPacket_BattleInfoRequest;

typedef struct
{
	char	packetType;
	char	name[2][30];
	in_addr	ip[2];
	int		gamecount[2];
	char	chara[2];
}SPacket_BattleInfo;

typedef struct
{
	char	nametrip[30];
	char	rank;
	char	round;
	WORD	wins;
	WORD	oldcs;
	char	ex;
}SBlock_PlayerInfo;

#pragma pack(pop)

class CWatcher
{
public:
	CWatcher(void) { m_compData = new char[MAX_COMPWATCHDATASIZE]; }
	~CWatcher(void) { delete[] m_compData; }

	inline bool isActive(void) { return m_remoteAddr.sin_port != 0; }

	void init(void)
	{
		m_remoteAddr = NULL_ADDR;
		m_sendSize = 0;
		m_sendTime = 0xffffffff;
		m_childCount = 0;
		m_supportedFormat = 0;
		m_compOffset = 0;
		m_compSize = 0;
		m_compSendSize = 0;
	}

public:
	sockaddr_in	m_remoteAddr;
	DWORD		m_sendSize;			// ���M�ς݃f�[�^�T�C�Y
	int			m_childCount;		// �q�̐������ϐ�Ґ���m�邽�߂Ɏq����\�������e�ɕ񍐂���
	DWORD		m_sendTime;			// �Ō��watchData�𑗐M�������� watchDataReply���󂯂�ƃN���A����
	char		m_supportedFormat;	// �T�|�[�g�t�H�[�}�b�g(0=RawData�̂݁A1=���k�T�|�[�g)
	short		m_maxPacketSize;	// �ő�p�P�b�g�T�C�Y

// ���k�f�[�^�̃T�|�[�g�i�f�[�^�T�C�Y��128�ȏゾ�����爳�k�f�[�^���g�p����j
	char*		m_compData;			// ���M���I���܂ň��k�f�[�^��ێ����Ă���
	int			m_compOffset;		// ���f�[�^�ɑ΂��鈳�k�f�[�^�̃I�t�Z�b�g
	int			m_compSize;			// ���k�f�[�^�S�̂̃T�C�Y
	int			m_compSendSize;		// ���M�ς݈��k�f�[�^�̃T�C�Y
};

class CNetMgr
{
public:
	CNetMgr(void);
	~CNetMgr(void);
	
	bool init(int p_port, int p_delay, bool p_useLobby);
	void startThread(void);
	void stopThread(void);

	void connect(void);
	void disconnect(char* p_cause);
	void resume(void);
	void suspend(void);

	void setErrMsg(char* p_msg);

	char*		getStringFromAddr(sockaddr_in* p_addr, char* p_output);
	sockaddr_in getAddrFromString(char* p_str);

	bool watch(char* p_targetName, sockaddr_in* p_targetAddr, int p_targetGameCount, bool p_blockingMode);
	int  findFreeWatchEntry(sockaddr_in* p_addr);
	int  getChildWatcherCount(void);

	bool send_connect(sockaddr_in* p_addr);
	void send_connectReply(void);
	void send_key(int p_time);
	bool send_watchInRoot(sockaddr_in* p_addr, int p_targetGameCount, bool& p_success);
	void send_watchInRootReply(bool p_accept);
	bool send_watchIn(char* p_targetName, sockaddr_in* p_targetIP, int p_targetGameCount);
	void send_watchInReply(void);
	void send_watchData(int p_idx);
	void send_watchDataReply(int p_size);
	void send_compWatchDataReply(int p_compsize, int p_rawsize);
	bool send_ping(sockaddr_in* p_addr, int p_selNodeIdx);
	void send_pingReply120(bool p_needDetail, bool p_deny, bool p_underV113);
	void send_pingReply(bool p_deny);
	void send_comment(void);
	void send_vsLoadCompleted(void);
	bool send_suspend(void);
	void send_suspendReply(void);
	bool send_resume(void);
	void send_resumeReply(void);
	void send_dataReply(int p_seq);
	void send_nodeaddr115_3(sockaddr_in* p_addr, class CNode* p_node);
	void send_galleryCount(void);
	void send_galleryCountForLobby(void);
	void send_nameRequest(sockaddr_in* p_addr);
	void send_debugInfo(void);

	bool send_battleInfoRequest(char* p_targetName, sockaddr_in* p_targetIP, int p_targetGameCount);
	void send_battleInfo(char* p_name1, char* p_name2, DWORD p_ip1, DWORD p_ip2, DWORD p_gamecount1, DWORD p_gamecount2, char p_chara1, char p_chara2);

	bool sendDataBlock(char p_type, char* p_data, int p_dataSize, int p_timeout);
	bool recvDataBlock(char p_type, char* p_data, int p_dataSize, int p_timeout);

	void initWatchVars(void);

private:
	bool talking(void);

	int udpsend(sockaddr_in* p_addr, char* p_data, int p_dataSize);
	int udprecv(char* p_buf, int p_bufSize);

#if USE_TCP
	int tcpsend(char* p_data, int p_dataSize, int p_timeout);
	int tcprecv(char* p_buf, int p_bufSize, int p_timeout);
#endif

	friend DWORD WINAPI _recvThreadProc(LPVOID lpParameter);
	friend DWORD WINAPI _lobbyThreadProc(LPVOID lpParameter);

private:
	volatile bool	m_quitApp;
	volatile bool	m_recvThread_end;
	volatile bool	m_lobbyThread_end;
	HANDLE			m_recvThread;
	HANDLE			m_lobbyThread;

public:
	SPacket_Unknown	m_buf;

	sockaddr_in	m_remoteAddr_recv;		/* recvfrom���ňꎞ�I�ȑ���̃A�h���X */
	sockaddr_in	m_remoteAddr_active;	/* connection������̌Œ�I�ȑ���̃A�h���X */
										/* watch�̎�M���A�h���X�������Ɋi�[����� */
	SOCKET		m_udpSocket;
	SOCKET		m_tcpSocket;

	bool		m_networkEnable;
	bool		m_connect;		/* �ΐ풆���H */
	int			m_queueSize;
	int			m_delay;
	int			m_playSide;		/* 1=1P, 2=2P, 3=Watch */
	DWORD		m_time;
	DWORD*		m_key;
	WORD*		m_syncChk;		/* �����`�F�b�N */

	bool		m_suspend;		/* �L�[�̓��������Ȃ� */
	int			m_suspendFrame;	/* Suspend���Ă��鎞�� */
	int			m_vsloadFrame;	/* vsload�̌o�߃t���[�� */
	int			m_totalSlow;	/* �l�b�g���[�N�̓s���Œ�~�������ԁB���Ɏg�����͖��� */
	int			m_lobbyFrame;
	bool		m_initKeySet;	/* �L�[�ݒ肪����M�ς��H */

	int			m_enMaxPacketSize;	/* ����̍ő�p�P�b�g�T�C�Y */

	/* ���肩��̃f�[�^�҂��t���O */
	volatile bool	m_waitingConnectReply;
	volatile bool	m_waitingSuspendReply;
	volatile bool	m_waitingResumeReply;
	volatile bool	m_waitingData;
	volatile bool	m_waitingDataReply;
	volatile bool	m_waitingWatchInReply;
	volatile char	m_waitingWatchInRootReply;	/* �ҋ@���Ȃ� = 0, �ҋ@�� = 1, �ڑ��� = 2, �ڑ��s�� = 3 */
	volatile bool	m_waitingBattleInfoRequestReply;

	enum { EWIRReply_Idle = 0, EWIRReply_Wait, EWIRReply_Success, EWIRReply_Fail, };

	volatile int	m_waitingDataType;		/* Packet_Data�R�}���h�Ŏ�M�҂��̃f�[�^�^�C�v */

	volatile bool	m_recvSuspend;			/* �����Suspend�� */
	volatile bool	m_recvVSLoadCompleted;	/* ����̃��[�h�� */
	
	char*		m_recvDataPtr;			/* Packet_Data�R�}���h�ɂ���M����o�b�t�@�|�C���^ */
	DWORD		m_recvDataSize;			/* Packet_Data�R�}���h�ɂ���M����o�b�t�@�T�C�Y */

	int			m_sendDataSeq;	/* ���M�f�[�^�V�[�P���X�ԍ� */
	int			m_recvDataSeq;	/* ��M�ς݃f�[�^�V�[�P���X�ԍ� */
								/* connect�̂��тɃ��Z�b�g���� */
								/* ���̒l�ȉ��̃f�[�^�͎�M�ςȂ̂Ŏ󂯎��Ȃ� */

	CRITICAL_SECTION	m_csKey;
	CRITICAL_SECTION	m_csNode;
	CRITICAL_SECTION	m_csWatch;

	char		m_errMsg[1024];
	int			m_errMsgTime;

	// for watch client
	bool		m_watch;				// �ϐ�N���C�A���g�Ƃ��ē��쒆
	bool		m_1stCaster;			// �ꎟ�z�M�҂��H
	bool		m_watchRecvComplete;	// �f�[�^��M���������Ă��邩�H
	char		m_watchRootName[2][30];	// �z�M���̖��O
	sockaddr_in	m_watchRootAddr[2];		// �z�M���̃A�h���X
	int			m_watchRootGameCount[2];// �z�M���̎���ID
	int			m_watchRecvSize;		// ��M�σf�[�^�T�C�Y
	DWORD		m_lastWatchDataTime;	// �Ō��Packet_WatchData���󂯎��������
	int			m_totalGalleryCount;	// �����ϐ�Ґ��i�ϐ풆�ɔz�M�҂��ΐ�I������Ƃ��̂܂܍X�V����Ȃ��Ȃ邪�A����͎d�l�Ƃ���j
	int			m_watchFailCount;		// �A���Ŏ����ϐ�v���Ɏ��s������
	sockaddr_in	m_watchParentAddr;		// �e�̃A�h���X

	// ���k�f�[�^�̃T�|�[�g�i��M�f�[�^�T�C�Y��128�ȏゾ�����爳�k�f�[�^���g�p����j
	char*		m_watchRecvCompData;	// ���ׂĎ�M���I���܂ň��k�f�[�^��ێ����Ă���
	int			m_watchRecvCompSize;	// ��M�ς݈��k�f�[�^�̃T�C�Y

	// for watch server
	CWatcher	m_watcher[WATCH_MAX_CHILD];
	int			m_recvGalleryCount;		// ���葤�̊ϐ�Ґ�
};

// extern
extern CNetMgr* g_netMgr;
