/*****************************************************************************                                                
*
*	Project: Analyzer.dll
*	
*	Description: A Singleton, interface class.
*	It separates between the interface and the actual Analyzer which is non-static,
*	private data member.
*
*	Date	 |  Description
*	Nov 2010	Created
*	May 2011	Added data member - static CCriticalSection m_staticCriticalSection
*
******************************************************************************/

#pragma once
#include "afxmt.h"

class CAnalyzerManagerThread;

class AFX_EXT_CLASS CAnalyzerIF 
{
public:
	static CAnalyzerIF* CreateAnalyzerIF();
	static void DeleteAnalyzerIF();

	// Initializes the module
	// i_refProcessTitle - A reference to a CString describing the process being analyzed
	void Initialize(const CString& i_refProcessTitle, bool i_bTestMode = false);

	// Set Data as a vector of doubles
	void SetData(const vector<double>& i_refVecVals);

	// Finalize executes the relevant calculations on the whole data automatically
	void Finalize();

	// i_pMainWnd - pointer to main window. It is used to locate 
	// the dialog next to main window instead of on top of it
	// i_iProcessID - the process ID to which the analyzer belongs
	void OpenDialog(CWnd* i_pMainWnd, int i_iProcessID = 0);
	void CloseDialog();

protected:
	CAnalyzerIF() ;
	~CAnalyzerIF() ;
	CAnalyzerIF(const CAnalyzerIF& i_CAnalyzerIF) noexcept {}
	CAnalyzerIF& operator=(const CAnalyzerIF& i_CAnalyzerIF);

protected:
	// The interface class instance
	static CAnalyzerIF* m_pInstance;

	// The Manager object
	CAnalyzerManagerThread* m_pAnalyzerManagerThread{};

private:
	CRITICAL_SECTION m_CriticalSection{};
	static ::CCriticalSection m_staticCriticalSection;
};
