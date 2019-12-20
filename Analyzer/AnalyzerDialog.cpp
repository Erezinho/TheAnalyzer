/**************************************************************************************
*
*	Description: See header file
*
*	Date     |  Description
*	Nov 2010	Created
*	May 2011	Renamed OnBnClickedButtonClean to OnBnClickedButtonStep
*	Jul 2011	Changed HH_MM_SS
*				Added m_pBitmap
*				OpenDialog() - Added call to graphDrawOffScreenGraph()
*				FinishCalculations() - Added call to graphDrawOffScreenGraph()
*				Changed UpdateProgress
*				Removed Invalidate from graphBuildGraphicDataSets()
*				dlgSetComponents() - Added calls to UpdateProgress and dlgSetTimes
*				Changed dlgSetTimes argument type
*										
***************************************************************************************/

#include "stdafx.h"
#include "AnalyzerDialog.h"
#include "Messages.h"

#define GRAPH_MARGIN	6.0
#define PEN_SIZE		0.5
#define STRING_SIZE		11
#define GRAPH_X_OFFSET	15
#define GRAPH_Y_OFFSET	10
#define GRAPH_FONT_SIZE	10

#define DEFAULT_NUM_CLUSTERS						2
#define DEFAULT_WEIGHTING_EXPONENT					2.0
#define DEFAULT_WEIGHTING_PARAMETER__GAMMA			0.2		//Gamma
#define DEFAULT_TERMINATION_TOLERANCE__EPSILON		0.001	//Epsilon
#define DEFAULT_MAX_RATIO							1e15	//Beta
#define DEFAULT_DIMENSION							2
#define DEFAULT_GRADIENT_SIZE						0.4
#define DEFAULT_NUM_OF_PROJECTION_STEPS				2000

#define TIMES_NEW_ROMAN	L"Times New Roman"
#define ARIAL			L"Arial"

#define TYPE1			1
#define TYPE2			2

#define _BLACK			Gdiplus::Color::MakeARGB(255,0,0,0)
#define _WHITE			Gdiplus::Color::MakeARGB(255,255,255,255)
#define _LIGTHGRAY		Gdiplus::Color::MakeARGB(255,215,215,215)
#define _BLUE			Gdiplus::Color::MakeARGB(255,0,0,225)
#define _RED			Gdiplus::Color::MakeARGB(255,225,0,0)

#define ANALYZER_DLG_TIMER_EVENT_ID		10
#define ANALYZER_TIMER_MILLISEC			500
#define ANALYZER_DIALOG_TITLE			_T("Analyzer Dialog")

#define STATUS_READY					_T("Ready!")
// #define STATUS_PROCESSING_PARTIAL		_T("Processing (partial set)...")
// #define STATUS_PROCESSING_COMPLETE		_T("Processing (complete set)...")
#define STATUS_PROCESSING_MESSAGE1		_T("Processing... please be patient!")
#define STATUS_PROCESSING_MESSAGE2		_T("Still Processing... and it's hard... patience rules!!")
#define STATUS_PROCESSING_MESSAGE3		_T("One hell of a set... still working... keep patience!!!")
#define STATUS_CLUSTERING_INITIALIZING	_T("Clustering: Initialization...")
#define STATUS_CLUSTERING_CALCULATING	_T("Clustering: Calculating...")
#define STATUS_PROJECTION_INITIALIZING	_T("Projection: Initialization...")
#define STATUS_PROJECTION_STEP			_T("Projection: Calculating...")
#define STATUS_STOPPING_CALCS			_T("Stopping calculations...!")

#define PROGRESS_BAR_MIN_POS			0
#define PROGRESS_BAR_MAX_POS			10
#define PROGRESS_BAR_STEP_SIZE			1

#define HH_MM_SS						_T("00:00:00")

// CAnalyzerDialog dialog

IMPLEMENT_DYNAMIC(CAnalyzerDialog, CDialog)

CAnalyzerDialog::CAnalyzerDialog(CWnd* pParent /*= NULL*/)
	: CDialog(CAnalyzerDialog::IDD, pParent),
	m_iProcessID(0),
	m_bIsDialogCreated(false),
	m_sDialogTitle(ANALYZER_DIALOG_TITLE),
	m_sProcessName(_T("")),
	m_ParentThread(nullptr),
	m_uNumOfClusters(DEFAULT_NUM_CLUSTERS),
	m_dWeightingExponenet(DEFAULT_WEIGHTING_EXPONENT),
	m_dWeightingParameter(DEFAULT_WEIGHTING_PARAMETER__GAMMA),
	m_dTerminationTolerance(DEFAULT_TERMINATION_TOLERANCE__EPSILON),
	m_dMaxRatio(DEFAULT_MAX_RATIO),
	m_uDimension(DEFAULT_DIMENSION), 
	m_dGRadientSize(DEFAULT_GRADIENT_SIZE),
	m_uNumOfProjSteps(DEFAULT_NUM_OF_PROJECTION_STEPS),
	m_bIsTestMode(false),
	m_bIsCalculating(false),
	m_csTotalTime(HH_MM_SS),
	m_csEstimatedTime(HH_MM_SS),
	m_uLastEstimatedTime(0),
	m_uNumItersSoFar(0),
	m_pBitmap(nullptr)
{
	m_timeStartGeneral = m_timeStartProjection = m_timeFinish = CTime::GetCurrentTime();

	setClusteringAttributes();

	// Initialize GDI+.
	GdiplusStartupInput gdiplusStartupInput;
	VERIFY( Ok == GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, nullptr) );
	m_GraphicsPath = new GraphicsPath;
}

CAnalyzerDialog::~CAnalyzerDialog()
{
	delete m_pBitmap;
	delete m_GraphicsPath;
	GdiplusShutdown(m_gdiplusToken);
	DestroyWindow();
}

void CAnalyzerDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_PROCESS_TYPE, m_ceProcessType);
	DDX_Text(pDX, IDC_EDIT_NUMBER_OF_CLUSTERS, m_uNumOfClusters);
	DDX_Text(pDX, IDC_EDIT_WEIGHTING_EXPONENT, m_dWeightingExponenet);
	DDX_Text(pDX, IDC_EDIT_WEIGHTING_PARAMETER, m_dWeightingParameter);
	DDX_Text(pDX, IDC_EDIT_TERMINATION_TOLERANCE, m_dTerminationTolerance);
	DDX_Text(pDX, IDC_EDIT_MAX_RATIO, m_dMaxRatio);
	DDX_Text(pDX, IDC_EDIT_DIMENSION, m_uDimension);
	DDX_Text(pDX, IDC_EDIT_GRADIENT_SIZE, m_dGRadientSize);
	DDX_Text(pDX, IDC_EDIT_NUM_OF_PROJ_STEPS, m_uNumOfProjSteps);
	DDX_Text(pDX, IDC_TOTAL_TIME, m_csTotalTime);
	DDX_Text(pDX, IDC_ESTIMATED_TIME, m_csEstimatedTime);
	DDX_Control(pDX, IDOK, m_cbOK);
	DDX_Control(pDX, IDCANCEL, m_cbCancel);
	DDX_Control(pDX, IDC_GRAPH_CONTROL, m_PicCtrl);
	DDX_Control(pDX, IDC_BUTTON_EXECUTE_CLUSTERING, m_cbExecuteClustering);
	DDX_Control(pDX, IDC_COMBO_CLUSTERING_ALG, m_cbClusteringAlg);
	DDX_Control(pDX, IDC_COMBO_PROJECTION_ALG, m_cbProjectionAlg);
	DDX_Control(pDX, IDC_EDIT_STATUS, m_ceStatus);
	DDX_Control(pDX, IDC_EDIT_SIZE_CLUSTERED_SET, m_ceSizeOfClusteredSet);
	DDX_Control(pDX, IDC_BUTTON_SAVE_DATA_SET, m_cbSaveDataSet);
	DDX_Control(pDX, IDC_BUTTON_SAVE_PARTITION_MATRIX, m_cbSavePartitionMatrix);
	DDX_Control(pDX, IDC_BUTTONPROJECTION_MATRIX, m_cbSaveProjectionMatrix);
	DDX_Control(pDX, IDC_BUTTON_STOP, m_cbStopCalculations);
	DDX_Control(pDX, IDC_PROGRESS_CALCS, m_pcProgressCalcs);
	DDX_Control(pDX, IDC_CHECK_ANALYZE_AUTOMATICALLY, m_cbAnalyzeAutomatically);
}

// Modeless functionality methods
BOOL CAnalyzerDialog::OpenDialog(CWnd* i_pMainWnd, int i_iProcessID) // = Create()
{
	BOOL bRet = FALSE;

	m_pParentWnd = i_pMainWnd;
	m_iProcessID = i_iProcessID;

	if ( m_bIsDialogCreated == false )
	{
		// Use GetDesktopWindow() to acquire "regular" window behavior
		bRet = CDialog::Create(CAnalyzerDialog::IDD, GetDesktopWindow()); 

		//Set position on screen next to reference window
		CRect cRefWndRec(0,0,0,0);

		if( m_pParentWnd )
		{
			m_pParentWnd->GetWindowRect(&cRefWndRec);
			cRefWndRec.right += 1;
		}
		SetWindowPos(nullptr, cRefWndRec.right, cRefWndRec.top , 0, 0, SWP_NOSIZE | SWP_NOZORDER); // 2nd arg = left side , 3rd arg = top

		// Already after OnInitDialog()
		m_bIsDialogCreated = true;

		dlgSetComponents();

		dlgUpdateDialogGUI();

		// Call those two methods in order to draw existing data (arrived before the dialog has been opened) 
		// or to draw just axes and grid (no data at this point)
		graphBuildGraphicDataSets();
		graphDrawOffScreenGraph();
	}

	ShowWindow(SW_SHOW | SW_RESTORE);

	return bRet;
}

void CAnalyzerDialog::CloseDialog()
{
	if ( m_bIsDialogCreated == true)
		OnBnClickedOk();
}

BOOL CAnalyzerDialog::OnInitDialog()
{
	BOOL bRet = CDialog::OnInitDialog();

	// Initialize data members
	m_uNumOfClusters		= DEFAULT_NUM_CLUSTERS;
	m_dWeightingExponenet	= DEFAULT_WEIGHTING_EXPONENT;
	m_dWeightingParameter	= DEFAULT_WEIGHTING_PARAMETER__GAMMA;		//Gamma
	m_dTerminationTolerance = DEFAULT_TERMINATION_TOLERANCE__EPSILON;	//Epsilon
	m_dMaxRatio				= DEFAULT_MAX_RATIO;		//Beta
	m_uDimension			= DEFAULT_DIMENSION; 
	m_dGRadientSize			= DEFAULT_GRADIENT_SIZE;
	m_uNumOfProjSteps		= DEFAULT_NUM_OF_PROJECTION_STEPS;

	m_cbClusteringAlg.SetCurSel(0);
	m_cbProjectionAlg.SetCurSel(0);

	//m_pcProgressCalcs.SetRange(PROGRESS_BAR_MIN_POS, PROGRESS_BAR_MAX_POS);
	m_pcProgressCalcs.SetStep(PROGRESS_BAR_STEP_SIZE);
	
	m_cbAnalyzeAutomatically.SetCheck( BST_CHECKED );
	
	// change availability of those particular buttons
	m_cbSaveDataSet.EnableWindow(FALSE);
	m_cbSavePartitionMatrix.EnableWindow(FALSE);
	m_cbSaveProjectionMatrix.EnableWindow(FALSE);
	m_cbStopCalculations.EnableWindow(FALSE);

	// Initialize graphic area size
	CRect ClientRec;
	m_PicCtrl.GetClientRect(&ClientRec);
	m_dXSize = ClientRec.Width()/2.0;
	m_dYSize = ClientRec.Height()/2.0;

	m_pBitmap = new Bitmap(ClientRec.right,ClientRec.bottom);

	// Set default maximum values to 0.5 in order to draw
	// default grid in case dialog is opened before there are results
	m_structMinMax.m_dMaxX = 0.5;
	m_structMinMax.m_dMaxY = 0.5;

	graphSetScale();
	
	return bRet;
}

void CAnalyzerDialog::Initialize(const CString& i_refProcessTitle, bool i_bTestMode)
{
	m_sProcessName = i_refProcessTitle;
	m_bIsTestMode = i_bTestMode;

	dlgSetComponents();
}

void CAnalyzerDialog::SetParentThread(CWinThread* i_pParentThread)
{
	m_ParentThread = i_pParentThread;
}

void CAnalyzerDialog::SavingCompletionReport(bool i_bIsOK)
{
	dlgSetControlsAccessibility();
	if ( i_bIsOK == true )
		AfxMessageBox(ANALYZER_SAVE_SUCCESSFULY, MB_OK | MB_ICONINFORMATION);
	else
		AfxMessageBox(ANALYZER_SAVE_FAILED, MB_OK | MB_ICONEXCLAMATION);
}

Status CAnalyzerDialog::FinishCalculations(WPARAM wParam, LPARAM lParam)
{
	Status sRet = Ok;
	try
	{
		// Calculations might have been stopped 
		if ( m_bIsCalculating == true )
		{
			bool bIsResultsOK = true;
			CString sReport;
			SecureVariable<SClusterProjectionResult>* pResults = (SecureVariable<SClusterProjectionResult> *) wParam;

			pResults->EnterCriticalSection();
			m_ProjCentersMatrix = pResults->m_Variable.m_MatrixXdCenters;
			m_ProjDataSetMatrix = pResults->m_Variable.m_MatrixXdDataSet;
			bIsResultsOK = pResults->m_Variable.m_bIsOK;
			sReport = pResults->m_Variable.m_sMessage;
			pResults->LeaveCriticalSection();
			
			m_bIsCalculating = false;

			if( bIsResultsOK == true )
			{
				graphBuildGraphicDataSets();
				graphDrawOffScreenGraph();
			}
			else
			{
				AfxMessageBox(sReport, MB_OK | MB_ICONEXCLAMATION);
			}
		}

		if( m_bIsDialogCreated == true )
		{
			//onStopTimer();
			dlgSetTimes(enTimeFinish);
			UpdateProgress(enProgressFinalize);
			dlgSetClusteredSetSize();
		}

		dlgSetControlsAccessibility();
	}
	catch (...)
	{
		TRACE(_T("Exception caught in CAnalyzerDialog::UpdateProjMatrix()"));
		_ASSERT(nullptr);
		sRet = GenericError;
	}
	return sRet;
}

void CAnalyzerDialog::UpdateProgress(enProgressStatus i_enStatus)
{
	LONGLONG lSecondsSoFar = 0;

	if ( i_enStatus == enProjectionInitializing )
	{
		m_timeStartProjection = CTime::GetCurrentTime();
	}
	else if ( i_enStatus == enProjectionStepping )
	{
		m_uNumItersSoFar++;
		lSecondsSoFar = dlgSetTimes(enTimeProgress);
	}

	if ( m_bIsDialogCreated != true ) //|| m_bIsCalculating != true )
		return;

	if ( i_enStatus == enClusteringInitializing )
	{
		m_ceStatus.SetWindowText(STATUS_CLUSTERING_INITIALIZING);
	}
	else if ( i_enStatus == enClusteringCalculating )
	{
		m_ceStatus.SetWindowText(STATUS_CLUSTERING_CALCULATING);
	}
	else if ( i_enStatus == enProjectionInitializing )
	{
		m_ceStatus.SetWindowText(STATUS_PROJECTION_INITIALIZING);
	}
	else if ( i_enStatus == enProjectionStepping )
	{
		if( lSecondsSoFar <= 30 )
		{
			m_ceStatus.SetWindowText(STATUS_PROJECTION_STEP);
		}
		else if ( lSecondsSoFar > 30 && lSecondsSoFar <= 90 )
		{
			m_ceStatus.SetWindowText(STATUS_PROCESSING_MESSAGE1);
		}

		else if ( lSecondsSoFar > 90 && lSecondsSoFar <= 180 )
		{
			m_ceStatus.SetWindowText(STATUS_PROCESSING_MESSAGE2);
		}
		else if ( lSecondsSoFar > 180)
		{
			m_ceStatus.SetWindowText(STATUS_PROCESSING_MESSAGE3);
		}
	}
	else if ( i_enStatus ==enProgressFinalize )
	{
		m_ceStatus.SetWindowText(STATUS_READY);
	}
	
	m_pcProgressCalcs.SetPos((int)m_uNumItersSoFar);
}

void CAnalyzerDialog::Finalize()
{
	if ( m_cbAnalyzeAutomatically.GetCheck() == BST_CHECKED )
		executeCalculations();
}

BEGIN_MESSAGE_MAP(CAnalyzerDialog, CDialog)
	//ON_WM_TIMER()
	ON_WM_PAINT()
	ON_BN_CLICKED(IDOK, &CAnalyzerDialog::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CAnalyzerDialog::OnBnClickedCancel)
	ON_EN_KILLFOCUS(IDC_EDIT_NUMBER_OF_CLUSTERS, &CAnalyzerDialog::OnEnKillfocusEditNumberOfClusters)
	ON_BN_CLICKED(IDC_BUTTON_EXECUTE_CLUSTERING, &CAnalyzerDialog::OnBnClickedButtonExecuteClustering)
	ON_EN_KILLFOCUS(IDC_EDIT_WEIGHTING_EXPONENT, &CAnalyzerDialog::OnEnKillfocusEditWeightingExponent)
	ON_EN_KILLFOCUS(IDC_EDIT_MAX_RATIO, &CAnalyzerDialog::OnEnKillfocusEditMaxRatio)
	ON_EN_KILLFOCUS(IDC_EDIT_WEIGHTING_PARAMETER, &CAnalyzerDialog::OnEnKillfocusEditWeightingParameter)
	ON_EN_KILLFOCUS(IDC_EDIT_TERMINATION_TOLERANCE, &CAnalyzerDialog::OnEnKillfocusEditTerminationTolerance)
	ON_EN_KILLFOCUS(IDC_EDIT_DIMENSION, &CAnalyzerDialog::OnEnKillfocusEditDimension)
	ON_EN_KILLFOCUS(IDC_EDIT_GRADIENT_SIZE, &CAnalyzerDialog::OnEnKillfocusEditGradientSize)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_DATA_SET, &CAnalyzerDialog::OnBnClickedButtonSaveDataSet)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_PARTITION_MATRIX, &CAnalyzerDialog::OnBnClickedButtonSavePartitionMatrix)
	ON_BN_CLICKED(IDC_BUTTONPROJECTION_MATRIX, &CAnalyzerDialog::OnBnClickedButtonSaveProjectionMatrix)
	ON_EN_KILLFOCUS(IDC_EDIT_NUM_OF_PROJ_STEPS, &CAnalyzerDialog::OnEnKillfocusEditNumOfProjSteps)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CAnalyzerDialog::OnBnClickedButtonStopCalculations)
END_MESSAGE_MAP()

// CAnalyzerDialog message handlers

void CAnalyzerDialog::OnPaint()
{
	CDialog::OnPaint();
	graphDrawGraph();
}

void CAnalyzerDialog::OnBnClickedOk()
{
	// Do not destroy the window yet. Leave it alive but just hide it.
	ShowWindow(SW_HIDE);
	m_bIsDialogCreated = true; // Still opened
}

void CAnalyzerDialog::OnBnClickedCancel()
{
	// Do not destroy the window yet. Leave it alive but just hide it.
	ShowWindow(SW_HIDE);
	m_bIsDialogCreated = true; // Still opened
}

void CAnalyzerDialog::OnEnKillfocusEditNumberOfClusters()
{
	UINT uCurrVal = m_uNumOfClusters;

	UpdateData();

	if( m_uNumOfClusters <= 1 )
	{
		AfxMessageBox(_T("Invalid Number of clusters!\n\nNumber of clusters must be an integer grather than 1!\nPlease set a valid value."));

		m_uNumOfClusters = uCurrVal;
		UpdateData(FALSE);

		CWnd* pWnd = GetDlgItem(IDC_EDIT_NUMBER_OF_CLUSTERS);
		pWnd->SetFocus();		
	}
}

void CAnalyzerDialog::OnEnKillfocusEditWeightingExponent()
{
	double dCurrVal = m_dWeightingExponenet;

	UpdateData();

	if( m_dWeightingExponenet < 2.0 )
	{
		AfxMessageBox(_T("Invalid Weighting exponent!\n\nWeighting exponent must be grather than/equal to 2.0!\nPlease set a valid value."));

		m_dWeightingExponenet = dCurrVal;
		UpdateData(FALSE);

		CWnd* pWnd = GetDlgItem(IDC_EDIT_WEIGHTING_EXPONENT);
		pWnd->SetFocus();		
	}
}

void CAnalyzerDialog::OnEnKillfocusEditMaxRatio()
{
	double dCurrVal = m_dMaxRatio;

	UpdateData();

	if( m_dMaxRatio <= 0.0 )
	{
		AfxMessageBox(_T("Invalid Max ratio!\n\nMax ratio must be grather than 0.0!\nPlease set a valid value."));

		m_dMaxRatio = dCurrVal;
		UpdateData(FALSE);

		CWnd* pWnd = GetDlgItem(IDC_EDIT_MAX_RATIO);
		pWnd->SetFocus();		
	}
}

void CAnalyzerDialog::OnEnKillfocusEditWeightingParameter()
{
	double dCurrVal = m_dWeightingParameter;

	UpdateData();

	if( m_dWeightingParameter < 0.0 || m_dWeightingParameter > 1.0)
	{
		AfxMessageBox(_T("Invalid Weighting parameter!\n\nWeighting parameter must be between 0.0 and 1.0!\nPlease set a valid value."));

		m_dWeightingParameter = dCurrVal;
		UpdateData(FALSE);

		CWnd* pWnd = GetDlgItem(IDC_EDIT_WEIGHTING_PARAMETER);
		pWnd->SetFocus();		
	}
}

void CAnalyzerDialog::OnEnKillfocusEditTerminationTolerance()
{
	double dCurrVal = m_dTerminationTolerance;

	UpdateData();

	if( m_dTerminationTolerance <= 0.0 )
	{
		AfxMessageBox(_T("Invalid Termination tolerance!\n\nTermination tolerance must be grather than 0.0!\nPlease set a valid value."));

		m_dTerminationTolerance = dCurrVal;
		UpdateData(FALSE);

		CWnd* pWnd = GetDlgItem(IDC_EDIT_TERMINATION_TOLERANCE);
		pWnd->SetFocus();		
	}
}

void CAnalyzerDialog::OnEnKillfocusEditDimension()
{
	UINT uCurrVal = m_uDimension;

	UpdateData();

	if( m_uDimension != 2.0 )
	{
		AfxMessageBox(_T("Invalid Dimension!\n\nDimension must be exactly 2!\nPlease set a valid value."));

		m_uDimension = uCurrVal;
		UpdateData(FALSE);

		CWnd* pWnd = GetDlgItem(IDC_EDIT_DIMENSION);
		pWnd->SetFocus();		
	}
}

void CAnalyzerDialog::OnEnKillfocusEditGradientSize()
{
	double dCurrVal = m_dGRadientSize;

	UpdateData();

	if( m_dGRadientSize <= 0.0 )
	{
		AfxMessageBox(_T("Invalid Gradient size!\n\nGradient size must be an grather than 0.0!\nPlease set a valid value."));

		m_dGRadientSize = dCurrVal;
		UpdateData(FALSE);

		CWnd* pWnd = GetDlgItem(IDC_EDIT_GRADIENT_SIZE);
		pWnd->SetFocus();		
	}
}

void CAnalyzerDialog::OnEnKillfocusEditNumOfProjSteps()
{
	UINT uCurrVal = m_uNumOfProjSteps;

	UpdateData();

	if( m_uNumOfProjSteps < 0 )
	{
		AfxMessageBox(_T("Invalid number of iterations!\n\nNumber of projection iterations must be positive integer!\nPlease set a valid value."));

		m_uNumOfProjSteps = uCurrVal;
		UpdateData(FALSE);

		CWnd* pWnd = GetDlgItem(IDC_EDIT_NUM_OF_PROJ_STEPS);
		pWnd->SetFocus();		
	}
}

void CAnalyzerDialog::OnBnClickedButtonExecuteClustering()
{
	executeCalculations();
}

void CAnalyzerDialog::OnBnClickedButtonSaveDataSet()
{
	dlgSetControlsAccessibility(FALSE);
	if (m_ParentThread)
	{
		m_ParentThread->PostThreadMessage(WM_AMT_SAVE_DATA_SET, 0, 0);
	}
}

void CAnalyzerDialog::OnBnClickedButtonSavePartitionMatrix()
{
	dlgSetControlsAccessibility(FALSE);
	if (m_ParentThread)
	{
		m_ParentThread->PostThreadMessage(WM_AMT_SAVE_PARTITION_MATRIX, 0, 0);
	}
}

void CAnalyzerDialog::OnBnClickedButtonSaveProjectionMatrix()
{
	dlgSetControlsAccessibility(FALSE);
	if (m_ParentThread)
	{
		m_ParentThread->PostThreadMessage(WM_AMT_SAVE_PROJECTION_MATRIX, 0, 0);
	}
}

void CAnalyzerDialog::OnBnClickedButtonStopCalculations()
{
	m_cbStopCalculations.EnableWindow(FALSE);
	m_ceStatus.SetWindowText(STATUS_STOPPING_CALCS);
	m_bIsCalculating = false;
	
	//onStopTimer();

	if (m_ParentThread)
	{
		m_ParentThread->PostThreadMessage(WM_AMT_STOP_CALCULATIONS, 0, 0);
	}
}

void CAnalyzerDialog::graphDrawGraph()
{
	UpdateData();

	// CPaintDC probably know how to work with Static control
	// while CClientDC cannot (try: CClientDC pdc(&m_PicCtrl); and you'll get an exception)
	// but it does know how to work with CDialog
	// interesting why....

	// Create a device context and connect it to a graphic object
	CPaintDC pdc(&m_PicCtrl);
	Graphics myGraphicsOrig(pdc.GetSafeHdc());

	// Get the borders of the picture control
	CRect ClientRec;
	m_PicCtrl.GetClientRect(&ClientRec);

	// Draw the image
	myGraphicsOrig.DrawImage(m_pBitmap, ClientRec.left,ClientRec.top,ClientRec.right,ClientRec.bottom);

#if 0 
	Bitmap bmp(ClientRec.right,ClientRec.bottom);
	// Create a Graphics object that is associated with the image.
	Graphics* graph = Graphics::FromImage(&bmp);

	///////
	//COLORREF  bb = pdc.SetBkColor(RGB(255, 200, 100));
	//pdc.TextOut(20,20,_T("HelloArea"));
	//pdc.SetBkColor(RGB(255, 200, 100));

	// Set the background
	graphSetBackground(*graph);//myGraphics);

	// Set transformation to reflect image across the X axis
	// (as the origin of axes is at the top left corner of the screen/control
	// and x axes goes right (positive) and y axes goes down (positive)
	graphSetTransformation(*graph);//myGraphics);

	// Draw axes
	graphDrawAxes(*graph);//myGraphics);

	// Draw grid
	graphDrawGrid(*graph);//myGraphics);

	// Draw data set
	graphDrawDataSet(*graph);//myGraphics);

	myGraphicsOrig.DrawImage(&bmp, ClientRec.left,ClientRec.top,ClientRec.right,ClientRec.bottom);
#endif
}

void CAnalyzerDialog::graphDrawOffScreenGraph()
{
	// Create a Graphics object that is associated with the off-screen image.
	Graphics* graph = Graphics::FromImage(m_pBitmap);

	// Set the background
	graphSetBackground(*graph);//myGraphics);

	// Set transformation to reflect image across the X axis
	// (as the origin of axes is at the top left corner of the screen/control
	// and x axes goes right (positive) and y axes goes down (positive)
	graphSetTransformation(*graph);//myGraphics);

	// Draw axes
	graphDrawAxes(*graph);//myGraphics);

	// Draw grid
	graphDrawGrid(*graph);//myGraphics);

	// Draw data set
	graphDrawDataSet(*graph);//myGraphics);

	// Redraw
	if ( m_bIsDialogCreated == true ) 
	{
		Invalidate();
	}
}

void CAnalyzerDialog::graphSetBackground(Graphics& io_refGraphics)
{
	io_refGraphics.Clear(_WHITE);

	// Margin for the drawing area 
	//CRect ClientRec;
	//m_PicCtrl.GetClientRect(&ClientRec);
	//SolidBrush SolidBrush2(_WHITE);
	
	//io_refGraphics.FillRectangle(&SolidBrush2, ClientRec.TopLeft().x, ClientRec.TopLeft().y, ClientRec.Width(), ClientRec.Height());
}

void CAnalyzerDialog::graphSetTransformation(Graphics& io_refGraphics, bool i_bTransformY /* = true*/ )
{
	// Transformation matrix for "normal" view of graph
	Gdiplus::Matrix mTransformation( 1.0f, 0.0f, 0.0f, (i_bTransformY == true) ? -1.0f : 1.0f , (REAL)m_dXSize, (REAL)m_dYSize);
	io_refGraphics.SetTransform(&mTransformation);
}

void CAnalyzerDialog::graphDrawAxes(Graphics& io_refGraphics)
{
	Pen pen( _BLACK, PEN_SIZE );
	Pen endPen( _BLACK, PEN_SIZE * (REAL)2.0 );
	endPen.SetEndCap(LineCapArrowAnchor);

	// Draw axes origin
	//SolidBrush mySolidBrush(_BLUE);
	//io_refGraphics.DrawRectangle(&pen, -5,-5, 10 , 10);
	//io_refGraphics.FillEllipse(&mySolidBrush, -50, -50, 100, 100);

	PointF pointStart, pointEnd;

	// Draw X
	pointStart.X	= (REAL) (GRAPH_MARGIN - m_dXSize );
	pointStart.Y	= (REAL) 0;
	pointEnd.X		= (REAL) (m_dXSize - GRAPH_MARGIN);
	pointEnd.Y		= (REAL) 0;
	io_refGraphics.DrawLine(&pen, pointStart, pointEnd);

	// Set arrow end to X axis
	pointStart.X = pointEnd.X - (REAL)0.2;
	pointStart.Y = pointEnd.Y;
	io_refGraphics.DrawLine(&endPen, pointStart, pointEnd);

	// Draw Y
	pointStart.X	= (REAL) 0;
	pointStart.Y	= (REAL) (GRAPH_MARGIN - m_dYSize);
	pointEnd.X		= (REAL) 0;
	pointEnd.Y		= (REAL) (m_dYSize - GRAPH_MARGIN);
	io_refGraphics.DrawLine(&pen, pointStart, pointEnd);

	// Set arrow end to Y axis
	pointStart.X = pointEnd.X;
	pointStart.Y = pointEnd.Y - (REAL)0.2;
	io_refGraphics.DrawLine(&endPen, pointStart, pointEnd);
}

void CAnalyzerDialog::graphDrawGrid(Graphics& io_refGraphics)
{
	// Transformation matrix for "normal" view of graph
	graphSetTransformation(io_refGraphics, false);

	// Change the color for the grid 
	Color myColor(_LIGTHGRAY);
	Pen pen( myColor, PEN_SIZE*0.1f );

	FontFamily		fontFamily(TIMES_NEW_ROMAN);
	Font			font(&fontFamily, GRAPH_FONT_SIZE, FontStyleRegular, UnitPixel);
	SolidBrush		solidBrush(myColor);
	StringFormat	stringFormat;
	stringFormat.SetAlignment(StringAlignmentCenter);

	double dCaptionX = 0.0;
	wchar_t	tickCaption[STRING_SIZE];
	RectF pointF;
	PointF PntStart, PntEnd;
	double dGridUnitValue, dGridUnitSizePixel;

	// Y Grid lines 
	// Draw the grid line and its value.
	// Note: grid lines are set on the X axis but are parallel to Y axis
	// Set the start and end points of grid line and draw the line

	graphCalculateGridUnitValue(m_structMinMax.m_dMaxX, dGridUnitValue);
	dGridUnitSizePixel = dGridUnitValue * m_dScaleX;

	// Draw first negative grid lines and values and then positive ones
	for ( int iSign = -1 ; iSign < 2 ; iSign += 2 )
	{
		for ( int i = 1 ; dGridUnitValue*(i-1) <= m_structMinMax.m_dMaxX ; ++i )
		{
			PntStart.X = PntEnd.X = (REAL)iSign * (REAL)(dGridUnitSizePixel) * (REAL)i;
			PntStart.Y = REAL(m_dYSize - GRAPH_MARGIN);
			PntEnd.Y = REAL(GRAPH_MARGIN - m_dYSize);
			io_refGraphics.DrawLine(&pen, PntStart.X, PntStart.Y, PntEnd.X, PntEnd.Y);

			// Draw the corresponding grid line value
			dCaptionX = iSign * dGridUnitValue * i;
			if(swprintf_s(tickCaption, STRING_SIZE, L"%.2g", dCaptionX))
			{
				pointF.X = (REAL)PntStart.X;
				pointF.Y = GRAPH_X_OFFSET ;

				io_refGraphics.DrawString(tickCaption, -1, &font, pointF, &stringFormat, &solidBrush);
			}
		}
	}

	// X Grid lines 
	// Draw the grid line and its value.
	// Note: grid lines are set on the Y axis but are parallel to X axis
	// Set the start and end points of grid line and draw the line
	graphCalculateGridUnitValue(m_structMinMax.m_dMaxY, dGridUnitValue);
	dGridUnitSizePixel = dGridUnitValue * m_dScaleY;

	// Draw first negative grid lines and values and then positive ones
	for ( int iSign = -1 ; iSign < 2 ; iSign += 2 )
	{
		for ( int i = 1 ; dGridUnitValue*(i-1) <= m_structMinMax.m_dMaxY ; ++i )
		{
			PntStart.X = REAL(m_dXSize - GRAPH_MARGIN);
			PntEnd.X = REAL(GRAPH_MARGIN - m_dXSize);
			PntStart.Y = PntEnd.Y = (REAL)iSign * (REAL)(dGridUnitSizePixel) * (REAL)i;// * (REAL)m_fScalePosY;
			io_refGraphics.DrawLine(&pen, PntStart.X, PntStart.Y, PntEnd.X, PntEnd.Y);

			// Draw the corresponding grid line value
			dCaptionX = -1 * iSign * dGridUnitValue * i;
			if(swprintf_s(tickCaption, STRING_SIZE, L"%.2g", dCaptionX))
			{
				pointF.X = -GRAPH_Y_OFFSET;
				pointF.Y = (REAL)PntStart.Y ;
				
				io_refGraphics.DrawString(tickCaption, -1, &font, pointF, &stringFormat, &solidBrush);
			}
		}
	}

	// Transformation matrix to reflect image across the X axis
	graphSetTransformation(io_refGraphics);
}

void CAnalyzerDialog::graphDrawDataSet(Graphics& io_refGraphics)
{
	// Draw data set
	multimap<int,PointF>::iterator mmIter, mmIterEnd;
	pair< multimap<int,PointF>::iterator, multimap<int,PointF>::iterator > ret;

	FontFamily		fontFamily(ARIAL);
	Font			font(&fontFamily, GRAPH_FONT_SIZE, FontStyleBold, UnitPixel);
	StringFormat	stringFormat;
	stringFormat.SetAlignment(StringAlignmentCenter);

	// This two values helps locating the characters in the right place 
	// Since each font has a line spacing which includes the font size
	// the character must be 'moved' along the Y axis
	REAL rFontHeight = font.GetHeight(&io_refGraphics);
	REAL rYTranslation = (rFontHeight - font.GetSize()/2.0f);
	PointF point;

	// TYPE1
	WCHAR wChar = 'O';
	SolidBrush mySolidBrush(_BLUE);
	ret = m_GraphicSets.equal_range(TYPE1);
	for(mmIter = ret.first ; mmIter != ret.second ; ++mmIter )
	{
		point = mmIter->second;
		point.Y -= rYTranslation;
		io_refGraphics.DrawString(&wChar, 1, &font, point, &stringFormat, &mySolidBrush);
	}

	// TYPE2
	// Draw centers only after data point so centers will be above data point in the graphic area
	wChar = 'X';
	mySolidBrush.SetColor(_RED);
	ret = m_GraphicSets.equal_range(TYPE2);
	for(mmIter = ret.first ; mmIter != ret.second ; ++mmIter )
	{
		point = mmIter->second;
		point.Y -= rYTranslation;
		io_refGraphics.DrawString(&wChar, 1, &font, point, &stringFormat, &mySolidBrush);
	}

	//PointF point(0.0f, - (rFontHeight-rVal/2.0f)+(REAL)GRAPH_MARGIN);//(lineSpacingPixel - (ascentPixel+descentPixel)/2.0f));//-(lineSpacingPixel + ((0*descentPixel-ascentPixel)/2.0f)));//, -(descentPixel+ascentPixel)/2.0f);
	//io_refGraphics.DrawString(&wChar, 1, &font, point, &stringFormat, &brush);
}

void CAnalyzerDialog::graphCalculateGridUnitValue(double i_dABSMaxVal, double &o_dUnit) const
{
	int iPower;

	if ( i_dABSMaxVal >= 1.0 )
	{
		iPower = (int)( abs( log10(i_dABSMaxVal) )); 
		o_dUnit =  (int)(i_dABSMaxVal / pow(10.0, (double)iPower ) + 0.5 ) * pow(10.0, (double)iPower - 1.0);
	}
	else if ( i_dABSMaxVal > 0.0 && i_dABSMaxVal < 1.0)
	{
		iPower = (int)( abs( log10(i_dABSMaxVal) ) + 1.0 ); 
		o_dUnit = 1.0 / (int) pow(10.0, (double)iPower);
	}
	else
	{
		//_ASSERT(false);
	}
}

Status CAnalyzerDialog::graphBuildGraphicDataSets()
{
	Status sRet = Ok;

	try
	{
		if( m_ProjCentersMatrix.size() <= 0 || m_ProjDataSetMatrix.size() <= 0 )
			return InvalidParameter;

		structMaxXY structMinMax;
		sRet = graphFindMaxVal(&m_ProjCentersMatrix, m_structMinMax);

		if ( sRet != Ok )
			throw 0;

		sRet = graphFindMaxVal(&m_ProjDataSetMatrix, structMinMax);

		m_structMinMax.m_dMaxX = max(m_structMinMax.m_dMaxX, structMinMax.m_dMaxX);
		m_structMinMax.m_dMaxY = max(m_structMinMax.m_dMaxY, structMinMax.m_dMaxY);

		if ( sRet != Ok )
			throw 0;

		m_GraphicSets.clear();

		// Process projected centers
		sRet = graphFillGraphicSet(&m_ProjDataSetMatrix, TYPE1);

		if ( sRet != Ok )
			throw 0;

		// Process projected data set
		sRet = graphFillGraphicSet(&m_ProjCentersMatrix, TYPE2);
	}
	catch (...)
	{
		AfxMessageBox(_T("Analyzer dialog - error while trying to draw results.\n\nAborting!"), MB_OK | MB_ICONEXCLAMATION);
		TRACE(_T("Exception caught in CAnalyzerDialog::graphBuildGraphicDataSets()"));
		_ASSERT(false);
		sRet = GenericError;
	}

	return sRet;
}

Status CAnalyzerDialog::graphFillGraphicSet(MatrixXd* i_pMatrix, int i_iType)
{
	Status sRet = Ok;

	if ( m_structMinMax.m_dMaxX != 0.0 && m_structMinMax.m_dMaxY != 0.0 && i_pMatrix != nullptr )
	{
		PointF point;
		double dValX, dValY;
		graphSetScale();

		int iSize = i_pMatrix->rows(); 

		for( int i = 0 ; i < iSize; ++i )
		{
			dValX = (*i_pMatrix)(i,0);
			dValY = (*i_pMatrix)(i,1);

			point.X = (REAL)(dValX * m_dScaleX);//+ (dValX < 0.0 ? GRAPH_MARGIN : -GRAPH_MARGIN ));
			point.Y = (REAL)(dValY * m_dScaleY);//+ (dValY < 0.0 ? GRAPH_MARGIN : -GRAPH_MARGIN ));

			m_GraphicSets.insert(pair<int, PointF>(i_iType, point));
		}
	}

	if ( sRet != Ok )
	{
		TRACE(_T("Exception caught in CAnalyzerDialog::graphFillGraphicSet()"));
		_ASSERT(false);
	}

	return sRet;
}

Status CAnalyzerDialog::graphFindMaxVal(MatrixXd* i_pMatrix, structMaxXY& o_structMinMax)
{
	if ( i_pMatrix == nullptr )
		return GenericError;

	o_structMinMax.m_dMaxX = fabs((*i_pMatrix)(0,0));
	o_structMinMax.m_dMaxY = fabs((*i_pMatrix)(0,1));
	int iSize = i_pMatrix->rows();

	for( int i = 0 ; i < iSize; ++i )
	{
		if(o_structMinMax.m_dMaxX < fabs((*i_pMatrix)(i,0)) )
			o_structMinMax.m_dMaxX = fabs((*i_pMatrix)(i,0));

		if(o_structMinMax.m_dMaxY < fabs((*i_pMatrix)(i,1)) )
			o_structMinMax.m_dMaxY = fabs((*i_pMatrix)(i,1));
	}

	return Ok;
}

void CAnalyzerDialog::graphSetScale()
{
	m_dScaleX = (m_dXSize - GRAPH_MARGIN)/ m_structMinMax.m_dMaxX;
	m_dScaleY = (m_dYSize - GRAPH_MARGIN)/ m_structMinMax.m_dMaxY;
}

void CAnalyzerDialog::dlgSetControlsAccessibility(BOOL i_bIsEnabled /*= TRUE*/)
{
	if ( m_bIsDialogCreated == true )
	{
		m_cbOK.EnableWindow(i_bIsEnabled);
		m_cbCancel.EnableWindow(i_bIsEnabled);
		m_cbExecuteClustering.EnableWindow(i_bIsEnabled);
		m_cbStopCalculations.EnableWindow(!i_bIsEnabled);
		m_cbSaveDataSet.EnableWindow(i_bIsEnabled);
		m_cbSavePartitionMatrix.EnableWindow(i_bIsEnabled);
		m_cbSaveProjectionMatrix.EnableWindow(i_bIsEnabled);

		UpdateData(FALSE);
	}
}

inline void CAnalyzerDialog::dlgSetComponents()
{
	if ( m_bIsDialogCreated == true )
	{
		// Set dialog window title
		dlgSetTitle();

		// set controls with data
		m_ceProcessType.SetWindowText(m_sProcessName);
		m_ceStatus.SetWindowText(STATUS_READY);
		UpdateProgress(enProgressInitialize);
		dlgSetClusteredSetSize();
		dlgSetTimes(enTimeInitialize);
		
		// Update GUI
		UpdateData(FALSE);
	}
}

inline void CAnalyzerDialog::dlgSetTitle()
{
	if ( m_bIsDialogCreated != true )
		return;
	
	m_sDialogTitle = ANALYZER_DIALOG_TITLE;
	
	if ( m_bIsTestMode == true )
	{
		m_sProcessName.AppendFormat(_T(" (Test mode)"));
		m_sDialogTitle = _T("!!Test Mode!!Test Mode!! - ") + m_sDialogTitle;
	}

	if ( m_iProcessID > 0 )
	{
		CString Info;
		Info.Format(_T(" (PID %d)"), m_iProcessID);

		m_sDialogTitle = m_sDialogTitle + Info ;
	}

	// Title
#if _DEBUG
	m_sDialogTitle = _T("DEBUG - ") + m_sDialogTitle + _T(" - DEBUG");
#endif

	SetWindowText(m_sDialogTitle);
}

void CAnalyzerDialog::dlgSetClusteredSetSize()
{
	if ( m_bIsDialogCreated != true )
		return;

	CString cstr;
	cstr.Format(_T("%d"), m_ProjDataSetMatrix.rows() );
	m_ceSizeOfClusteredSet.SetWindowText(cstr);
}

LONGLONG CAnalyzerDialog::dlgSetTimes(enTimeStatus i_enStatus)
{
	CString sTextTotalTime(HH_MM_SS), sTextEstimatedTime(HH_MM_SS);

	if ( i_enStatus == enTimeInitialize )
	{
		m_timeDuration = m_timeFinish - m_timeStartGeneral;
		sTextTotalTime.SetString(m_timeDuration.Format(_T("%H:%M:%S")));		
		//sTextEstimatedTime
	}
	else if ( i_enStatus == enTimeFinish )
	{
		m_timeStartProjection = m_timeFinish = CTime::GetCurrentTime();
		m_timeDuration = m_timeFinish - m_timeStartGeneral;
		sTextTotalTime.SetString(m_timeDuration.Format(_T("%H:%M:%S")));		
	}
	else if ( i_enStatus == enTimeProgress )
	{
		m_timeFinish = CTime::GetCurrentTime();
		m_timeDuration = m_timeFinish - m_timeStartProjection;
		
		float coeff = (float)(m_uNumOfProjSteps) / (float)(m_uNumItersSoFar);
		float secondsSoFar = (float)m_timeDuration.GetTotalSeconds();
		float currentEstimatedTime = secondsSoFar * coeff - secondsSoFar + 1.0f;

		// exponential smoothing
		static float alpha = 0.125; //  0 <= alpha >= 1
		currentEstimatedTime = alpha*currentEstimatedTime + (1-alpha)*(float)m_uLastEstimatedTime;

		UINT seconds = (UINT)currentEstimatedTime%60;
		UINT total_minutes = (UINT)currentEstimatedTime/60;
		UINT minutes = (UINT)total_minutes%60;
		UINT hours = (UINT)total_minutes/60;

		m_uLastEstimatedTime = (UINT)currentEstimatedTime;

		sTextEstimatedTime.Format( _T("%02d:%02d:%02d"), hours, minutes, seconds);
	}
	// else (i_enStatus == enTimeInitialize) - do nothing

	if ( m_bIsDialogCreated == true)
	{
		m_csTotalTime = sTextTotalTime;
		m_csEstimatedTime = sTextEstimatedTime;
		UpdateData( FALSE );
	}

	return m_timeDuration.GetTotalSeconds();
}

void CAnalyzerDialog::dlgUpdateDialogGUI()
{
	if ( m_bIsDialogCreated != true)
		return;

	if ( m_bIsCalculating == true)
	{
		m_ceSizeOfClusteredSet.SetWindowText(_T("--"));
		setClusteringAttributes();

		//m_uTimer = SetTimer(ANALYZER_DLG_TIMER_EVENT_ID, ANALYZER_TIMER_MILLISEC, 0);
		m_pcProgressCalcs.SetRange(PROGRESS_BAR_MIN_POS, m_uNumOfProjSteps);
		m_pcProgressCalcs.SetPos(PROGRESS_BAR_MIN_POS);
		m_csTotalTime.SetString(HH_MM_SS);
		m_csEstimatedTime.SetString(HH_MM_SS);

		dlgSetControlsAccessibility(FALSE);
	}
	else
	{
		dlgSetControlsAccessibility();
	}
}

void CAnalyzerDialog::setClusteringAttributes()
{
	m_ClusteringAtt.EnterCriticalSection();
	m_ClusteringAtt.m_Variable.m_uNumOfClusters			= m_uNumOfClusters;
	m_ClusteringAtt.m_Variable.m_dWeightingExponenet	= m_dWeightingExponenet;
	m_ClusteringAtt.m_Variable.m_dWeightingParameter	= m_dWeightingParameter;
	m_ClusteringAtt.m_Variable.m_dTerminationTolerance	= m_dTerminationTolerance;
	m_ClusteringAtt.m_Variable.m_dMaxRatio				= m_dMaxRatio;
	m_ClusteringAtt.m_Variable.m_uDimension				= m_uDimension;
	m_ClusteringAtt.m_Variable.m_dGRadientSize			= m_dGRadientSize;
	m_ClusteringAtt.m_Variable.m_uNumOfProjSteps		= m_uNumOfProjSteps;
	m_ClusteringAtt.LeaveCriticalSection();
}

void CAnalyzerDialog::executeCalculations()
{
	// This method can be executed even if the dialog is still not created!!!
	// It is done via the manager in order to get the clustering attributed (input parameters)
	// from one source - the dialog!
	m_bIsCalculating = true;
	m_uLastEstimatedTime = m_uNumItersSoFar = 0;
	m_timeStartGeneral = CTime::GetCurrentTime();

	dlgUpdateDialogGUI();

	//m_cbStopCalculations.EnableWindow(TRUE);

	if ( m_ParentThread )
		m_ParentThread->PostThreadMessage(WM_ACT_EXEC_CALCULATIONS, (WPARAM) &m_ClusteringAtt, 0 );
}

/*
void CAnalyzerDialog::advanceProgressBar()
{
	int iPos = m_pcProgressCalcs.GetPos();
	if ( m_pcProgressCalcs.GetPos() == PROGRESS_BAR_MAX_POS ) 
	{
		m_pcProgressCalcs.SetStep(-1 * PROGRESS_BAR_STEP_SIZE );
	}
	else if ( m_pcProgressCalcs.GetPos() == PROGRESS_BAR_MIN_POS )
	{
		m_pcProgressCalcs.SetStep(PROGRESS_BAR_STEP_SIZE);
	}
	m_pcProgressCalcs.StepIt();
}

void CAnalyzerDialog::onStopTimer() 
{
	KillTimer(m_uTimer);   
}

void CAnalyzerDialog::OnTimer(UINT nIDEvent) 
{
	//MessageBeep(0xFFFFFFFF);   // Beep

	if ( m_bIsDialogCreated == true ) 
	{
		double duration_t;
		m_timeFinish = clock();
		duration_t = (double)(m_timeFinish - m_timeStartGeneral) / CLOCKS_PER_SEC ;// seconds // * 1000; // milliseconds

		if ( duration_t > 5.0 && duration_t <= 12.0 )
		{
			m_ceStatus.SetWindowText(STATUS_PROCESSING_MESSAGE1);
		}

		else if ( duration_t > 12.0 && duration_t <= 20.0 )
		{
			m_ceStatus.SetWindowText(STATUS_PROCESSING_MESSAGE2);
		}
		else if ( duration_t > 20.0)
		{
			m_ceStatus.SetWindowText(STATUS_PROCESSING_MESSAGE3);
		}

		advanceProgressBar();
	}

	// Call base class handler.
	CDialog::OnTimer(nIDEvent);
}
*/


