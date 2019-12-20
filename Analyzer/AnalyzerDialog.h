/**************************************************************************************
*
*	Description: Simple dialog under the module control
*	
*
*	Date     |  Description
*	Nov 2010	Created
*	May 2011	Renamed OnBnClickedButtonClean to OnBnClickedButtonStep
*	Jul 2011	Added enTimeStatus
*				Added graphDrawOffScreenGraph()
*				Changed dlgSetTimes argument type
*				Added m_pBitmap for double buffering
*										
***************************************************************************************/
#pragma once
#include "afxcmn.h"
#include "resource.h"
#include "afxwin.h"
#include "Defines.h"

#include <vector>
#include <map>
#include <gdiplus.h>

using namespace Gdiplus;
using namespace std;

enum enTimeStatus
{
	enTimeInitialize = 0,
	enTimeProgress,
	enTimeFinish
};

struct structMaxXY
{
	structMaxXY() : m_dMaxX(0.0) , m_dMaxY(0.0) {}
	double m_dMaxX;
	double m_dMaxY;
};

// CAnalyzerDialog dialog
class CAnalyzerDialog : public CDialog
{
	DECLARE_DYNAMIC(CAnalyzerDialog)

public:
	CAnalyzerDialog(CWnd* pParent = nullptr); 
	virtual ~CAnalyzerDialog();

	// Dialog Data
	enum { IDD = IDD_ANALYZER_DIALOG };

	// Modeless functionality methods
	// i_pMainWnd - pointer to main window. It is used to locate 
	// the dialog next to main window instead of on top of it
	// i_iProcessID - the process ID to which the analyzer belongs
	BOOL OpenDialog(CWnd* i_pMainWnd, int i_iProcessID);
	void CloseDialog();

	void Initialize(const CString& i_refProcessTitle, bool i_bTestMode);//TCHAR* i_pStr);
	void SetParentThread(CWinThread* i_pParanetThread);

	// The method is used to update the dialog about the saving operation
	// (completion of operation) 
	void SavingCompletionReport(bool i_bIsOK);

	// The method is called when calculations (clustering+projection) are ended
	Status FinishCalculations(WPARAM wParam, LPARAM lParam); 

	// The method is used to update the dialog about current stage of calculations
	// i.e. where in the calculation the calculating unit is at a given time point
	void UpdateProgress(enProgressStatus i_enStatus);

	void Finalize();
	//afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnPaint();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnEnKillfocusEditNumberOfClusters();
	afx_msg void OnBnClickedButtonExecuteClustering();
	afx_msg void OnEnKillfocusEditWeightingExponent();
	afx_msg void OnEnKillfocusEditMaxRatio();
	afx_msg void OnEnKillfocusEditWeightingParameter();
	afx_msg void OnEnKillfocusEditTerminationTolerance();
	afx_msg void OnEnKillfocusEditDimension();
	afx_msg void OnEnKillfocusEditGradientSize();
	afx_msg void OnEnKillfocusEditNumOfProjSteps();
	afx_msg void OnBnClickedButtonSaveDataSet();
	afx_msg void OnBnClickedButtonSavePartitionMatrix();
	afx_msg void OnBnClickedButtonSaveProjectionMatrix();
	afx_msg void OnBnClickedButtonStopCalculations();

public:
	// Controls related data members 
	CEdit			m_ceProcessType;
	CEdit			m_ceStatus;
	CEdit			m_ceSizeOfClusteredSet;
	CStatic			m_PicCtrl;
	CButton			m_cbOK;
	CButton			m_cbCancel;
	CButton			m_cbExecuteClustering;
	CButton			m_cbStopCalculations;
	CButton			m_cbSaveDataSet;
	CButton			m_cbSavePartitionMatrix;
	CButton			m_cbSaveProjectionMatrix;
	CButton			m_cbAnalyzeAutomatically;
	CComboBox		m_cbClusteringAlg;
	CComboBox		m_cbProjectionAlg;
	CProgressCtrl	m_pcProgressCalcs;

	SecureVariable<SClustringAttributes> m_ClusteringAtt;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	
	void graphDrawGraph();
	void graphDrawOffScreenGraph();
	void graphSetBackground(Graphics& io_refGraphics);
	void graphSetTransformation(Graphics& io_refGraphics, bool i_bTransformY = true);
	void graphDrawAxes(Graphics& io_refGraphics);
	void graphDrawGrid(Graphics& io_refGraphics);
	void graphDrawDataSet(Graphics& io_refGraphics);
	void graphCalculateGridUnitValue(double i_dABSMaxVal, double &o_dUnit) const;

	// Set graphic area scales
	void graphSetScale();

	Status graphBuildGraphicDataSets();
	Status graphFindMaxVal(MatrixXd* i_pMatrix, structMaxXY& o_structMinMax);
	Status graphFillGraphicSet(MatrixXd* i_pMatrix, int i_iType);//GraphicItemStruct& i_refAtt);

	DECLARE_MESSAGE_MAP()

	// Initialize the dialog after creation and when all data exist
	void dlgSetComponents();

	// Set the dialog title
	void dlgSetTitle();
	LONGLONG dlgSetTimes(enTimeStatus i_enStatus);
	void dlgSetClusteredSetSize();
	void dlgSetControlsAccessibility(BOOL i_bIsEnabled = TRUE);
	void dlgUpdateDialogGUI();

	void setClusteringAttributes();
	void executeCalculations();

	//void onStopTimer();
	//void advanceProgressBar();

protected:
	// Pointer main thread
	// Used to communicate (post messages) with creator
	CWinThread* m_ParentThread;

	int			m_iProcessID;
	bool		m_bIsDialogCreated;
	bool		m_bIsTestMode;
	bool		m_bIsCalculating;
	CString		m_sDialogTitle;
	CString		m_sProcessName;
	CString		m_csTotalTime;				// Total time
	CString		m_csEstimatedTime;			// Estimated time to finish calculations
	UINT_PTR	m_uTimer;
	CTime		m_timeStartGeneral;
	CTime		m_timeStartProjection;		// Projection is the heaviest step which requires estimation
	CTime		m_timeFinish;
	CTimeSpan	m_timeDuration;
	UINT		m_uLastEstimatedTime;
	UINT		m_uNumItersSoFar;

	multimap<int, PointF> m_GraphicSets;

	// The final matrices to be drawn in the graphic area
	MatrixXd m_ProjCentersMatrix;
	MatrixXd m_ProjDataSetMatrix;

	// Graph related data members
	double m_dXSize;
	double m_dYSize;
	double m_dScaleX;
	double m_dScaleY;

	ULONG_PTR		m_gdiplusToken;
	structMaxXY		m_structMinMax;
	GraphicsPath*	m_GraphicsPath;

	// Input parameters
	UINT	m_uNumOfClusters;			// must be integer > 1 
	double	m_dWeightingExponenet;		// must be > 1
	double	m_dWeightingParameter;		// gamma, must be 0 <= >= 1
	double	m_dTerminationTolerance;	// epsilon, must be > 0
	double	m_dMaxRatio;				// Beta, must be > 0
	UINT	m_uDimension;				// must be integer >= 1 
	double	m_dGRadientSize;			// must be > 0
	UINT	m_uNumOfProjSteps;			// must be integer > 0

	// The bitmap class is used for double buffering of the drawn data
	// to avoid flickering of the image (drawn results)
	// In case of large scale data, flickering of the image (drawn results) / window / dialog 
	// is occurring because of the time required to draw the data while the window is active
	// By using double buffering, upon "arrival" of new/updated data to the dialog, the data is drawn  
	// into an off-screen Bitmap instance (image) and then the image is drawn into the screen/window/dialog 
	Bitmap* m_pBitmap;
};
