#pragma once


#include "resource.h"

// CExtraOptionDlg �_�C�A���O

class CExtraOptionDlg : public CDialog
{
	DECLARE_DYNAMIC(CExtraOptionDlg)

public:
	CExtraOptionDlg(CWnd* pParent = NULL);   // �W���R���X�g���N�^�[
	virtual ~CExtraOptionDlg();

// �_�C�A���O �f�[�^
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_EXTRAOPTIONDLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g

	DECLARE_MESSAGE_MAP()

	class CggncfgDlg*	m_ggncfgdlg;
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedPks();
	afx_msg void OnBnClickedPkhs();
};
