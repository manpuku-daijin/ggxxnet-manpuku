#pragma once

//******************************************************************
// structures
//******************************************************************
#pragma pack(push)
#pragma pack(1)

typedef struct
{
	char	scriptAddress[256];
	char	userName[11];
	char	trip[11];
	char	enableNet;
	WORD	port;
	char	delay;
	short	wait;

	char	useEx;

	char	dispInvCombo;
	char	syncCheck;
	WORD	wins;
	char	rank;
	char	score[20];
}SettingInfo_v100;// ver1.00�`1.01

typedef struct
{
	char	scriptAddress[256];
	char	userName[11];
	char	trip[11];
	char	enableNet;
	WORD	port;
	char	delay;

	char	ignoreMisNode;
	short	wait;

	char	useEx;

	char	dispInvCombo;
	char	syncCheck;
	WORD	wins;
	char	rank;
	char	score[20];
}SettingInfo_v102;// ver1.02�`1.04

typedef struct
{
	char	scriptAddress[256];
	char	userName[11];
	char	trip[11];
	char	enableNet;
	WORD	port;
	char	delay;

	char	ignoreMisNode;
	char	ignoreSlow;
	short	wait;

	char	useEx;

	char	dispInvCombo;
	char	syncCheck;
	WORD	wins;
	char	rank;
	char	score[20];
	char	reserved[64];
}SettingInfo_v105;// ver1.05

typedef struct
{
	char	scriptAddress[256];
	char	addressList[10][256];

	char	userName[11];
	char	trip[11];
	char	enableNet;
	WORD	port;
	char	delay;

	char	ignoreMisNode;
	char	ignoreSlow;
	short	wait;

	char	useEx;

	char	dispInvCombo;
	char	syncCheck;
	WORD	wins;
	char	rank;
	char	score[20];
	char	reserved[64];
}SettingInfo_v106;// ver1.06�`1.09-3

typedef struct
{
	int		ver;
	char	scriptAddress[256];
	char	addressList[10][256];

	char	userName[41];	// 40byte�����邪���ۂɂ�20byte�܂�
	char	trip[11];
	char	enableNet;
	WORD	port;
	char	delay;

	char	ignoreMisNode;
	char	ignoreSlow;
	short	wait;

	char	useEx;

	char	dispInvCombo;
	char	showfps;
	WORD	wins;
	char	rank;
	int		score;
	int		totalBattle;
	int		totalWin;
	int		totalLose;
	int		totalDraw;
	int		totalError;
	int		slowRate;	/* �����������@�펞60fps������ꍇ100%�ɂȂ� */
	char	rounds;
	char	msg[256];
}SettingInfo_v110;	// ver1.10�`1.15-3

typedef struct
{
	int		ver;
	char	scriptAddress[256];
	char	addressList[10][256];

	char	userName[41];	// 40byte�����邪���ۂɂ�20byte�܂�
	char	trip[11];
	char	enableNet;
	WORD	port;
	char	delay;

	char	ignoreMisNode;
	char	ignoreSlow;
	short	wait;

	char	useEx;

	char	dispInvCombo;
	char	showfps;
	WORD	wins;
	char	rank;
	int		score;
	int		totalBattle;
	int		totalWin;
	int		totalLose;
	int		totalDraw;
	int		totalError;
	int		slowRate;	/* �����������@�펞60fps������ꍇ100%�ɂȂ� */
	char	rounds;
	char	msg[256];
	char	watchBroadcast;		// �z�M����
	char	watchIntrusion;		// ��������
	char	watchSaveReplay;	// �ϐ킵�����v���C��ۑ����邩�H
	char	watchMaxNodes;		// 1���z�M��t��
}SettingInfo;	// ver1.20�`

#pragma pack(pop)

//******************************************************************
// proto types
//******************************************************************
void readSettingFile(void);
void writeSettingFile(void);
