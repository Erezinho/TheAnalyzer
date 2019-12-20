/**************************************************************************************
*
*	Description: See header file
*	
*	Date     |  Description
*	Nov 2010	Created
*	May 2011	CreateAnalyzerIF & DeleteAnalyzerIF - removed local static variable and 
*				added m_staticCriticalSection instead
*										
***************************************************************************************/
#include "stdafx.h"
#include "Defines.h"
#include "..\Include\AnalyzerIF.h"
#include "AnalyzerManagerThread.h"

//A pointer to static IF object (based on lazy instantiation approach) - one interface
//class which holds a private instance
CAnalyzerIF* CAnalyzerIF::m_pInstance = nullptr;

// lazy instantiation and double check lock mechanism.
// It is known that those techniques might be unsafe in 
// case of multi-threaded environment.
// (see http://www.aristeia.com/Papers/DDJ_Jul_Aug_2004_revised.pdf and
//		http://www.cs.umd.edu/~pugh/java/memoryModel/DoubleCheckedLocking.html )

::CCriticalSection CAnalyzerIF::m_staticCriticalSection{};

CAnalyzerIF* CAnalyzerIF::CreateAnalyzerIF()
{
	try
	{
		// In case the object is known to be instantiated very rarely (or part of GUI operations)
		// it is better to avoid this double check as it may cause, in some circumstances, fatal problems (see link).
		// It is preferred to lose efficiency in cases the object is rarely required!
		if ( m_pInstance == nullptr )
		{
			m_staticCriticalSection.Lock();
			
			if ( m_pInstance == nullptr )
			{
				m_pInstance = new CAnalyzerIF();
			}
			
			m_staticCriticalSection.Unlock();
		}
	}
	catch(...)
	{
		delete m_pInstance;
		m_pInstance = nullptr;
		TRACE(_T("Exception caught in CAnalyzerIF::CreateAnalyzerIF()"));
		_ASSERT(false);
	}

	return m_pInstance;
}

void CAnalyzerIF::DeleteAnalyzerIF()
{
	try
	{
		if ( m_pInstance != nullptr )
		{
			m_staticCriticalSection.Lock();

			if ( m_pInstance != nullptr )
			{
				delete m_pInstance; 
				m_pInstance = nullptr;
			}

			m_staticCriticalSection.Unlock();
		}
	}
	catch(...)
	{
		TRACE(_T("Exception caught in CAnalyzerIF::DeleteAnalyzerIF()"));
		_ASSERT(false);
	}
}

/////////////////////////////////////////////////////////////////////////////////

CAnalyzerIF::CAnalyzerIF() :
m_pAnalyzerManagerThread (nullptr) 
{
	try
	{
		::InitializeCriticalSection( &m_CriticalSection );

		BOOL bRet = FALSE;
		m_pAnalyzerManagerThread = new CAnalyzerManagerThread;
		if (m_pAnalyzerManagerThread)
		{
			bRet = m_pAnalyzerManagerThread->CreateThread();
		}
		
		(bRet == FALSE) ? throw : 0 ;
	}
	catch(...)
	{
		if (m_pAnalyzerManagerThread)
		{
			m_pAnalyzerManagerThread->EndThread();
		}
		
		m_pAnalyzerManagerThread = nullptr;

		::DeleteCriticalSection( &m_CriticalSection );

		TRACE(_T("Exception caught in CAnalyzerIF::CAnalyzerIF()"));
		_ASSERT(false);
	}
}

CAnalyzerIF::~CAnalyzerIF()
{
	try
	{
		if ( m_pAnalyzerManagerThread )
		{
			m_pAnalyzerManagerThread->EndThread();
			m_pAnalyzerManagerThread = nullptr;
		}
		::DeleteCriticalSection( &m_CriticalSection );
	}
	catch (...)
	{
		::DeleteCriticalSection( &m_CriticalSection );

		TRACE(_T("Exception caught in CAnalyzerIF::~CAnalyzerIF()"));
		_ASSERT(false);
	}
}

/////////////////////////////////////////////////////////////////////////////////
//							Interface methods

void CAnalyzerIF::Initialize(const CString& i_refProcessTitle, bool i_bTestMode)
{
	::EnterCriticalSection( &m_CriticalSection );

	try
	{
		if ( m_pAnalyzerManagerThread )
		{
			m_pAnalyzerManagerThread->Initialize(i_refProcessTitle, i_bTestMode);
		}
	}
	catch (...)
	{
		TRACE(_T("Exception caught in CAnalyzerIF::Initialize()"));
		_ASSERT(false);
	}

	::LeaveCriticalSection( &m_CriticalSection );
}

void CAnalyzerIF::SetData(const vector<double>& i_refVecVals)
{
	::EnterCriticalSection( &m_CriticalSection );

	try
	{
		if ( m_pAnalyzerManagerThread )
		{
			m_pAnalyzerManagerThread->SetData(i_refVecVals);
		}
	}
	catch(...)
	{

		TRACE(_T("Exception caught in CAnalyzerIF::SetData()"));
		_ASSERT(false);

	}

	::LeaveCriticalSection( &m_CriticalSection );
}

void CAnalyzerIF::Finalize()
{
	::EnterCriticalSection( &m_CriticalSection );

	try
	{
		if ( m_pAnalyzerManagerThread )
		{
			m_pAnalyzerManagerThread->Finalize();
		}
	}
	catch (...)
	{
		TRACE(_T("Exception caught in CAnalyzerIF::Finalize()"));
		_ASSERT(false);
	}

	::LeaveCriticalSection( &m_CriticalSection );
}

void CAnalyzerIF::OpenDialog(CWnd* i_pMainWnd, int i_iProcessID /*= 0*/)
{
	::EnterCriticalSection( &m_CriticalSection );

	try
	{
		if ( m_pAnalyzerManagerThread )
		{	
			m_pAnalyzerManagerThread->OpenDialog(i_pMainWnd, i_iProcessID);

		}
	}
	catch (...)
	{
		TRACE(_T("Exception caught in CAnalyzerIF::OpenDialog()"));
		_ASSERT(false);
	}

	::LeaveCriticalSection( &m_CriticalSection );
}

void CAnalyzerIF::CloseDialog()
{	
	::EnterCriticalSection( &m_CriticalSection );

	try
	{
		if ( m_pAnalyzerManagerThread )
		{	
			m_pAnalyzerManagerThread->CloseDialog();
		}
	}
	catch (...)
	{
		TRACE(_T("Exception caught in CAnalyzerIF::CloseDialog()"));
		_ASSERT(false);
	}

	::LeaveCriticalSection( &m_CriticalSection );
}
