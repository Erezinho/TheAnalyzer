/*****************************************************************************                                                
*
*	Project: OptimizationDemoDlg.exe
*	
*	Description: An optimization & Parameter estimation simulator dialog
*	There are two working threads which generate random numbers at each iteration
*	as a set of parameters which is then transferred to the Analyzer module
*
*	Date	 |  Description
*	Nov 2010	Created
*	Jul 2011	Finalized
*
******************************************************************************/

#pragma once
#include "afxwin.h"

#include "../Include/OutputDataTypes.h" 
#include "afxcmn.h"

class CAnalyzerIF;
class CParameterSet;
class COptimizationExecUnit;

// COptimizationDemoDlg dialog
class COptimizationDemoDlg : public CDialog
{
// Construction
public:
	COptimizationDemoDlg(CWnd* pParent = NULL);	// standard constructor
	virtual ~COptimizationDemoDlg();

	// Dialog Data
	enum { IDD = IDD_OPTIMIZATIONDEMO_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	// Handlers
	afx_msg void OnBnClickedButtonStartOptDemo();
	afx_msg void OnBnClickedButtonStopOptDemo();
	afx_msg void OnAnalyzerOpenDialog();
	afx_msg void OnAnalyzerCloseDialog();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnEnKillfocusEditIdcParEstimNumParam();

public:
	CEdit m_ceNumParams{};
	CButton m_cbStart{};
	CButton m_cbStop{};
	CButton m_cbTestMode{};
	CButton m_cbRestrictedRanges{};
	CButton m_cbFourClusters{};
	CComboBox m_cbProcessType{};

	UINT m_uNumOfIterations{};
	UINT m_uTotalIters{};
	short m_shortNumParams{};

	CListCtrl m_lcGeneratedValues{};
	CAnalyzerIF* m_pAnalyzer{};

protected:
	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

protected:
	HICON m_hIcon{};
	int m_iCurrSel{};
	UINT_PTR m_nTimer{};

	COptimizationExecUnit* m_pOptUnit1{};
	COptimizationExecUnit* m_pOptUnit2{};

	CWinThread* m_pThread1{};
	CWinThread* m_pThread2{};
public:
	afx_msg void OnBnClickedCheckFourClustersPe();
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//							"Execution" class							//
// Executed by working threads
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class COptimizationExecUnit
{
public:
	COptimizationExecUnit();
	~COptimizationExecUnit();
	static UINT OptimizationDemoThread(LPVOID pParam);

	void GetGeneratedProtocol(CCompositeProtocol& o_refCOmpProtocol);
	void GetEstimatedParameters(vector< _ParameterStruct >& o_refVecEstimParams);

public:
	int m_iMinRandNumber{};
	int m_iMaxRandNumber{};
	int m_iCurrSel{};
	bool m_bTestMode{};
	bool m_bRun{};
	bool m_bFourClusters{};
	UINT m_uTotalIters{};
	UINT m_uCounter{};
	short m_shortNumParams{};
	CAnalyzerIF* m_pAnalyzer{};

	byte m_byteFourClustersFlip{};

protected:
	void initializeDataStructures();

	// Thread related methods	
	void startOptimizationDemo();
	void resetDataStructures();
	void generatePTOParams();
	void generateEstimParams();
	float getRandomNumber();

	void extractData(const CParameterSet& i_pParamSet, vector<double>& o_vecDataSet);
	void extractDataSet(const std::vector<_ParameterStruct>& i_vParameterStruct, vector<double>& o_vecDataSet);
	void extractDataSet(const CCompositeProtocol& i_comProtocol, vector<double>& o_vecDataSet);

protected:
	CParameterSet* m_pParameterSet{};

	// map of protocol parameters - used when generating values
	map< int, FloatVector > m_mapProtParams{};

	// These two data structures are used by dialog to show data
	// The vector is used also when generating values
	CCompositeProtocol m_CompProtocol{};
	vector< _ParameterStruct > m_vecEstimParams{};

	// Critical section to lock data structures.
	// Required when client requires data 
	CRITICAL_SECTION m_CriticalSection{};
};
