#pragma once

//******************************************************************
// include
//******************************************************************
#define WIN32_LEAN_AND_MEAN
#ifndef MANPUKU
#define WINVER			0x0501
#define _WIN32_WINNT	0x0501
#endif // #ifndef MANPUKU
#include <windows.h>
#include <mmsystem.h>
#include <stdarg.h>
#include <stdlib.h>

#include "dynArray.h"
#include "cpuid.h"
#include "settingfile.h"

//******************************************************************
// const
//******************************************************************

#if TESTER
	#define DEBUG_SYNCERROR_INQUIRY 0
	#define	DEBUG_OUTPUT_RND	0
	#define DEBUG_OUTPUT_KEY	0
	#define DEBUG_OUTPUT_NET	1
	#define DEBUG_OUTPUT_LOG	0
#else
	#ifdef _DEBUG
		#define DEBUG_INQUIRY_MODE	1	// 調査用の機能などを有効にする。リリース版とは対戦しないこと！
		#define	DEBUG_OUTPUT_RND	0	// 乱数に関するログを出す
		#define DEBUG_OUTPUT_KEY	0	// キー入力に関するログを出す
		#define DEBUG_OUTPUT_NET	1	// 通信に関するログを出す
		#define DEBUG_OUTPUT_LOG	1	// 一般的なログを出す
	#else
		#define DEBUG_SYNCERROR_INQUIRY 0
		#define	DEBUG_OUTPUT_RND	0
		#define DEBUG_OUTPUT_KEY	0
		#define DEBUG_OUTPUT_NET	0
		#define DEBUG_OUTPUT_LOG	1
	#endif
#endif

#define DEFAULT_CW 0x027f

#define	DEFAULT_DELAY	2
#define	DEFAULT_PORT	11563

#define INPUT_SYNCERROR		0xfffffffe
#define INPUT_DISCONNECT	0xfffffffd
#define INPUT_COMPLETE		0xfffffffc
//#define INPUT_SOFTRESET	0x090f
//#define INPUT_NONE		0xffff

#define MAXREPSIZE		(60 * 60 * 30)	/* 約30分あれば十分 */

#define PALCOUNT	12
#define PALLEN		260

#define	ENTERLOBBY_INTERVAL	60000 // 60sec

#define DATVERSION	120
#define DATNAME		"ggn_setting.dat"

enum
{
	FONT_W			= 400,
	FONT_AA			= 1,
	FONT_W_SHADOW	= 800,
	FONT_AA_SHADOW	= 4,
};

const BYTE INVALID_MID[10]	= { 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0xff, 0xff };

#define UNKNOWN_NAME "@UnknownName@"

enum
{
	CID_SOL = 0,
	CID_KY,
	CID_MAY,
	CID_MILLIA,
	CID_EDDIE,
	CID_POTEMKIN,
	CID_CHIPP,
	CID_FAUST,
	CID_AXL,
	CID_BAIKEN,
	CID_ANJI,
	CID_VENOM,
	CID_JOHNNY,
	CID_JAM,
	CID_TESTAMENT,
	CID_DIZZY,
	CID_BRIDGET,
	CID_SLAYER,
	CID_ZAPPA,
	CID_INO,
	CID_ROBOKY,
	CID_KLIFF,
	CID_JUSTICE,
	CHARACOUNT,
};

enum
{
	SORT_NAME_A = 0,
	SORT_NAME_D,
	SORT_RANK_A,
	SORT_RANK_D,
	SORT_WIN_A,
	SORT_WIN_D,
	SORT_COUNT_A,
	SORT_COUNT_D,
	SORT_PING,
	SORT_EX,
	SORT_WATCH,
	SORT_STATUS,
	SORTTYPECOUNT,
};

enum ERank
{
	Rank_S = 0,
	Rank_A,
	Rank_B,
	Rank_C,
	Rank_D,
	Rank_E,
	Rank_F,
	RankCount,
};

//******************************************************************
// macro
//******************************************************************

#define __mbschr(p1, p2) (char*)_mbschr((unsigned char*)p1, (unsigned int)p2)
#define __strncpy(p1, p2, p3) { strncpy(p1, p2, p3); p1[p3] = '\0'; }

#define ENTERCS(x)	{\
	/*DBGOUT_LOG("st EnterCS %s:%d\n", __FILE__, __LINE__);*/\
	EnterCriticalSection(x);\
	/*DBGOUT_NET("ed EnterCS %s:%d\n", __FILE__, __LINE__);*/\
}
#define LEAVECS(x)	{\
	/*DBGOUT_NET("st LeaveCS %s:%d\n", __FILE__, __LINE__);*/\
	LeaveCriticalSection(x);\
	/*DBGOUT_NET("ed LeaveCS %s:%d\n", __FILE__, __LINE__);*/\
}

#define CHR_HEX2INT(x) ((x<'a')?x-'0':10+x-'a')

//******************************************************************
// structure
//******************************************************************
#pragma pack(push)
#pragma pack(1)

typedef struct
{
	int		battleStage;
	int		round;
	int		time;

	char	name1P[20];
	char	rank1P;
	WORD	wins1P;
	WORD	chara1P;
	char	ex1P;
	char	voice1P;

	char	name2P[20];
	char	rank2P;
	WORD	wins2P;
	WORD	chara2P;
	char	ex2P;
	char	voice2P;

	char	keySetting1P[0x18];
	char	keySetting2P[0x18];
	char	randTable[0x9c4];
	char	palette1P[0x410];
	char	palette2P[0x410];
	DWORD	inputData[MAXREPSIZE];
}ReplayFile_v109;

typedef struct
{
	int		battleStage;
	int		round;
	int		time;

	char	name1P[30];
	char	rank1P;
	WORD	wins1P;
	WORD	chara1P;
	char	ex1P;
	char	voice1P;

	char	name2P[30];
	char	rank2P;
	WORD	wins2P;
	WORD	chara2P;
	char	ex2P;
	char	voice2P;

	char	keySetting1P[0x18];
	char	keySetting2P[0x18];
	char	randTable[0x9c4];
	char	palette1P[0x410];
	char	palette2P[0x410];
	DWORD	inputData[MAXREPSIZE];
}ReplayFile;

#define REPLAY_HEADER_SIZE (sizeof(ReplayFile) - sizeof(DWORD)*MAXREPSIZE)

typedef struct
{
	char	fname[256];
	bool	dir;
	WORD	ver;
}ReplayItem;

class CVsNetModeInfo
{
public:
	CVsNetModeInfo(void)
	{
		m_selectItemIdx	= 0;
		m_itemPerPage	= 16;
		m_dispItemHead	= 0;
		m_sortType		= SORT_STATUS;
		
		m_menu_cursor = 0;
		m_menu_visible = false;
	}
	~CVsNetModeInfo(void) {}

	int	m_selectItemIdx;
	int	m_itemPerPage;
	int	m_dispItemHead;
	int	m_sortType;

	int  m_menu_cursor;
	bool m_menu_visible;
};

class CReplayModeInfo
{
public:
	CReplayModeInfo(void)
	{
		m_selectItemIdx	= 0;
		m_itemPerPage	= 16;
		m_dispItemHead	= 0;
		
		m_repRecording	= false;
		m_frameCount	= 0;
		m_format		= -1;
		m_playing		= false;
		strcpy(m_curdir, "REPLAY");
	}
	~CReplayModeInfo(void)
	{
		m_itemlist.deleteAll();
	}

	int		m_selectItemIdx;
	int		m_itemPerPage;
	int		m_dispItemHead;

	TDynArray<ReplayItem*>	m_itemlist;
	char					m_curdir[256];
	
	int						m_level;
	TDynArray<int>			m_stackSelect;
	TDynArray<int>			m_stackPageHead;

	bool			m_playing;
	int				m_skipFrame;

	ReplayFile		m_data;
	bool			m_repRecording;		// 115-3以前はm_frameCount >= 0で記録するかどうかを判断していたがそれをやめる
	int				m_frameCount;
	int				m_format;
};

class EnemyInfo
{
public:
	EnemyInfo(void) { clear(); }
	~EnemyInfo(void) {}

	void clear(void)
	{
		memset(m_name, 0, 30);
		memset(m_palette, 0, PALLEN);
		m_ex = 0;
		m_rank = -1;
		m_wins = 0;
		m_watchMaxNode = 0;
		m_gameCount = 0;
	}
	char	m_name[30];
	int		m_rank;
	WORD	m_wins;
	char	m_ex;
	char	m_watchMaxNode;
	int		m_gameCount;
	DWORD	m_palette[PALLEN];
};

typedef struct
{
	char m_fontName[256];
	int  m_fontSize;
	bool m_fontAntialias;

	int   m_posx;
	int   m_posy;
	float m_zoomx;
	float m_zoomy;

	int  m_recvThreadPriority;
	int  m_recvThreadInterval;
	int  m_recvThreadMethod;
	int  m_maxPacketSize;
	//int  m_watchDataFrequency;
	bool m_ignoreSlowConnections;

#ifdef MANPUKU
	int m_AutoReadServerInterval;
	int m_AutoReadServerSortWait;
	char m_NodeDisplayMode;
#endif // #ifdef MANPUKU

#if _DEBUG // データ参照ディレクトリ
	char m_dataDir[256];
#endif
}IniFileInfo;

#pragma pack(pop)

//******************************************************************
// externs
//******************************************************************
extern CVsNetModeInfo		g_vsnet;
extern CReplayModeInfo		g_replay;
extern EnemyInfo			g_enemyInfo;
extern SettingInfo			g_setting;
//extern CD3DFont*		g_d3dfont;
//extern LPDIRECT3DDEVICE8	g_d3dDev;
//extern CIcon*				g_dirIcon;
//extern DWORD*				g_myPalette[CHARACOUNT][PALCOUNT];
//extern CSharedMemory*		g_smPallette;
extern IniFileInfo			g_iniFileInfo;

//extern HINSTANCE			g_dllInst;
extern char					g_machineID[10];
extern DWORD				g_scriptCode;
//extern DWORD				g_startBattleTime;
//extern WORD				g_oldCS;
extern CCpuID				g_cpuid;
extern bool					g_ignoreWatchIn;

//-------------------------------------------------------debug
extern CRITICAL_SECTION		g_csLogOut;
extern char*				g_netLog;
extern char*				g_keyLog;
//extern char*				g_rndLog;
//extern char				g_syncErrLog[10][256];
extern char					g_moduleDir[256];

extern char g_charaNames[CHARACOUNT][256];

//******************************************************************
// proto types
//******************************************************************
void getNameTrip(char* p_str);
bool useLobbyServer(void);
void onDisconnect(char* p_cause);
bool idcmp(const BYTE* p_id1, const BYTE* p_id2);
void convertModulePath(char* p_out, char* p_in);

DWORD getGGXXMODE2(void);

//-------------------------------debug
void DBGOUT_NET(char* fmt, ...);
void DBGOUT_LOG(char* fmt, ...);
void WRITE_DEBUGLOG(char* p_cause);
void WRITE_REPLAY_RAWDATA(int p_size);


#ifdef MANPUKU

extern char g_OrgWatchBroadcast;

#define DEFAULT_ARS_INTERVAL 60
#define DEFAULT_ARS_SORT_WAIT 1000

#define	LogoSkipAddr	(BYTE *)0x004A6F96
#define LogoSkipSize	1
#define LogoSkipOrgVal	0x74
#define LogoSkipRewVal	0xEB


#define MES( Str )	MessageBox( NULL, Str, TEXT( "Press the ESC or ENTER to close." ), MB_OK | MB_ICONWARNING )
#define ACSERR	TEXT( "アクセス保護の変更ができませんでした" )

template < class T, class T2 > bool RewValue( T addr, BYTE size, T2 value )
{
	DWORD flOldProtect;
	if( VirtualProtect( addr, size, PAGE_READWRITE, &flOldProtect ) )
	{
		*addr = value;
		if(! VirtualProtect( addr, size, flOldProtect, &flOldProtect ) )
		{
			MES( ACSERR );
			return true;
		}
	}
	else
	{
		MES( ACSERR );
		return true;
	}
	return false;
}


#define EX2HitAddr (WORD *)0x0046D472
#define EX2HitSize	2
#define EX2HitOrgVal	0x39E8
#define EX2HitRewVal	0x03EB

#define EX2GuardFix1Addr (BYTE *)0x0044E093
#define EX2GuardFix1Size	1
#define EX2GuardFix1OrgVal	0x85
#define EX2GuardFix1RewVal	0xB8

#define EX2GuardFix2Addr (DWORD *)( 0x0044E093 + 1 )
#define EX2GuardFix2Size	4
#define EX2GuardFix2OrgVal	0x5F956815
#define EX2GuardFix2RewVal	0x0000FFE0

#define EX2GuardFix3Addr (WORD *)( 0x0044E093 + 1 + 4 )
#define EX2GuardFix3Size	2
#define EX2GuardFix3OrgVal	0x7400
#define EX2GuardFix3RewVal	0x2CEB

void Ex2Fix( bool b );


extern bool bKeyConfigHook;
extern bool bKeyConfigFlag;

#define KeyConfigHookAddr1	0x0045B388
#define KeyConfigHookAddr2	0x0045B3CB

#define KeyConfigDisplayModifyRewAddr	0x0045AF83
#define KeyConfigDisplayModifyOrgVal	0x00566088
/*
#define KeyConfigExchangeSize	1
#define KeyConfigExchangeOrgVal	0x4c
#define KeyConfigExchangeRewVal	0x4e
#define KeyConfigExchangeRewAddr1	(BYTE *)0x0044BF26
#define KeyConfigExchangeRewAddr2	(BYTE *)0x0044BE8B
#define KeyConfigExchangeRewAddr3	(BYTE *)0x0044C103
#define KeyConfigExchangeRewAddr4	(BYTE *)0x0044C063
#define KeyConfigExchangeRewAddr5	(BYTE *)0x0044BFC3
*/





#endif // #ifdef MANPUKU
