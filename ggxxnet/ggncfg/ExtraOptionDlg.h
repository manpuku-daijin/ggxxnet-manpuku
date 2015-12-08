#pragma once


#include "resource.h"

// CExtraOptionDlg ダイアログ

class CExtraOptionDlg : public CDialog
{
	DECLARE_DYNAMIC(CExtraOptionDlg)

public:
	CExtraOptionDlg(CWnd* pParent = NULL);   // 標準コンストラクター
	virtual ~CExtraOptionDlg();

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_EXTRAOPTIONDLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()

	class CggncfgDlg*	m_ggncfgdlg;
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedPks();
	afx_msg void OnBnClickedPkhs();
};
