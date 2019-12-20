/*****************************************************************************                                                
*
*	Project: OptimizationDemoDlg.exe
*	
*	Description: See header file
*
*	Date	 |  Description
*	Nov 2010	Created
*	Jul 2011	Finalized
*
******************************************************************************/

#include "stdafx.h"
#include "OptimizationDemo.h"
#include "OptimizationDemoDlg.h"
#include <time.h>
#include "..\Include\AnalyzerIF.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// arbitrary values
#define MAX_RAND_RANGE	1000
#define MIN_RAND_RANGE	0

#define MAX_RAND_RANGE_RESTRICTED 50
#define MIN_RAND_RANGE_RESTRICTED 10

// CAboutDlg dialog used for App About
class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// COptimizationDemoDlg dialog
COptimizationDemoDlg::COptimizationDemoDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(COptimizationDemoDlg::IDD, pParent)
	, m_uNumOfIterations{ 0 }
	, m_uTotalIters{ 2000000 }
	, m_shortNumParams{ 4 }
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pOptUnit1 = new COptimizationExecUnit;
	m_pOptUnit2 = new COptimizationExecUnit;

	m_pThread1 = m_pThread2 = nullptr;

	m_nTimer = 0;
	
	// CAnalyzerIF is a singleton - create it via interface static method
	m_pAnalyzer = CAnalyzerIF::CreateAnalyzerIF();
}

COptimizationDemoDlg::~COptimizationDemoDlg()
{
	// Check if threads are still active
	DWORD result;

	if ( m_pThread1 != nullptr )
	{
		GetExitCodeThread(m_pThread1->m_hThread, &result);

		if (result == STILL_ACTIVE)
		{
			VERIFY( TerminateThread(m_pThread1->m_hThread, 0) != 0 );
		}
	}

	if ( m_pThread2 != nullptr )
	{
		GetExitCodeThread(m_pThread2->m_hThread, &result);

		if (result == STILL_ACTIVE)
		{
			VERIFY( TerminateThread(m_pThread2->m_hThread, 0) != 0 );
		}
	}

	if ( m_pOptUnit1 )
		delete m_pOptUnit1;

	if ( m_pOptUnit2 )
		delete m_pOptUnit2;

	// CAnalyzerIF is a singleton - delete it via interface static method
 	if ( m_pAnalyzer )
 		m_pAnalyzer->DeleteAnalyzerIF();

	m_pOptUnit1 = m_pOptUnit2 = nullptr;
 	m_pAnalyzer = nullptr;

	//_CrtDumpMemoryLeaks();
}

void COptimizationDemoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON_START_OPT_DEMO, m_cbStart);
	DDX_Control(pDX, IDC_BUTTON_STOP_OPT_DEMO, m_cbStop);
	DDX_Text(pDX, IDC_EDIT_NUM_OF_ITERATIONS, m_uNumOfIterations);
	DDX_Control(pDX, IDC_COMBO_PROCESS_TYPE, m_cbProcessType);
	DDX_Control(pDX, IDC_LIST_GEN_VALUES, m_lcGeneratedValues);
	DDX_Control(pDX, IDC_CHECK1, m_cbTestMode);
	DDX_Control(pDX, IDC_CHECK_RESTRICTED_RANGES, m_cbRestrictedRanges);
	DDX_Text(pDX, IDC_EDIT_TOTAL_ITERS, m_uTotalIters);
	DDX_Text(pDX, IDC_EDIT_IDC_PAR_ESTIM_NUM_PARAM, m_shortNumParams);
	DDX_Control(pDX, IDC_EDIT_IDC_PAR_ESTIM_NUM_PARAM, m_ceNumParams);
	DDX_Control(pDX, IDC_CHECK_FOUR_CLUSTERS_PE, m_cbFourClusters);
}

BEGIN_MESSAGE_MAP(COptimizationDemoDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_START_OPT_DEMO, &COptimizationDemoDlg::OnBnClickedButtonStartOptDemo)
	ON_BN_CLICKED(IDC_BUTTON_STOP_OPT_DEMO, &COptimizationDemoDlg::OnBnClickedButtonStopOptDemo)
	ON_COMMAND(ID_ANALYZER_OPENDIALOG, &COptimizationDemoDlg::OnAnalyzerOpenDialog)
	ON_COMMAND(ID_ANALYZER_CLOSEDIALOG, &COptimizationDemoDlg::OnAnalyzerCloseDialog)
	ON_WM_TIMER()
	ON_EN_KILLFOCUS(IDC_EDIT_IDC_PAR_ESTIM_NUM_PARAM, &COptimizationDemoDlg::OnEnKillfocusEditIdcParEstimNumParam)
	ON_BN_CLICKED(IDC_CHECK_FOUR_CLUSTERS_PE, &COptimizationDemoDlg::OnBnClickedCheckFourClustersPe)
END_MESSAGE_MAP()


// COptimizationDemoDlg message handlers

BOOL COptimizationDemoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	m_cbStart.EnableWindow(TRUE);
	m_cbStop.EnableWindow(FALSE);

	//Process Type
	m_iCurrSel = 1;
	m_cbProcessType.InsertString(0, _T("Population Treatment Optimization"));
	m_cbProcessType.InsertString(1, _T("Parameter Estimation"));
	m_cbProcessType.SetCurSel(m_iCurrSel);
	m_cbProcessType.EnableWindow(TRUE);

	//Generated values list
	m_lcGeneratedValues.InsertColumn(0, _T("Parameter ID"), LVCFMT_LEFT, 95, 0);
	m_lcGeneratedValues.InsertColumn(1, _T("Value"), LVCFMT_LEFT, 95, 0);

	m_nTimer = 0;
	m_uNumOfIterations = 0;

	UpdateData(FALSE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void COptimizationDemoDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void COptimizationDemoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR COptimizationDemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

/////////

void COptimizationDemoDlg::OnBnClickedButtonStartOptDemo()
{
	UpdateData(TRUE);

	m_cbStart.EnableWindow(FALSE);
	m_cbStop.EnableWindow(TRUE);
	m_cbProcessType.EnableWindow(FALSE);
	m_iCurrSel =  m_cbProcessType.GetCurSel();

	// Set timer to sample working thread
	m_nTimer = SetTimer(1, 10, 0);

	m_uNumOfIterations = 0;
	UpdateData(FALSE);

	bool bTestMode = m_cbTestMode.GetCheck() == BST_CHECKED ? true : false ;
	CString csProcName = (m_iCurrSel == 0) ? _T("Population Treatment Optimization") : _T("Parameter Estimation");
	m_pAnalyzer->Initialize(csProcName, bTestMode);

	//Set data to both execution objects (could be done more generic)
	m_pOptUnit1->m_uCounter = m_pOptUnit2->m_uCounter = 0;
	m_pOptUnit1->m_pAnalyzer = m_pOptUnit2->m_pAnalyzer = m_pAnalyzer;
	
	m_pOptUnit1->m_bRun = m_pOptUnit2->m_bRun = true;

	m_pOptUnit1->m_bTestMode = m_pOptUnit2->m_bTestMode = bTestMode;
	
	m_pOptUnit1->m_shortNumParams = m_shortNumParams;
	m_pOptUnit2->m_shortNumParams = m_shortNumParams;

	m_pOptUnit1->m_iCurrSel = m_iCurrSel;
	m_pOptUnit2->m_iCurrSel = m_iCurrSel;


	if ( m_cbRestrictedRanges.GetCheck() == BST_CHECKED )
	{
		m_pOptUnit1->m_iMaxRandNumber = MAX_RAND_RANGE_RESTRICTED;
		m_pOptUnit1->m_iMinRandNumber = MIN_RAND_RANGE_RESTRICTED;
		
		m_pOptUnit2->m_iMaxRandNumber = -1*MAX_RAND_RANGE_RESTRICTED;
		m_pOptUnit2->m_iMinRandNumber = -1*MIN_RAND_RANGE_RESTRICTED;
	}

	if ( m_cbFourClusters.GetCheck() == BST_CHECKED )
	{
		m_pOptUnit1->m_bFourClusters = m_pOptUnit2->m_bFourClusters = true;		
	}

	if ( m_uTotalIters % 2 == 0 )
	{
		m_pOptUnit1->m_uTotalIters = m_uTotalIters/2;
		m_pOptUnit2->m_uTotalIters = m_uTotalIters/2;
	}
	else
	{
		m_pOptUnit1->m_uTotalIters = m_uTotalIters/2 + 1;
		m_pOptUnit2->m_uTotalIters = m_uTotalIters/2;
	}

	m_pThread1 = AfxBeginThread(COptimizationExecUnit::OptimizationDemoThread, m_pOptUnit1);
	Sleep(20);
	m_pThread2 = AfxBeginThread(COptimizationExecUnit::OptimizationDemoThread, m_pOptUnit2);
}

void COptimizationDemoDlg::OnBnClickedButtonStopOptDemo()
{
	m_cbStart.EnableWindow(TRUE);
	m_cbStop.EnableWindow(FALSE);
	m_cbProcessType.EnableWindow(TRUE);

	m_pOptUnit1->m_bRun = false;
	m_pOptUnit2->m_bRun = false;

	KillTimer(m_nTimer);

	OnAnalyzerOpenDialog();
	m_pAnalyzer->Finalize();

	//Sleep(1000);
	m_uNumOfIterations = m_pOptUnit1->m_uCounter + m_pOptUnit2->m_uCounter;
	m_pThread1 = m_pThread2 = nullptr;

	UpdateData(FALSE);
}

void COptimizationDemoDlg::OnTimer(UINT nIDEvent)
{
	int iIndex = 0;
	CString str;

	// Update number of iterations 
	m_uNumOfIterations = m_pOptUnit1->m_uCounter + m_pOptUnit2->m_uCounter;

	m_lcGeneratedValues.DeleteAllItems();

	// Update list of generated values based on first thread
	if ( m_iCurrSel == 0 )//population treatment optimization
	{
		CCompositeProtocol tempCompProtocol;
		m_pOptUnit1->GetGeneratedProtocol(tempCompProtocol);

		// Go over each protocol drug
		static const MapIntDrugProtocol* pCompProtMap = nullptr;
		pCompProtMap = tempCompProtocol.GetDrugProtocolsMap();
		_ASSERT(pCompProtMap != nullptr);
		static MapIntDrugProtocol::const_iterator mapDIter, mapDIterEnd;
		mapDIter = pCompProtMap->begin(), mapDIterEnd = pCompProtMap->end();

		for( ; mapDIter != mapDIterEnd ; ++mapDIter )
		{
			//Get matrix of values
			//not safe but quick!!!!!
			static map< int, FloatVector >::const_iterator mapIter, mapIterEnd;
			mapIter = mapDIter->second.GetGeneratedParames().GetMatrix().begin();
			mapIterEnd = mapDIter->second.GetGeneratedParames().GetMatrix().end();

			for ( ; mapIter != mapIterEnd ; ++mapIter , ++iIndex)
			{
				str.Format(_T("%d"), mapIter->first);
				m_lcGeneratedValues.InsertItem(iIndex, str);
				str.Format(_T("%.6f"), (mapIter->second)[0]);
				m_lcGeneratedValues.SetItemText(iIndex, 1, str);
			}
		}
	}
	else //Parameter estimation
	{
		vector< _ParameterStruct > tempVec;
		m_pOptUnit1->GetEstimatedParameters(tempVec);

		// the vector of estimated parameters is enough here 
		static vector< _ParameterStruct >::iterator vecIter, vecIterEnd;
		vecIter = tempVec.begin(), vecIterEnd = tempVec.end();
		for ( ; vecIter != vecIterEnd ; ++vecIter, ++iIndex )
		{
			str.Format(_T("%u"), (*vecIter).m_uID);
			m_lcGeneratedValues.InsertItem(iIndex, str);
			str.Format(_T("%.6f"), (*vecIter).m_fValue);
			m_lcGeneratedValues.SetItemText(iIndex, 1, str);
		}
	}

	if ( m_uNumOfIterations == m_uTotalIters ) 
		OnBnClickedButtonStopOptDemo();

	UpdateData(FALSE);
}

void COptimizationDemoDlg::OnAnalyzerOpenDialog()
{
	if (m_pAnalyzer)
	{
		m_pAnalyzer->OpenDialog(this, 2);
	}
}

void COptimizationDemoDlg::OnAnalyzerCloseDialog()
{
	if (m_pAnalyzer)
	{
		m_pAnalyzer->CloseDialog();
	}
}

void COptimizationDemoDlg::OnEnKillfocusEditIdcParEstimNumParam()
{
	UpdateData();

	if ( m_shortNumParams < 1 || m_shortNumParams > 4 )
	{
		AfxMessageBox(_T("Please set a number between 1 and 4!"));
		m_ceNumParams.SetFocus();
	}
}

void COptimizationDemoDlg::OnBnClickedCheckFourClustersPe()
{
	if ( m_cbFourClusters.GetCheck() == BST_CHECKED )
	{
		m_shortNumParams = 2;
		m_cbRestrictedRanges.SetCheck( BST_CHECKED );
	}

	UpdateData(FALSE);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//																		//
//		"Execution" class - used by working Thread to					//
//		generates optimization-like output data							//
//																		//
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
COptimizationExecUnit::COptimizationExecUnit()
{
	::InitializeCriticalSection( &m_CriticalSection );
	m_iMaxRandNumber = MAX_RAND_RANGE;
	m_iMinRandNumber = MIN_RAND_RANGE;
	m_uCounter = 0;
	m_iCurrSel = 0;
	m_bTestMode = false;
	m_bRun = false;
	m_bFourClusters = false;
	m_byteFourClustersFlip = 0;
	m_uTotalIters = 0;
	m_uCounter = 0;
	m_shortNumParams = 0;
	m_pAnalyzer = nullptr;

	m_pParameterSet = new CParameterSet;
}

COptimizationExecUnit::~COptimizationExecUnit()
{
	if ( m_pParameterSet )
		delete m_pParameterSet;

	m_pParameterSet = nullptr;

	::DeleteCriticalSection( &m_CriticalSection );
}

void COptimizationExecUnit::GetGeneratedProtocol(CCompositeProtocol& o_refCOmpProtocol)
{
	::EnterCriticalSection( &m_CriticalSection );
	o_refCOmpProtocol = m_CompProtocol;
	::LeaveCriticalSection( &m_CriticalSection );
}

void COptimizationExecUnit::GetEstimatedParameters(vector< _ParameterStruct >& o_refVecEstimParams)
{
	::EnterCriticalSection( &m_CriticalSection );
	o_refVecEstimParams = m_vecEstimParams;
	::LeaveCriticalSection( &m_CriticalSection );
}

UINT COptimizationExecUnit::OptimizationDemoThread(LPVOID pParam)
{
	COptimizationExecUnit* pThis = (COptimizationExecUnit*)pParam;
	pThis->startOptimizationDemo();
	return 0;
}

void COptimizationExecUnit::startOptimizationDemo()
{
	if(m_bTestMode == true)
	{
		srand(1);
	}
	else
	{
		// change the seed for rand()
		srand( (unsigned)time( nullptr ) );
	}
	
	resetDataStructures();
	initializeDataStructures();

	vector<double> vecDataSet;
	
	m_uCounter = 0;
	while( m_bRun && m_uCounter < m_uTotalIters )
	{
		// Population Treatment optimization
		m_iCurrSel == 0 ? generatePTOParams() : 0 ;

		// Parameter Estimation
		m_iCurrSel == 1 ? generateEstimParams() : 0 ;
		
		if ( m_pParameterSet )
		{
			extractData(*m_pParameterSet, vecDataSet);
			m_pAnalyzer->SetData(vecDataSet);
		}

		// Counter is used by GUI to update the number of iteration so far (shows progress)
		m_uCounter++;
		
		// Change number of milliseconds to control loop "speed"
		Sleep(1);
	}

	// The thread will be terminated automatically when it will exit the loop (due to m_bRun = false)
}

void COptimizationExecUnit::generatePTOParams()
{
	CDrugProtocol tempProtocol;
	CCompositeProtocol tempCompProtocol;
	map< int, FloatVector >::iterator mapIter , mapIterEnd = m_mapProtParams.end();

	// For the sack of this demo, we know and assume only two "known drugs"
	for ( int i = 0 ; i < 2 ; ++i )
	{
		for ( mapIter = m_mapProtParams.begin() ; mapIter != mapIterEnd ; ++mapIter )
		{
			// Dangerous but we know the vector is of size 1 
			(mapIter->second)[0] = getRandomNumber();		
		}

		// Add the space into Protocol data structure
		tempProtocol.SetGeneratedSet(m_mapProtParams);

		tempCompProtocol.InsertDrugProtocol( i == 0 ? DRUG1 : DRUG2, tempProtocol);
	}

	::EnterCriticalSection( &m_CriticalSection );
	m_CompProtocol = tempCompProtocol;
	::LeaveCriticalSection( &m_CriticalSection );

	// Set composite protocol to parameter Set
	if ( m_pParameterSet )
		m_pParameterSet->m_CompositeProtocol = tempCompProtocol;
}

void COptimizationExecUnit::generateEstimParams()
{
	::EnterCriticalSection( &m_CriticalSection );
	vector< _ParameterStruct > tempVec = m_vecEstimParams;
	::LeaveCriticalSection( &m_CriticalSection );

	vector< _ParameterStruct >::iterator vecIter = tempVec.begin(), vecIterEnd = tempVec.end();

	for ( ; vecIter != vecIterEnd ; ++vecIter )
	{
		(*vecIter).m_fValue = getRandomNumber();
	}
	
	// This section is responsible for generating "4 clusters" sets
	// It assumes there are at least 2 parameters and every second iteration
	// it changes the sign of the second parameter
	if ( m_bFourClusters == true )
	{
		if ( m_byteFourClustersFlip == 1 )
			tempVec[0].m_fValue *= -1.0;

		m_byteFourClustersFlip = !m_byteFourClustersFlip;
	}

	::EnterCriticalSection( &m_CriticalSection );
	m_vecEstimParams = tempVec;
	::LeaveCriticalSection( &m_CriticalSection );

	// Set vector of estimated parameters to parameter Set 
	if ( m_pParameterSet )
		m_pParameterSet->m_vecOptimizedParameterStructs = tempVec;
}

inline float COptimizationExecUnit::getRandomNumber()
{
	// Generate random numbers in the half-closed interval [range_min, range_max).
	// In other words, range_min <= random number < range_max
	return (float)rand() / (RAND_MAX + 1) * (m_iMaxRandNumber - m_iMinRandNumber) + m_iMinRandNumber;
}

void COptimizationExecUnit::initializeDataStructures()
{
	// Population Treatment Optimization
	// Lets assume our space includes 7 parameters
	m_mapProtParams[PARAM1] = vector<float>(1,0.0f);
	m_mapProtParams[PARAM2] = vector<float>(1,0.0f);
	m_mapProtParams[PARAM3] = vector<float>(1,0.0f);
	m_mapProtParams[PARAM4] = vector<float>(1,0.0f);
	m_mapProtParams[PARAM5] = vector<float>(1,0.0f);
	m_mapProtParams[PARAM6] = vector<float>(1,0.0f);
	m_mapProtParams[PARAM7] = vector<float>(1,0.0f);

	// Create a drug protocol
	CDrugProtocol tempDrugProtocol;
	tempDrugProtocol.SetGeneratedSet(m_mapProtParams);

	// Insert the drug protocol for each drug
	// For the sack of this demo, we assume only two "known drugs"
	::EnterCriticalSection( &m_CriticalSection );
	m_CompProtocol.ClearAllDrugProtocols();
	m_CompProtocol.InsertDrugProtocol(DRUG1, tempDrugProtocol);
	m_CompProtocol.InsertDrugProtocol(DRUG2, tempDrugProtocol);
	::LeaveCriticalSection( &m_CriticalSection );

	// Parameter Estimation
	m_vecEstimParams.clear();
	_ParameterStruct structParam;

	for ( short i = 0 ; i < m_shortNumParams ; ++i )
	{
		structParam.m_uID = PARAM10+i;
		structParam.m_fValue = NO_VALUE;
		m_vecEstimParams.push_back( structParam );
	}
}

void COptimizationExecUnit::resetDataStructures()
{
	// Reset all values in the map to 0.0f
	// For the sack of this demo, we assume only two "known drugs"

	m_CompProtocol.ResetGeneratedParams();

	// Reset all values in the vector to NO_VALUE
	vector< _ParameterStruct >::iterator vecIter = m_vecEstimParams.begin(), vecIterEnd = m_vecEstimParams.end();
	for ( ; vecIter != vecIterEnd ; ++vecIter )
	{
		(*vecIter).m_fValue = NO_VALUE;
	}

	if ( m_pParameterSet )
		m_pParameterSet->Clear();
}

void COptimizationExecUnit::extractData(const CParameterSet& i_pParamSet, vector<double>& o_vecDataSet)
{
	o_vecDataSet.clear();
	o_vecDataSet.reserve(20);

	if( m_iCurrSel == 0 )
	{
		extractDataSet(i_pParamSet.m_CompositeProtocol, o_vecDataSet);
	}
	else
	{
		extractDataSet(i_pParamSet.m_vecOptimizedParameterStructs, o_vecDataSet);
	}
}

void COptimizationExecUnit::extractDataSet(const std::vector<_ParameterStruct>& i_vParameterStruct, vector<double>& o_vecDataSet)
{
	std::vector<_ParameterStruct>::const_iterator vecIter = i_vParameterStruct.begin(), vecIterEnd = i_vParameterStruct.end();

	for(; vecIter != vecIterEnd ; ++vecIter)
	{
		o_vecDataSet.push_back((*vecIter).m_fValue);
	}
}

void COptimizationExecUnit::extractDataSet(const CCompositeProtocol& i_comProtocol, vector<double>& o_vecDataSet)
{
	const MapIntDrugProtocol& refMapIntDrugProtocol = *(i_comProtocol.GetDrugProtocolsMap());

	//defining iterations for next three for loops
	MapIntDrugProtocol::const_iterator mapIterDrugs = refMapIntDrugProtocol.begin(), mapIterDrugsEnd = refMapIntDrugProtocol.end();
	map<int, FloatVector>::const_iterator mapIterVals, mapIterValsEnd;
	vector<float>::const_iterator vecIter, vecIterEnd;

	for ( ; mapIterDrugs != mapIterDrugsEnd ; ++mapIterDrugs )
	{
		const CTimeSeries& cCTimeSeries = (*mapIterDrugs).second.GetGeneratedParames();

		mapIterVals = cCTimeSeries.GetMatrix().begin();
		mapIterValsEnd = cCTimeSeries.GetMatrix().end();

		for( ; mapIterVals != mapIterValsEnd ; ++mapIterVals )
		{
			vecIter = (*mapIterVals).second.begin();
			vecIterEnd = (*mapIterVals).second.end();

			for ( ; vecIter != vecIterEnd ; ++vecIter )
			{
				o_vecDataSet.push_back((double)(*vecIter));
			}
		}			
	}
}