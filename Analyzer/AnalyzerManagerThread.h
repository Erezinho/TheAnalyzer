/**************************************************************************************
*
*	Project: Analyzer.dll
*
*	Description: This class is a User Interface (UI) Thread class, derived from CWinThread.
*	It is intendant to work only as UI thread and not as worker thread.
*	
*	As a UI Thread, it has its own message loop.
*
*	Any request from a client to the thread to do something ('task') must be submitted via service method. 
*	The service method posts a message to message loop using PostThreadMessage and the 
*	corresponding user message.
*	Other option is to post a message directly to the thread's message loop  
*	One way or another, the message is processed by the thread's engine (Run()) and the corresponding handler
*	is executed.
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
*	In addition, critical section are used to protect internal data structure which are used by the analyzer thread,
*	the calculator thread and the client thread (only one can affect this class due to the protection in the IF layer)
*	Currently, those critical sections are attached to data structures which need protection.
*
*	The analyzer manager thread class holds pointers to the analyzer dialog and the analyzer calculating unit
*	(which is implemented in a separated thread as well)
*	The hierarchy/design here is simple without "too much" abstraction.
*	The module can be re-designed to be more generic with layers of abstractions for easy extension.
*
*	NOTE: since the dialog is created by the Analyzer manager thread, all operation on the dialog must go
*	through the analyzer manager thread (i.e. main thread executes the dialog code) otherwise an exception
*	will occure and a the application will crash (the reason is that some methods of CDialog are not thread
*	safe therefore, generate assertion when invoked from different thread than the one who created the 
*	dialog e.g. UpdateData() )
*
*
*	Date	 |  Description
*	Nov 2010	Created
*	Jul 2011	Updated
*
******************************************************************************/
#pragma once
#include "Defines.h"

class CAnalyzerCalculatorThread;
class CAnalyzerDialog;

class CAnalyzerManagerThread : public CWinThread
{
	DECLARE_DYNCREATE(CAnalyzerManagerThread)
public:
	CAnalyzerManagerThread();

	// Services

	// Called by creator to end the thread (interface above Destructor).
	// The method posts the WM_QUIT message which cause the Run method
	// to quit the message loop and call ExitInstance which deletes CWinThread
	// (=> calls the destructor) only if m_bAutoDelete is TRUE.
	void EndThread();

	// Initializes the class
	// The method must be called only once because if more than one Thread is
	// using the module and each one calls Initialize, some data will be lost
	// Make sure only one thread (manager thread) calls this method
	void Initialize(const CString& i_refProcessTitle, bool i_bTestMode);
	
	void SetData(const vector<double>& i_refVecVals);
	
	void Finalize();
 	
	void OpenDialog(CWnd* i_pMainWnd, int i_iProcessID);
 	
	void CloseDialog();

public:
	// The matrix holds the data (sets) received from client(s)
	// The matrix is wrapped in a structure which includes critical section and a flag. 
	// The idea is that the matrix can be used also by the calculator when no data is received
	// any more (indicated by the flag) but only the clustering/projection parameters are changed 
	// This way the data is not copied again and again (the matrix might get very big)
	// The flag is updated by Finalize and also by the Calculator
	SecureVariable< SFlaggedMatrix > m_SecureMatrix;
	
protected:
	virtual ~CAnalyzerManagerThread();

	// Handlers - called automatically

	// The method must be added to override the base class implementation.
	// The method initializes the new instance of the user-interface thread
	virtual BOOL InitInstance();

	// The default implementation of this function deletes the CWinThread
	// object if m_bAutoDelete is TRUE
	virtual int ExitInstance();

	// Handlers

	// wParam is a pointer to CString which holds the analyzed process title
	afx_msg void OnInitialize(WPARAM wParam, LPARAM lParam);

	// No arguments required
	afx_msg void OnFinalize(WPARAM wParam, LPARAM lParam);

	// wParam holds a pointer to attributes structure
	afx_msg	void OnExecuteCalculations(WPARAM wParam, LPARAM lParam);

	// No arguments required
	afx_msg	void OnClusteringInitComplete(WPARAM wParam, LPARAM lParam);

	// No arguments required
	afx_msg	void OnClusteringCalcComplete(WPARAM wParam, LPARAM lParam);

	// No arguments required
	afx_msg	void OnProjectionInitComplete(WPARAM wParam, LPARAM lParam);

	// No arguments required
	afx_msg	void OnProjectionStep(WPARAM wParam, LPARAM lParam);

	// No arguments required
	afx_msg	void OnStopCalculations(WPARAM wParam, LPARAM lParam);

	// wParam and lParam hold pointers to Matrices - projected centers and projected data set
	afx_msg	void OnCalcCompleted(WPARAM wParam, LPARAM lParam);

	// lParam - must be a pointer to main window. It is used to locate 
	// the dialog next to main window instead of on top of it
	afx_msg void OnOpenDialog(WPARAM wParam, LPARAM lParam);

	// No arguments required
	afx_msg void OnCloseDialog(WPARAM wParam, LPARAM lParam);

	// No arguments required
	afx_msg void OnSaveDataSet(WPARAM wParam, LPARAM lParam);

	// No arguments required
	afx_msg void OnSavePartitionMatrix(WPARAM wParam, LPARAM lParam);

	// No arguments required
	afx_msg void OnSaveProjectionMatrix(WPARAM wParam, LPARAM lParam);

	// wParam holds a boolean 1 = successfully , 0 = failed
	afx_msg void OnSaveCompleted(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

protected:
	bool m_bTestMode;

	// Pointer to analyzer dialog
	CAnalyzerDialog* m_pAnalyzerDialog;
	CAnalyzerCalculatorThread* m_pAnalyzerCalculatorThread;

private:
	int m_iNumOfSets; // Each set is a row in the matrix
};
