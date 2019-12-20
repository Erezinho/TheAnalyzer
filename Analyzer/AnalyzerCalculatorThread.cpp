/**************************************************************************************
*
*	Description: See header file
*
*	Date     |  Description
*	Nov 2010	Created
*										
***************************************************************************************/

#include "stdafx.h"
#include "Defines.h"
#include "AnalyzerCalculatorThread.h"
#include "Messages.h"

IMPLEMENT_DYNCREATE(CAnalyzerCalculatorThread, CWinThread)

CAnalyzerCalculatorThread::CAnalyzerCalculatorThread()
{
	// Note: Since the pointer points to a window of another thread, it must be used only
	// to post messages otherwise exceptions may be raised
	m_ParentThread = nullptr;
	m_bTestMode = false;
	m_bStop = false;

	CWinThread::m_bAutoDelete = TRUE;
}

// Destructor is called automatically by ExitInstance only if m_bAutoDelete = TRUE 
// (which is the the default)
CAnalyzerCalculatorThread::~CAnalyzerCalculatorThread()
{}

// Override InitInstance to perform tasks that must be completed when a thread is first created. 
BOOL CAnalyzerCalculatorThread::InitInstance()
{
	return TRUE;
}

int CAnalyzerCalculatorThread::ExitInstance()
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
void CAnalyzerCalculatorThread::EndThread()
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

void CAnalyzerCalculatorThread::SetParentThread(CWinThread* i_pParanetThread)
{
	m_ParentThread = i_pParanetThread;
}

void CAnalyzerCalculatorThread::SetTestMode(bool i_bTestMode)
{
	m_bTestMode = i_bTestMode;
}

void CAnalyzerCalculatorThread::StopCalculations()
{
	m_bStop = true;
	m_Clustering.StopCalcs(m_bStop);
	m_FuzSam.StopCalcs(m_bStop);
}

//////////////////////////////////////////////////////////////////////////
//																		//
//								Handlers								//
//																		//
//		All methods below are executed by the thread itself				//
//																		//
//////////////////////////////////////////////////////////////////////////

// Message map
BEGIN_MESSAGE_MAP(CAnalyzerCalculatorThread, CWinThread)
	ON_THREAD_MESSAGE(WM_ACT_EXEC_CALCULATIONS,			&CAnalyzerCalculatorThread::OnExecuteCalculations)
	ON_THREAD_MESSAGE(WM_AMT_SAVE_DATA_SET,				&CAnalyzerCalculatorThread::OnSaveDataSet)
	ON_THREAD_MESSAGE(WM_AMT_SAVE_PARTITION_MATRIX,		&CAnalyzerCalculatorThread::OnSavePartitionMatrix)
	ON_THREAD_MESSAGE(WM_AMT_SAVE_PROJECTION_MATRIX,	&CAnalyzerCalculatorThread::OnSaveProjectionMatrix)
END_MESSAGE_MAP()

void CAnalyzerCalculatorThread::OnExecuteCalculations(WPARAM wParam, LPARAM lParam)
{
	bool bIsOK = true;
	m_bStop = false;
	m_Clustering.StopCalcs(m_bStop);
	m_FuzSam.StopCalcs(m_bStop);
	
	CString sReport;

	// NOTE: There are cases when only the attributes are changed and the data set is left untouched 
	// so it is possible to optimize the process by saving the extracted matrix 
	// and use it again and again as long as it does not change.
	// BUT, this approach has one big drawback: both the calculator and the manager hold the same data 
	// twice and the data may be huge.
	// There are other ways to handle the data, each one has its advantages and disadvantages
	// and need to be considered wisely before implementation 
	// The solution here is to hold the data only in the manager and only if no new data is received by 
	// the module (indicated by a flag), the original data set is used directly otherwise it is copied 
	// locally here and used for calculations.

	// Get the attributes
	SecureVariable<SClustringAttributes>* pClusteringAtt = (SecureVariable<SClustringAttributes> *) wParam;

	if( pClusteringAtt != nullptr )
	{
		pClusteringAtt->EnterCriticalSection();
		m_ClusteringAtts = pClusteringAtt->m_Variable;
		pClusteringAtt->LeaveCriticalSection();
	}	

	// Get the data set
	SecureVariable< SFlaggedMatrix >* pDataSetMatrix = (SecureVariable< SFlaggedMatrix > *) lParam;

	if( pDataSetMatrix != nullptr )
	{
		// Note: the idea here is to use to original matrix if possible instead of
		// copying it each time calculations are executed.
		// The advantage of this approach is when the matrix is very huge and no new data is 
		// received by the module but only the parameters are changed (from GUI)
		pDataSetMatrix->EnterCriticalSection();

		if ( pDataSetMatrix->m_Variable.m_MatrixXd.rows() < 10 )
		{
			bIsOK = false;
			sReport = ANALYZER_CLUSTERING_SMALL_SET;
			pDataSetMatrix->LeaveCriticalSection();
			//AfxMessageBox(ANALYZER_CLUSTERING_SMALL_SET, MB_OK | MB_ICONEXCLAMATION);
		}
		else
		{
			// use the matrix itself
			if ( pDataSetMatrix->m_Variable.m_bFlagged == false )
			{
				bIsOK = calculate(pDataSetMatrix->m_Variable.m_MatrixXd, sReport);
				pDataSetMatrix->LeaveCriticalSection();
			}
			else // copy the matrix
			{
				MatrixXd dataSetMatrix = pDataSetMatrix->m_Variable.m_MatrixXd;
				pDataSetMatrix->m_Variable.m_bFlagged = false;
				pDataSetMatrix->LeaveCriticalSection();

				bIsOK = calculate(dataSetMatrix, sReport);
			}
		}

		//Save the results
		m_ClusteringProjectionResults.EnterCriticalSection();

		m_ClusteringProjectionResults.m_Variable.m_bIsOK = bIsOK;
		m_ClusteringProjectionResults.m_Variable.m_sMessage = sReport;

		if ( bIsOK == true)
		{
			m_ClusteringProjectionResults.m_Variable.m_MatrixXdCenters = m_FuzSam.GetProjectedCenters();
			m_ClusteringProjectionResults.m_Variable.m_MatrixXdDataSet = m_FuzSam.GetProjectedDataSet();
		}

		m_ClusteringProjectionResults.LeaveCriticalSection();

		// Update the manager that calculations are completed
		if (m_ParentThread)
		{
			m_ParentThread->PostThreadMessage(WM_AMT_CALC_COMPLETED, (WPARAM)&m_ClusteringProjectionResults, 0);
		}
	}
}

void CAnalyzerCalculatorThread::OnSaveDataSet(WPARAM wParam, LPARAM lParam)
{
	// Since this method is a handler it means that it is executed by he thread itself
	// and since m_Clustering is protected, no other thread can access it simultaneously
	// so no locking are necessary here
	bool bRet = m_Clustering.SaveDataSet();
	if (m_ParentThread)
	{
		m_ParentThread->PostThreadMessage(WM_AMT_SAVE_COMPLETED, bRet, 0);
	}
}

void CAnalyzerCalculatorThread::OnSavePartitionMatrix(WPARAM wParam, LPARAM lParam)
{
	// Since this method is a handler it means that it is executed by he thread itself
	// and since m_Clustering is protected, no other thread can access it simultaneously
	// so no locking are necessary here
	bool bRet = m_Clustering.SavePartitionMatrix();
	if (m_ParentThread)
	{
		m_ParentThread->PostThreadMessage(WM_AMT_SAVE_COMPLETED, bRet, 0);
	}
}

void CAnalyzerCalculatorThread::OnSaveProjectionMatrix(WPARAM wParam, LPARAM lParam)
{
	// Since this method is a handler it means that it is executed by he thread itself
	// and since m_FuzSam is protected, no other thread can access it simultaneously
	// so no locking are necessary here
	bool bRet = m_FuzSam.SaveProjectionMatrices();
	if (m_ParentThread)
	{
		m_ParentThread->PostThreadMessage(WM_AMT_SAVE_COMPLETED, bRet, 0);
	}
}

bool CAnalyzerCalculatorThread::calculate(MatrixXd& i_DataSetMatrix, CString& o_sReport)
{
	// Note: The calculator should not pop up UI messages but currently 
	// it is the simplest way to give the user feedback.
	// Should be changed in the future!!!

	try
	{
		// Clustering
		m_Clustering.SetTestMode(m_bTestMode);
		
		// Clustering - Initialize
		if ( m_Clustering.Initialize(i_DataSetMatrix, m_ClusteringAtts.m_uNumOfClusters,
									 m_ClusteringAtts.m_dWeightingExponenet,
									 m_ClusteringAtts.m_dTerminationTolerance,
									 m_ClusteringAtts.m_dWeightingParameter,
									 m_ClusteringAtts.m_dMaxRatio) == false )
		{
			//AfxMessageBox(ANALYZER_CLUSTERING_ERROR_INIT, MB_OK | MB_ICONEXCLAMATION);
			o_sReport = ANALYZER_CLUSTERING_ERROR_INIT;
			throw 1;
		}
		if (m_ParentThread)
		{
			m_ParentThread->PostThreadMessage(WM_ACT_CLUSTERING_INITIALIZING, 0, 0);
		}
		
		// Clustering - Calculate
		if( m_Clustering.FindClusters() == false)
		{
			//AfxMessageBox(ANALYZER_CLUSTERING_ERROR_CALC, MB_OK | MB_ICONEXCLAMATION);
			o_sReport = ANALYZER_CLUSTERING_ERROR_CALC;
			throw 1;
		}
		if (m_ParentThread)
		{
			m_ParentThread->PostThreadMessage(WM_ACT_CLUSTERING_CALCULATING, 0, 0);
		}

		// Projection
		m_FuzSam.SetTestMode(m_bTestMode);

		// Projection - Initialize
		if( m_FuzSam.Initialize(m_Clustering.GetDistanceMatrix(), m_Clustering.GetPartitionMatrix(),
								m_ClusteringAtts.m_dWeightingExponenet,
								m_ClusteringAtts.m_uDimension, m_ClusteringAtts.m_dGRadientSize,
								m_ClusteringAtts.m_uNumOfProjSteps ) == false)
		{
			//AfxMessageBox(ANALYZER_PROJECTION_ERROR_INIT, MB_OK | MB_ICONEXCLAMATION);
			o_sReport = ANALYZER_PROJECTION_ERROR_INIT;
			throw 1;
		}

		if (m_ParentThread)
		{
			m_ParentThread->PostThreadMessage(WM_ACT_PROJECTION_INITIALIZING, 0, 0);
		}

		// Projection - Calculate
		for( UINT uIndex = 0 ; uIndex < m_ClusteringAtts.m_uNumOfProjSteps && m_bStop == false ; ++uIndex )
		{
			if( m_FuzSam.StepProjection(uIndex) == false )
			{
				m_FuzSam.ReduceMemUsage();
				o_sReport = ANALYZER_PROJECTION_ERROR_CALC;
				throw 1;
			}
		
			if (m_ParentThread)
			{
				m_ParentThread->PostThreadMessage(WM_ACT_PROJECTION_STEP, 0, 0);
			}
		}
		m_FuzSam.ReduceMemUsage();

// 		if ( m_FuzSam.CompleteProjection() == false) 
// 		{
// 			//AfxMessageBox(ANALYZER_PROJECTION_ERROR_CALC, MB_OK | MB_ICONEXCLAMATION);
// 			o_sReport = ANALYZER_PROJECTION_ERROR_CALC;
// 			throw 1;
// 		}

	}
	catch (...)
	{
		TRACE(_T("Exception caught in CAnalyzerCalculatorThread::OnExecuteCalculations()"));
		_ASSERT(false);
		return false;
	}

	return true;
}