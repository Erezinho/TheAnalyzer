/**************************************************************************************
*
*	Description: This class is a User Interface (UI) Thread class, derived from CWinThread.
*	It is intendant to work only as UI thread and not as worker thread.
*	
*	As a UI Thread, it has its own message loop.
*
*	Any request from the thread to do something ('task') must be submitted via service method. 
*	The service method posts a message to message loop using PostThreadMessage and 
*	corresponding user message.
*	Next, message is processed by the thread's engine (Run()) and the corresponding handler
*	is executed:
*
*	Calling thread		 UI Thread			UI Thread
*	--------------		-----------			---------
*	service method	|	message loop	|	 handler
*
*	Since there is a message loop, 'tasks' are executed based on posting order.
*
*	The use of critical section in this class may be dangerous since it can be mixed between 
*	service methods and handlers and cause deadlocks or undesired behavior.
*	It is better to have critical section defense in IF/Wrapper object (may be useful when data is 
*	set to the thread object via an IF method - if two thread will access the method without synchronization, 
*	data may be lost or corrupted. The use of critical section in IF level will avoid such a scenario)
*	
*	In this example code, the UI thread class holds a pointer to a (modeless) dialog which 
*	displays (interesting) data and a pointer to the calculator object
*	(Interfaces should be considered here!).
*
*
*	  Date	 |  Version	|	Name	|	Description
*	03.11.10	2.2.6.0		Erez		Created
*
***************************************************************************************/
#pragma once

class CCalculator;

// CAnalyzerThread
class CAnalyzerThread : public CWinThread
{
	DECLARE_DYNCREATE(CAnalyzerThread)

protected:
	CAnalyzerThread();           // protected constructor used by dynamic creation
	virtual ~CAnalyzerThread();

public:
	CAnalyzerThread(int i);//CWnd* i_pMainWnd);//= NULL);

	// Services
	void EndThread();
	void SetMainThread(CWnd* i_pMainWnd);
	//void OpenDialog(CWnd* i_pMainWnd);
	//void CloseDialog();
	//LRESULT Update(WPARAM wParam, LPARAM lParam);

	void Initialize(int iLength);
	void UpdateData(int iCurrent);//, CWrapper* i_Wrapper);

	// Handlers
	virtual BOOL InitInstance();
	virtual int ExitInstance();

protected:
	// Handlers
	//afx_msg void OnOpenDialog(WPARAM wParam, LPARAM lParam);
	afx_msg void OnPBSetMainThread(WPARAM wParam, LPARAM lParam);
	afx_msg void OnInitialize(WPARAM wParam, LPARAM lParam);
	afx_msg void OnUpdateData(WPARAM wParam, LPARAM lParam);

protected:
	DECLARE_MESSAGE_MAP()

	// Pointer main thread. Used to communicate (post messages) with creator
	CWnd* m_pMainWnd;

	CCalculator* m_pCalculator;
	//CToolDialog* m_pToolDialog;
	//CToolCalculator* m_pToolCalculator; 

	//IToolCalculator* m_pToolCalculator;
};


