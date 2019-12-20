/**************************************************************************************
*
*	Description: The file includes different definitions Dummy class just for testing.
*				 Will be eliminated or changed in the future
*	
*
*	Date     |  Description
*	Nov 2010	Created
*	Jul 2011	Added enumerations to enProgressStatus
*										
***************************************************************************************/
#pragma once
#include <vector>

using namespace std;

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#define NOMINMAX
#include "../Eigen/Core"
using namespace Eigen;

//Analyzer Manager Thread (AMT) messages
//WM_APP+X where X=[200,499]
#define WM_AMT_INITIALIZE					WM_APP+200
#define WM_AMT_FINALIZE						WM_APP+201
#define WM_AMT_SET_DATA				    	WM_APP+202
#define WM_AMT_OPEN_DIALOG					WM_APP+203
#define WM_AMT_CLOSE_DIALOG					WM_APP+204
#define WM_AMT_CALC_COMPLETED				WM_APP+205 //Message from Calculator to manager
#define WM_AMT_SAVE_COMPLETED				WM_APP+206
#define	WM_AMT_SAVE_DATA_SET				WM_APP+207
#define	WM_AMT_SAVE_PARTITION_MATRIX		WM_APP+208
#define	WM_AMT_SAVE_PROJECTION_MATRIX		WM_APP+209
#define	WM_AMT_STOP_CALCULATIONS			WM_APP+210

//Analyzer Calculator Thread (ACT) messages 
//WM_APP+X where X=[500,699]
#define	WM_ACT_EXEC_CALCULATIONS			WM_APP+500
#define WM_ACT_CLUSTERING_INITIALIZING		WM_APP+501
#define WM_ACT_CLUSTERING_CALCULATING		WM_APP+502
#define WM_ACT_PROJECTION_INITIALIZING		WM_APP+503
#define WM_ACT_PROJECTION_STEP				WM_APP+504

// The class attaches a critical section to a data structure but does not force the user to use it.
// It is the user's responsibility to enter a the critical section before accessing the 
// data structure and leave the critical section when finished
// The class can be extended in the future
template< typename T >
class SecureVariable
{
public:
	SecureVariable() 
	{
		::InitializeCriticalSection( &m_CriticalSection );
	}

	~SecureVariable()
	{
		::DeleteCriticalSection( &m_CriticalSection );
	}

	void EnterCriticalSection()
	{
		::EnterCriticalSection( &m_CriticalSection );
	}

	void LeaveCriticalSection()
	{
		::LeaveCriticalSection( &m_CriticalSection );
	}

	T					m_Variable;
	CRITICAL_SECTION	m_CriticalSection;
};

struct SFlaggedMatrix
{
	SFlaggedMatrix() noexcept : m_bFlagged(false) {}

	MatrixXd	m_MatrixXd;
	bool		m_bFlagged;
};

struct SClusterProjectionResult
{
	SClusterProjectionResult() noexcept
	{
		m_bIsOK = true;
	}

	MatrixXd	m_MatrixXdDataSet;
	MatrixXd	m_MatrixXdCenters;
	bool		m_bIsOK;
	CString		m_sMessage;	
};

// Currently the structure includes parameters of both clustering and projection
struct SClustringAttributes
{
	UINT	m_uNumOfClusters;
	double	m_dWeightingExponenet;
	double	m_dWeightingParameter;
	double	m_dTerminationTolerance;
	double	m_dMaxRatio;
	UINT	m_uDimension;
	double	m_dGRadientSize;
	UINT	m_uNumOfProjSteps;
};


enum enProgressStatus
{
	enProgressInitialize = 0,
	enClusteringInitializing,
	enClusteringCalculating,
	enProjectionInitializing,
	enProjectionStepping,
	enProgressFinalize
};