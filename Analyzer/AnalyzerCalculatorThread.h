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
*	service methods and handlers and cause deadlocks or undesired behavior so be aware!
*	Currently, critical sections are attached to data structures which need protection.
*	
*
*	Date	 |  Description
*	Nov 2010	Created
*
***************************************************************************************/
#pragma once

#include "ClusteringIF.h"
#include "Defines.h"

// CAnalyzerCalculatorThread
class CAnalyzerCalculatorThread : public CWinThread
{
	DECLARE_DYNCREATE(CAnalyzerCalculatorThread)

public:
	CAnalyzerCalculatorThread();
	
	// Services

	// Called by creator to end the thread (interface above Destructor).
	// The method posts the WM_QUIT message which cause the Run method
	// to quit the message loop and call ExitInstance which deletes CWinThread
	// (=> calls the destructor) only if m_bAutoDelete is TRUE.
	void EndThread();

	void SetParentThread(CWinThread* i_pParanetThread);

	void SetTestMode(bool i_bTestMode);

	// The method allows external thread to stop current thread's calculations
	// Working with message loop here might not have immediate influence because if 
	// the thread is calculating, the posted message (i.e. 'STOP') won't be processed until
	// the thread is available i.e finished calculations.
	// THe idea here is to get immediate influence on the thread
	void StopCalculations();

public:
	SecureVariable<SClusterProjectionResult> m_ClusteringProjectionResults;

protected:
	//CAnalyzerCalculatorThread();           // protected constructor used by dynamic creation
	virtual ~CAnalyzerCalculatorThread();

	// Handlers - called automatically
	// The method must be added to override the base class implementation.
	// The method initializes the new instance of the user-interface thread
	virtual BOOL InitInstance();
	
	// The default implementation of this function deletes the CWinThread
	// object if m_bAutoDelete is TRUE
	virtual int ExitInstance();

	// Handlers 
	// Those handlers as protected so no one could use them
	// directly but only via PostThreadMessage
	afx_msg void OnExecuteCalculations(WPARAM wParam, LPARAM lParam);

	// No arguments required
	afx_msg void OnSaveDataSet(WPARAM wParam, LPARAM lParam);

	// No arguments required
	afx_msg void OnSavePartitionMatrix(WPARAM wParam, LPARAM lParam);

	// No arguments required
	afx_msg void OnSaveProjectionMatrix(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

protected:
	// Pointer to the main thread.
	// Used to communicate (post messages) with creator
	CWinThread* m_ParentThread;

	// Clustering and projection instances
	CGKCluster  m_Clustering;
	CFuzSam		m_FuzSam;
	
	// Indicates if calculations should be executed under test mode
	bool m_bTestMode;
	bool m_bStop;

private:
	bool calculate(MatrixXd& i_DataSetMatrix, CString& o_sReport);

private:
	// Clustering attributes
	SClustringAttributes m_ClusteringAtts;
};


