// ggncfg.h : PROJECT_NAME �A�v���P�[�V�����̃��C�� �w�b�_�[ �t�@�C���ł��B
//

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// ���C�� �V���{��


// CggncfgApp:
// ���̃N���X�̎����ɂ��ẮAggncfg.cpp ���Q�Ƃ��Ă��������B
//

class CggncfgApp : public CWinApp
{
public:
	CggncfgApp();

// �I�[�o�[���C�h
	public:
	virtual BOOL InitInstance();

// ����

	DECLARE_MESSAGE_MAP()
};

extern CggncfgApp theApp;
