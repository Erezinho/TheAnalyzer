/**************************************************************************************
*
*	Description: See header file
*	
*
*	Date     |  Version	|	Name	|	Description
*	03.11.10	2.2.6.0		Erez		Created
*										
***************************************************************************************/

// AnalyzerThread.cpp : implementation file
//

#include "stdafx.h"
#include "AnalyzerThread.h"
#include "Defines.h"
#include "Calculator.h"
//#include "AnalyzerDialog.h"
//#include "AnalyzerCalculator.h"

// CAnalyzerThread

//////////////////////////////////////////////////////////////////////////
//																		//
//							"Service methods"							//
//																		//
//	All methods below are executed by calling thread (e.g. main thread)	//
//																		//
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CAnalyzerThread, CWinThread)
CAnalyzerThread::CAnalyzerThread()
{
	m_pMainWnd = NULL;
// 	m_pAnalyzerDialog = NULL;
// 	m_pAnalyzerCalculator = NULL;
}
CAnalyzerThread::CAnalyzerThread(int i)//CWnd* i_pMainWnd)
{
	// Note: Since the pointer points to a window of another thread, it must be used only
	// to post messages otherwise exceptions may be raised
	m_pMainWnd = NULL;//i_pMainWnd;

// 	m_pAnalyzerDialog = NULL;
// 	m_pAnalyzerCalculator = NULL;
}

void CAnalyzerThread::EndThread()
{
// 	if ( m_pAnalyzerDialog )
// 	{
// 		delete m_pAnalyzerDialog;
// 		m_pAnalyzerDialog = NULL;
// 	}

	// The casting operator from CWinThread to HANDLE returns a handle
	// to the thread.
	HANDLE h = *this;

	// Sending WM_QUIT causes GetMessage in CWinThread::MessagePump() to return FALSE.
	// This causes termination of the thread. The exit code of the thread
	// will be the parameter sent to the WM_QUIT message.
	WPARAM exit_code = 1;
	PostThreadMessage(WM_QUIT,exit_code,0);

	// We wait for the thread to end (resources are released automatically due to m_bAutoDelete)
	WaitForSingleObject(h, INFINITE);
	//delete this; //no need as long as m_bAutoDelete is not set to FALSE
}

// void CAnalyzerThread::OpenDialog(CWnd* i_pMainWnd)
// {
// 	//CWnd* pWnd = GetMainWnd();
// 	if ( !m_pAnalyzerDialog )
// 	{
// 		m_pAnalyzerDialog = new CAnalyzerDialog(i_pMainWnd);//CWnd* pParent );
// 
// 		//must pass main window to UI Thread so it can post a message to 
// 		//main thread's message queue whenever it needs to notify something
// 		PostThreadMessage(WM_PB_SET_MAIN_THREAD, (WPARAM)m_pAnalyzerDialog, 0);//(WPARAM)i_pMainWnd, 0);
// 	}
// 
// 	m_pAnalyzerDialog->OpenDialog();
// 	//PostThreadMessage(WM_OPEN_DIALOG, (WPARAM)i_pMainWnd, 0);
// }
// 
// void CAnalyzerThread::CloseDialog()
// {
// 	if ( m_pAnalyzerDialog )
// 		m_pAnalyzerDialog->OnBnClickedCancel();
// 
// 	//Set Null pointer to dialog in UI thread
// 	PostThreadMessage(WM_PB_SET_MAIN_THREAD, 0, 0);//(WPARAM)i_pMainWnd, 0);
// }

void CAnalyzerThread::SetMainThread(CWnd* i_pParent)
{
	PostThreadMessage(WM_PB_SET_MAIN_THREAD, 0, (LPARAM)i_pParent);//(WPARAM)i_pMainWnd, 0);
}

void CAnalyzerThread::Initialize(int iLength)
{
	#pragma once ("CAnalyzerThread::Initialize - save necessary data into the threadd's object with locking");
	PostThreadMessage(WM_UIT_INITIALIZE, iLength, 0);
}

void CAnalyzerThread::UpdateData(int iCurrent)//, CWrapper* i_Wrapper)
{
	#pragma once ("CAnalyzerThread::UpdateData - insert data into a container with locking. run client fast and thread slow, see that it works");
	PostThreadMessage(WM_UIT_UPDATE_DATA, iCurrent, 0);//(LPARAM)i_Wrapper);
}

//////////////////////////////////////////////////////////////////////////
//																		//
//								Handlers								//
//																		//
//		All methods below are executed by the thread itself				//
//																		//
//////////////////////////////////////////////////////////////////////////

// Override InitInstance to perform tasks that must be completed when a thread is first created. 
BOOL CAnalyzerThread::InitInstance()
{
	// TODO:  perform and per-thread initialization here
	m_pCalculator = new CCalculator();

	return TRUE;
}

int CAnalyzerThread::ExitInstance()
{
	// Do not call this member function from anywhere but within the Run
	// member function (here Run is not touched!!!) 
	// The default implementation of this function deletes the CWinThread
	// object if m_bAutoDelete is TRUE

	// TODO:  perform any per-thread cleanup here
	return CWinThread::ExitInstance();
}

// D'tor is called automatically by ExitInstance only if m_bAutoDelete = TRUE 
//(which is the the default)
CAnalyzerThread::~CAnalyzerThread()
{
	#pragma message ("CAnalyzerThread::~CAnalyzerThread - Remove iTest!")
	int iTest = 0;
	iTest++;

	if ( m_pCalculator )
	{
		delete m_pCalculator;
		m_pCalculator = NULL;
	}
			// 	if ( m_pAnalyzerDialog )
	// 	{
	// 		delete m_pAnalyzerDialog;
	// 		m_pAnalyzerDialog = NULL;
	// 	}
}

// Message map
BEGIN_MESSAGE_MAP(CAnalyzerThread, CWinThread)
	//ON_THREAD_MESSAGE(WM_OPEN_DIALOG, OnOpenDialog)
	ON_THREAD_MESSAGE(WM_PB_SET_MAIN_THREAD, OnPBSetMainThread)
	ON_THREAD_MESSAGE(WM_UIT_INITIALIZE ,OnInitialize)
	ON_THREAD_MESSAGE(WM_UIT_UPDATE_DATA, OnUpdateData)
END_MESSAGE_MAP()


// void CAnalyzerThread::OnOpenDialog(WPARAM wParam, LPARAM lParam)
// {
// 	m_pAnalyzerDialog->OpenDialog((CWnd*)wParam);
// }


void CAnalyzerThread::OnPBSetMainThread(WPARAM wParam, LPARAM lParam)
{
	// Note: Since the pointer points to a window of another thread, it must be used only
	// to post messages otherwise exceptions may be raised
	m_pMainWnd = (CWnd*)lParam;
}

void CAnalyzerThread::OnInitialize(WPARAM wParam, LPARAM lParam)
{
	m_pCalculator->AnalyzeLength((int)wParam);

	if ( m_pMainWnd )
		m_pMainWnd->PostMessage(WM_MT_INITIALIZED, wParam, lParam);

	//m_pAnalyzerDialog->Update()

	//m_pAnalyzerDialog->Initialize((int)wParam);
}

void CAnalyzerThread::OnUpdateData(WPARAM wParam, LPARAM lParam)
{
	// Do internal calculations and update dialog
	m_pCalculator->AnalyzeCurentPos((int)wParam);

	if ( m_pMainWnd )
		m_pMainWnd->PostMessage(WM_MT_UPDATE_DATA, wParam, lParam);

	// OR call directly to the update method of the dialog (because the dialog is manged by the thread)
	//m_pAnalyzerDialog->Update((int)wParam, (CWrapper*)lParam);
}



