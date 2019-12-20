/**************************************************************************************
*
*	Description: See header file
*
*	Date     |  Description
*	Nov 2010	Created
*										
***************************************************************************************/

#include "stdafx.h"
#include "Messages.h"
#include "AnalyzerManagerThread.h"
#include "AnalyzerCalculatorThread.h"
#include "AnalyzerDialog.h"

#define MATRIX_RESERVE_SIZE	100

IMPLEMENT_DYNCREATE(CAnalyzerManagerThread, CWinThread)
CAnalyzerManagerThread::CAnalyzerManagerThread() :
m_pAnalyzerCalculatorThread(nullptr),
m_pAnalyzerDialog(nullptr),
m_iNumOfSets(0)
{
	CWinThread::m_bAutoDelete = TRUE;
}

// Destructor is called automatically by ExitInstance only if m_bAutoDelete = TRUE 
// (which is the the default)
CAnalyzerManagerThread::~CAnalyzerManagerThread()
{
	try
	{
		// If there is active UI thread, end it!
		if ( m_pAnalyzerCalculatorThread )
		{
			m_pAnalyzerCalculatorThread->StopCalculations();
			m_pAnalyzerCalculatorThread->EndThread();
			//delete CAnalyzerManagerThread; - No Need, see CAnalyzerManagerThread::ExitInstance()
			m_pAnalyzerCalculatorThread = nullptr;
		}
		if ( m_pAnalyzerDialog )
		{
			m_pAnalyzerDialog->CloseDialog();
			delete m_pAnalyzerDialog;
			m_pAnalyzerDialog = nullptr;
		}
	}		
	catch (...)
	{
		TRACE(_T("Exception caught in CAnalyzerManagerThread::~CAnalyzerManagerThread()"));
		_ASSERT(false);
	}
}

// Override InitInstance to perform tasks that must be completed when a thread is first created. 
BOOL CAnalyzerManagerThread::InitInstance()
{
	BOOL bRet = TRUE;
	try
	{
		/*m_pAnalyzerCalculatorThread = (CAnalyzerThread*)AfxBeginThread(RUNTIME_CLASS(CAnalyzerCalculatorThread));/*,
																THREAD_PRIORITY_NORMAL,
																0,
																CREATE_SUSPENDED);*/
		
		m_pAnalyzerCalculatorThread = new CAnalyzerCalculatorThread();
		
		if ( m_pAnalyzerCalculatorThread )
			bRet = m_pAnalyzerCalculatorThread->CreateThread();
		
		// Create the dialog even though it may not be used!
		// This way the manager can update it with input and other received data 
		// without the need to save it by itself and once the dialog will be invoked (most cases)
		// it will be quick and easy
		m_pAnalyzerDialog = new CAnalyzerDialog();

		// if bRet != FALSE then m_pAnalyzerCalculatorThread is not nullptr
		( bRet == FALSE || m_pAnalyzerDialog == nullptr || m_pAnalyzerCalculatorThread == nullptr ) ? throw : 0;

		// At this point, both pointers exist!
		m_pAnalyzerCalculatorThread->SetParentThread((CWinThread*)this);
		m_pAnalyzerDialog->SetParentThread((CWinThread*)this);		
	}
	catch (...)
	{
		if ( m_pAnalyzerCalculatorThread )
			m_pAnalyzerCalculatorThread->EndThread();

		delete m_pAnalyzerDialog;

		TRACE(_T("Exception caught in CAnalyzerManager::CAnalyzerManager()"));
		_ASSERT(false);
		bRet = FALSE;
	}

	return bRet;
}

int CAnalyzerManagerThread::ExitInstance()
{
	// Do not call this member function from anywhere but within the Run
	// member function (here Run is not touched!!!) 
	return CWinThread::ExitInstance();
}

//////////////////////////////////////////////////////////////////////////
//																		//
//							"Service methods"							//
//																		//
//	All methods below are executed by calling thread (e.g. main thread)	//
//																		//
//////////////////////////////////////////////////////////////////////////
void CAnalyzerManagerThread::EndThread()
{
	// The casting operator from CWinThread to HANDLE returns a handle
	// to the thread.
	HANDLE h = *this;

	// Sending WM_QUIT causes GetMessage in CWinThread::MessagePump() to return FALSE.
	// This causes termination of the thread. The exit code of the thread
	// will be the parameter sent to the WM_QUIT message.
	WPARAM exit_code = 1;
	PostThreadMessage(WM_QUIT, exit_code, 0);

	// We wait for the thread to end (resources are released automatically due to m_bAutoDelete)
	WaitForSingleObject(h, INFINITE);
	//delete this; //no need as long as m_bAutoDelete is not set to FALSE
}

void CAnalyzerManagerThread::Initialize(const CString& i_refProcessTitle, bool i_bTestMode)
{
	m_SecureMatrix.EnterCriticalSection();
	
	// Reset the matrix
	m_SecureMatrix.m_Variable.m_MatrixXd.resize(0,0);

	// Set the flag to be true meaning something has changed here
	m_SecureMatrix.m_Variable.m_bFlagged = true;

	m_iNumOfSets = 0;
	m_SecureMatrix.LeaveCriticalSection();

	m_bTestMode = i_bTestMode;

	CString* pCString = new CString;
	*pCString = i_refProcessTitle;

	PostThreadMessage(WM_AMT_INITIALIZE, (WPARAM) pCString, 0);
}

void CAnalyzerManagerThread::SetData(const vector<double>& i_refVecVals)
{
	RowVectorXd newRow;
	newRow.resize(i_refVecVals.size());
	vector<double>::const_iterator vecIter = i_refVecVals.begin(), vecIterEnd = i_refVecVals.end();

	for(int i = 0 ; vecIter != vecIterEnd ; ++ vecIter , ++i)
	{
		newRow(i) = *vecIter;
	}

#if WRITE_TO_FILE
 	CFileOutput::m_foutput.open(_T("o:\\output\\MatOriginal.txt"), ios::out | ios::app);
 	CFileOutput::m_foutput << newRow << endl;
 	CFileOutput::m_foutput.close();
#endif

	m_SecureMatrix.EnterCriticalSection();
	++m_iNumOfSets;

	MatrixXd& refMat = m_SecureMatrix.m_Variable.m_MatrixXd;

	// If there are no available rows in the matrix, expand the number
	// of rows (number of rows) in one time to save time
	if ( refMat.rows() <= m_iNumOfSets )
		refMat.conservativeResize( refMat.rows() + MATRIX_RESERVE_SIZE , newRow.size() ); // not so efficient
	
	// Insert the new data set into the matrix
	refMat.row(m_iNumOfSets-1) = newRow;

	// Set the flag to indicates there is new data / data is still entering
	m_SecureMatrix.m_Variable.m_bFlagged = true;

	m_SecureMatrix.LeaveCriticalSection();
}

void CAnalyzerManagerThread::Finalize()
{
	m_SecureMatrix.EnterCriticalSection();
	m_SecureMatrix.m_Variable.m_bFlagged = false;
	m_SecureMatrix.LeaveCriticalSection();

	PostThreadMessage(WM_AMT_FINALIZE, 0, 0);
}

void CAnalyzerManagerThread::OpenDialog(CWnd* i_pMainWnd, int i_iProcessID)
{
	PostThreadMessage(WM_AMT_OPEN_DIALOG, (WPARAM)i_pMainWnd, (LPARAM)i_iProcessID);
}

void CAnalyzerManagerThread::CloseDialog()
{
	PostThreadMessage(WM_AMT_CLOSE_DIALOG, 0, 0);
}

//////////////////////////////////////////////////////////////////////////
//																		//
//								Handlers								//
//																		//
//		All methods below are executed by the thread itself				//
//																		//
//////////////////////////////////////////////////////////////////////////

//Message handlers - received by clients (IF), the Dialog and the Calculator Thread
BEGIN_MESSAGE_MAP(CAnalyzerManagerThread, CWinThread) 
	ON_THREAD_MESSAGE(WM_AMT_INITIALIZE,				&CAnalyzerManagerThread::OnInitialize)
	ON_THREAD_MESSAGE(WM_AMT_FINALIZE,					&CAnalyzerManagerThread::OnFinalize)
	ON_THREAD_MESSAGE(WM_ACT_EXEC_CALCULATIONS,			&CAnalyzerManagerThread::OnExecuteCalculations)
	ON_THREAD_MESSAGE(WM_ACT_CLUSTERING_INITIALIZING,	&CAnalyzerManagerThread::OnClusteringInitComplete)
	ON_THREAD_MESSAGE(WM_ACT_CLUSTERING_CALCULATING,	&CAnalyzerManagerThread::OnClusteringCalcComplete)
	ON_THREAD_MESSAGE(WM_ACT_PROJECTION_INITIALIZING,	&CAnalyzerManagerThread::OnProjectionInitComplete)
	ON_THREAD_MESSAGE(WM_ACT_PROJECTION_STEP,			&CAnalyzerManagerThread::OnProjectionStep)
	ON_THREAD_MESSAGE(WM_AMT_STOP_CALCULATIONS,			&CAnalyzerManagerThread::OnStopCalculations)
	ON_THREAD_MESSAGE(WM_AMT_CALC_COMPLETED,			&CAnalyzerManagerThread::OnCalcCompleted)
	ON_THREAD_MESSAGE(WM_AMT_OPEN_DIALOG,				&CAnalyzerManagerThread::OnOpenDialog)
	ON_THREAD_MESSAGE(WM_AMT_CLOSE_DIALOG,				&CAnalyzerManagerThread::OnCloseDialog)

	ON_THREAD_MESSAGE(WM_AMT_SAVE_DATA_SET,				&CAnalyzerManagerThread::OnSaveDataSet)
	ON_THREAD_MESSAGE(WM_AMT_SAVE_PARTITION_MATRIX,		&CAnalyzerManagerThread::OnSavePartitionMatrix)
	ON_THREAD_MESSAGE(WM_AMT_SAVE_PROJECTION_MATRIX,	&CAnalyzerManagerThread::OnSaveProjectionMatrix)
	ON_THREAD_MESSAGE(WM_AMT_SAVE_COMPLETED,			&CAnalyzerManagerThread::OnSaveCompleted)
END_MESSAGE_MAP()

void CAnalyzerManagerThread::OnInitialize(WPARAM wParam, LPARAM lParam)
{
 	if ( m_pAnalyzerCalculatorThread )
 	{
 		// All of the following statements work!
		//PostThreadMessage(m_pAnalyzerCalculatorThread->m_nThreadID, WM_ACT_INITIALIZE, iLength, 0);
		//m_pAnalyzerCalculatorThread->PostThreadMessage(WM_ACT_INITIALIZE, wParam, 0);
		//m_pAnalyzerCalculatorThread->Initialize(i_iLength);
		m_pAnalyzerCalculatorThread->SetTestMode(m_bTestMode);
	}

 	if ( m_pAnalyzerDialog )
	{
		CString* pCString = (CString*)wParam;
		m_pAnalyzerDialog->Initialize(*pCString, m_bTestMode);

		delete pCString;
 	}
}

void CAnalyzerManagerThread::OnFinalize(WPARAM wParam, LPARAM lParam)
{
	if ( m_pAnalyzerDialog )
	{
		m_pAnalyzerDialog->Finalize();
	}
	else
	{
		AfxMessageBox(ANALYZER_ERROR_EXEC_CLUSTERING, MB_OK | MB_ICONEXCLAMATION);
	}
}

void CAnalyzerManagerThread::OnExecuteCalculations(WPARAM wParam, LPARAM lParam)
{
	m_SecureMatrix.EnterCriticalSection();
	MatrixXd& refMat = m_SecureMatrix.m_Variable.m_MatrixXd; 

	if (refMat.rows() > m_iNumOfSets)
	{
		refMat.conservativeResize(m_iNumOfSets, refMat.cols()); // not so efficient
	}

#if WRITE_TO_FILE
	CFileOutput::m_foutput.open(_T("o:\\output\\Mat.txt"), ios::out | ios::app);
	CFileOutput::m_foutput << refMat << endl << endl;
	CFileOutput::m_foutput.close();
#endif
	m_SecureMatrix.LeaveCriticalSection();
	
	if (m_pAnalyzerCalculatorThread)
	{
		m_pAnalyzerCalculatorThread->PostThreadMessage(WM_ACT_EXEC_CALCULATIONS, wParam, (LPARAM)&m_SecureMatrix);
	}
}

void CAnalyzerManagerThread::OnClusteringInitComplete(WPARAM wParam, LPARAM lParam)
{
	if ( m_pAnalyzerDialog )
	{
		m_pAnalyzerDialog->UpdateProgress(enClusteringInitializing);
	}
}

void CAnalyzerManagerThread::OnClusteringCalcComplete(WPARAM wParam, LPARAM lParam)
{
	if ( m_pAnalyzerDialog )
	{
		m_pAnalyzerDialog->UpdateProgress(enClusteringCalculating);
	}
}

void CAnalyzerManagerThread::OnProjectionInitComplete(WPARAM wParam, LPARAM lParam)
{
	if ( m_pAnalyzerDialog )
	{
		m_pAnalyzerDialog->UpdateProgress(enProjectionInitializing);
	}
}

void CAnalyzerManagerThread::OnProjectionStep(WPARAM wParam, LPARAM lParam)
{
	if ( m_pAnalyzerDialog )
	{
		m_pAnalyzerDialog->UpdateProgress(enProjectionStepping);
	}
}

void CAnalyzerManagerThread::OnStopCalculations(WPARAM wParam, LPARAM lParam)
{
	if (m_pAnalyzerCalculatorThread)
	{
		m_pAnalyzerCalculatorThread->StopCalculations();
	}
}

void CAnalyzerManagerThread::OnCalcCompleted(WPARAM wParam, LPARAM lParam)
{
	// In order to save coping time (as the matrices might be very huge)
	// just transfer the pointers of data structures to the dialog	
	if ( m_pAnalyzerDialog )
	{
		m_pAnalyzerDialog->FinishCalculations(wParam, lParam);
	}
}

void CAnalyzerManagerThread::OnOpenDialog(WPARAM wParam, LPARAM lParam)
{	
	if ( m_pAnalyzerDialog )
	{
		BOOL bRet = FALSE;
		bRet = m_pAnalyzerDialog->OpenDialog((CWnd*)wParam, (int)lParam);
	}
}

void CAnalyzerManagerThread::OnCloseDialog(WPARAM wParam, LPARAM lParam)
{	
	if ( m_pAnalyzerDialog )
	{		
		m_pAnalyzerDialog->CloseDialog();
	}
}

void CAnalyzerManagerThread::OnSaveDataSet(WPARAM wParam, LPARAM lParam)
{
	if (m_pAnalyzerCalculatorThread)
	{
		m_pAnalyzerCalculatorThread->PostThreadMessage(WM_AMT_SAVE_DATA_SET, wParam, lParam);
	}
}

void CAnalyzerManagerThread::OnSavePartitionMatrix(WPARAM wParam, LPARAM lParam)
{
	if (m_pAnalyzerCalculatorThread)
	{
		m_pAnalyzerCalculatorThread->PostThreadMessage(WM_AMT_SAVE_PARTITION_MATRIX, wParam, lParam);
	}
}

void CAnalyzerManagerThread::OnSaveProjectionMatrix(WPARAM wParam, LPARAM lParam)
{
	if (m_pAnalyzerCalculatorThread)
	{
		m_pAnalyzerCalculatorThread->PostThreadMessage(WM_AMT_SAVE_PROJECTION_MATRIX, wParam, lParam);
	}
}

// wParam holds a boolean 1 = successfully , 0 = failed
void CAnalyzerManagerThread::OnSaveCompleted(WPARAM wParam, LPARAM lParam)
{
	if ( m_pAnalyzerDialog )
	{
		bool bRet = wParam == 0 ? false : true;
		m_pAnalyzerDialog->SavingCompletionReport(bRet);
	}
}
