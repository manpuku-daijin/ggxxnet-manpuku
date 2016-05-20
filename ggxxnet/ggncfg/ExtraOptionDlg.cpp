// ExtraOptionDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "ExtraOptionDlg.h"
#include "afxdialogex.h"
#include "ggncfgDlg.h"


// CExtraOptionDlg ダイアログ

IMPLEMENT_DYNAMIC(CExtraOptionDlg, CDialog)

CExtraOptionDlg::CExtraOptionDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_EXTRAOPTIONDLG, pParent)
{
	m_ggncfgdlg = (CggncfgDlg*)pParent;
}

CExtraOptionDlg::~CExtraOptionDlg()
{
}

void CExtraOptionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CExtraOptionDlg, CDialog)
	ON_BN_CLICKED( IDOK, &CExtraOptionDlg::OnBnClickedOk )
	ON_BN_CLICKED( IDC_PKS, &CExtraOptionDlg::OnBnClickedPks )
	ON_BN_CLICKED( IDC_PKHS, &CExtraOptionDlg::OnBnClickedPkhs )
END_MESSAGE_MAP()


// CExtraOptionDlg メッセージ ハンドラー


BOOL CExtraOptionDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CheckDlgButton( IDC_USE_EX2, m_ggncfgdlg->m_useEx2 );
	CheckDlgButton( IDC_USE_KEY_HOOK, m_ggncfgdlg->m_useKeyHook );
	CheckRadioButton( IDC_PKS, IDC_PKHS, IDC_PKS + m_ggncfgdlg->m_KeyChangeFlag );

	CheckDlgButton( IDC_USE_VER_DENY, m_ggncfgdlg->m_useVersionDeny );

	CComboBox *cvd = (CComboBox*)GetDlgItem( IDC_COMBO_VER_DENY );
	cvd->ResetContent();
	cvd->AddString( "1.24" );

	cvd->AddString( "1.23" );

	cvd->AddString( "1.22-5" );
	cvd->AddString( "1.22-4" );
	cvd->AddString( "1.22-3" );
	cvd->AddString( "1.22-2" );
	cvd->AddString( "1.22-1" );
	cvd->AddString( "1.22" );

	cvd->AddString( "1.21-6" );
	cvd->AddString( "1.21-5" );
	cvd->AddString( "1.21-4" );
	cvd->AddString( "1.21-3" );
	cvd->AddString( "1.21-2" );
	cvd->AddString( "1.21-1" );
	cvd->AddString( "1.21" );

	cvd->AddString( "1.20-12" );
	cvd->AddString( "1.20-11" );
	cvd->AddString( "1.20-10" );
	cvd->AddString( "1.20-9" );
	cvd->AddString( "1.20-8" );
	cvd->AddString( "1.20-7" );
	cvd->AddString( "1.20-6" );
	cvd->AddString( "1.20-5" );
	cvd->AddString( "1.20-4" );

	cvd->AddString( "1.20-3" );
	cvd->AddString( "1.20-2" );
	cvd->AddString( "1.20-1" );
	cvd->AddString( "1.20" );

	int index = cvd->FindStringExact( 0, m_ggncfgdlg->m_VersionDenyStr );
	if( index != CB_ERR ) cvd->SetCurSel( index );
	else cvd->SetCurSel( 0 );



	CRect r;
	GetClientRect( &r );

	int cx = r.right - r.left;
	int cy = r.bottom - r.top;

	GetDlgItem( IDOK )->SetWindowPos( &CWnd::wndTop, cx - 176, cy - 26, 0, 0, SWP_NOSIZE );
	GetDlgItem( IDCANCEL )->SetWindowPos( &CWnd::wndTop, cx - 86, cy - 26, 0, 0, SWP_NOSIZE );

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 例外 : OCX プロパティ ページは必ず FALSE を返します。
}


void CExtraOptionDlg::OnBnClickedOk()
{
	m_ggncfgdlg->m_useEx2 = true & IsDlgButtonChecked( IDC_USE_EX2  );
	m_ggncfgdlg->m_useKeyHook = true & IsDlgButtonChecked( IDC_USE_KEY_HOOK );
	m_ggncfgdlg->m_KeyChangeFlag = true & ( GetCheckedRadioButton( IDC_PKS, IDC_PKHS ) - IDC_PKS );

	m_ggncfgdlg->m_useVersionDeny = true & IsDlgButtonChecked( IDC_USE_VER_DENY );
	GetDlgItem( IDC_COMBO_VER_DENY )->GetWindowText( m_ggncfgdlg->m_VersionDenyStr, 10 );

	CDialog::OnOK();
}


void CExtraOptionDlg::OnBnClickedPks()
{
	CheckRadioButton( IDC_PKS, IDC_PKHS, IDC_PKS );
}


void CExtraOptionDlg::OnBnClickedPkhs()
{
	CheckRadioButton( IDC_PKS, IDC_PKHS, IDC_PKHS );
}

