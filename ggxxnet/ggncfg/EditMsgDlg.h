#pragma once

#include "resource.h"

// CEditMsgDlg �_�C�A���O
class CEditMsgDlg : public CDialog
{
	DECLARE_DYNAMIC(CEditMsgDlg)

public:
	CEditMsgDlg(CWnd* pParent = NULL);
	virtual ~CEditMsgDlg();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_EDITMSGDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g

	DECLARE_MESSAGE_MAP()
public:
	class CggncfgDlg*	m_ggncfgdlg;

	afx_msg void OnBnClickedOk();
public:
	afx_msg void OnEnChangeEdit1();
public:
	virtual BOOL OnInitDialog();
};
