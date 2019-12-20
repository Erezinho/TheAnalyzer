/*****************************************************************************                                                
*
*	Project: Analyzer.dll
*	
*	Description: Analyzer manager class
*	Holds pointer to the analyzer dialog and the analyzer calculating unit
*	(which is implemented in a separated thread)
*
*	Date	 |  Version  |  Name  |		Description
*	03.11.10	2.2.6.0		Erez		Created
*
******************************************************************************/
#pragma once

class CAnalyzerThread;
class CAnalyzerDialog;

// The class inherits from CWnd just for the purpose of having a message loop.
// By inheriting from CWnd, CAnalyzerManager has a message loop which can be used to 
// post messages from both UI thread, whenever it completes work or "needs" something
// and from the dialog which may want to notify the manager that something has been changed in GUI
class CAnalyzerManager : public CWnd
{
public:
	CAnalyzerManager();
	~CAnalyzerManager();

	void Initialize(int i_iLength);
	void Finalize();

	void UpdateData(int i_iCurrent);

	BOOL OpenDialog(CWnd* i_pParent);
	void CloseDialog();

	//Handlers
	afx_msg LRESULT OnInitialized(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUpdateData(WPARAM wParam, LPARAM lParam);

protected:
	DECLARE_MESSAGE_MAP()

protected:
	CAnalyzerDialog* m_pAnalyzerDialog;
	CAnalyzerThread* m_pAnalyzerThread;

private:
	CRITICAL_SECTION m_CriticalSection;

};