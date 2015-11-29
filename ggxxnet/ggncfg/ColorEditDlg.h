#pragma once


// CColorEditDlg �_�C�A���O

class CColorEditDlg : public CDialog
{
	DECLARE_DYNAMIC(CColorEditDlg)

public:
	CColorEditDlg(int p_x, int p_y, CWnd* pParent = NULL);   // �W���R���X�g���N�^
	virtual ~CColorEditDlg();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_COLOR };

	void updateColor(void);

	afx_msg void OnEnChangeBe();
	afx_msg void OnEnChangeGe();
	afx_msg void OnEnChangeRe();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedOk();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();

	CggncfgDlg*	m_ggncfgdlg;
	int			m_posx;
	int			m_posy;
	DWORD		m_oldColor;
	int			m_selectPaletteIdx;
};
