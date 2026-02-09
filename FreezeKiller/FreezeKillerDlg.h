
// FreezeKillerDlg.h: 头文件
//
#pragma once
#include<string>


// CFreezeKillerDlg 对话框
class CFreezeKillerDlg : public CDialogEx
{
// 构造
public:
	CFreezeKillerDlg(CWnd* pParent = nullptr);	// 标准构造函数

protected:
	std::wstring m_ContentOfEdit;

	// 实现
protected:
	HICON m_hIcon;
	//CFont m_FontForButton;

	CEdit* m_Edit = nullptr;
	CButton* m_buttonSettingDeepFrz = nullptr, * m_deleteDeepFrz = nullptr,
		* m_TopMost = nullptr;

	HANDLE m_h_MyDriver = nullptr;

	bool isDFRuning = false;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	//按钮处理
	afx_msg void OnClickSettingDeepFrzClick();
	afx_msg void OnClickDeleteDeepFrz();
	afx_msg void OnClickTopMost();

	//退出消息

	afx_msg void OnClose();

	DECLARE_MESSAGE_MAP()
};
