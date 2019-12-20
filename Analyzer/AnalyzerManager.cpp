#include "stdafx.h"
#include "AnalyzerManager.h"
#include "AnalyzerThread.h"
#include "AnalyzerDialog.h"
#include "Defines.h"

CAnalyzerManager::CAnalyzerManager() :
m_pAnalyzerThread(NULL),
m_pAnalyzerDialog(NULL)
{
	::InitializeCriticalSection( &m_CriticalSection );

	try
	{
		//CWnd* pWnd = GetMainWnd();
		//Create UI Thread				
		//CAnalyzerThread* m_pAnalyzerThread = (CAnalyzerThread*)AfxBeginThread(RUNTIME_CLASS(CAnalyzerThread));/*,
		//														THREAD_PRIORITY_NORMAL,
		//														0,
		//														CREATE_SUSPENDED);*///new CAnalyzerThread();	

		m_pAnalyzerThread = new CAnalyzerThread(1);//i_pMainWnd);
		BOOL bRet = m_pAnalyzerThread->CreateThread();
		_ASSERT(bRet != FALSE);

		// Create the dialog even though it may not be used!
		// This way the manager can update it with input and other received data 
		// without the need to save it by itself and once the dialog will be invoked (most cases)
		// it will be quick and easy
		m_pAnalyzerDialog = new CAnalyzerDialog();

		if ( m_pAnalyzerThread == NULL || m_pAnalyzerDialog == NULL )
			throw;
	}
	catch (...)
	{
		TRACE(_T("Exception caught in CAnalyzerManager::CAnalyzerManager()"));
		_ASSERT(false);
	}
}

CAnalyzerManager::~CAnalyzerManager()
{
	// If there is active UI thread, end it!
	if ( m_pAnalyzerThread )
	{
		m_pAnalyzerThread->EndThread();
		//delete m_pAnalyzerThread; - No Need, see CAnalyzerThread::ExitInstance()
		m_pAnalyzerThread = NULL;
	}
	if ( m_pAnalyzerDialog )
	{
		delete m_pAnalyzerDialog;
		m_pAnalyzerDialog = NULL;
	}

	::DeleteCriticalSection( &m_CriticalSection );
}

void CAnalyzerManager::Initialize(int i_iLength)
{
	::EnterCriticalSection( &m_CriticalSection );

	if ( m_pAnalyzerThread )
	{
		// All of the following statements work!
		//PostThreadMessage(m_pAnalyzerThread->m_nThreadID,WM_PB_SET_LENGTH, iLength, 0);
		//m_pAnalyzerThread->PostThreadMessage(WM_PB_SET_LENGTH, iLength, 0);
		m_pAnalyzerThread->Initialize(i_iLength);
	}
// 	if ( m_pAnalyzerDialog )
// 	{
// 		m_pAnalyzerDialog->Initialize(i_iLength);
// 	}

	::LeaveCriticalSection( &m_CriticalSection );
}

void CAnalyzerManager::Finalize()
{
	::EnterCriticalSection( &m_CriticalSection );

// 	if ( m_pAnalyzerThread )
// 	{
// 		m_pAnalyzerThread->Finalize();
// 	}
// 	if ( m_pAnalyzerDialog )
// 	{
// 		m_pAnalyzerDialog->Finalize();
// 	}

	::LeaveCriticalSection( &m_CriticalSection );
}

void CAnalyzerManager::UpdateData(int i_iCurrent)
{
	::EnterCriticalSection( &m_CriticalSection );

	if ( m_pAnalyzerThread )
	{
		m_pAnalyzerThread->UpdateData(i_iCurrent);
	}
// 	if ( m_pAnalyzerDialog )
// 	{
// 		m_pAnalyzerDialog->UpdateData(i_iCurrent);
// 	}

	::LeaveCriticalSection( &m_CriticalSection );
}

BOOL CAnalyzerManager::OpenDialog(CWnd* i_pParent)
{	
	BOOL bRet = TRUE;

	::EnterCriticalSection( &m_CriticalSection );

	if ( m_pAnalyzerThread )
	{
		m_pAnalyzerThread->SetMainThread((CWnd*)this);//i_pParent);
	}

	if ( m_pAnalyzerDialog )
	{
		bRet = m_pAnalyzerDialog->OpenDialog((CWnd*)this);//i_pParent);
	}

	::LeaveCriticalSection( &m_CriticalSection );

	return bRet;
}

void CAnalyzerManager::CloseDialog()
{	
	::EnterCriticalSection( &m_CriticalSection );

	if ( m_pAnalyzerDialog )
	{		
		m_pAnalyzerDialog->CloseDialog();
	}

	::LeaveCriticalSection( &m_CriticalSection );
}

//Message handlers - received by the Dialog and the Thread
BEGIN_MESSAGE_MAP(CAnalyzerManager, CWnd)
	ON_MESSAGE(WM_MT_INITIALIZED , &CAnalyzerManager::OnInitialized)
	ON_MESSAGE(WM_MT_UPDATE_DATA, &CAnalyzerManager::OnUpdateData)
END_MESSAGE_MAP()


LRESULT CAnalyzerManager::OnInitialized(WPARAM wParam, LPARAM lParam)
{
	::EnterCriticalSection( &m_CriticalSection );

	Sleep(5000);
	if ( m_pAnalyzerDialog )
	{		
		m_pAnalyzerDialog->Initialize((int)wParam);
	}
	
	return 0L;

	::LeaveCriticalSection( &m_CriticalSection );
}

LRESULT CAnalyzerManager::OnUpdateData(WPARAM wParam, LPARAM lParam)
{
	::EnterCriticalSection( &m_CriticalSection );

	if ( m_pAnalyzerDialog )
	{		
		m_pAnalyzerDialog->UpdateData((int)wParam);
	}

	return 0L;

	::LeaveCriticalSection( &m_CriticalSection );
}





