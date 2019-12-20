#pragma once

//Analyzer tool/dialog
#define ANALYZER_OPEN_DIALOG			_T("The analyzer dialog can not be opened as the tool was never activated!\n\nThe tool can be activated only before (but never during) parameter estimation / population treatment optimization\nprocess begins and within its dialog using corresponding check box!")
#define ANALYZER_ERROR_EXEC_CLUSTERING	_T("The analyzer module can not execute the clustering algorithm!!\n\nPlease advise software administrator!")
#define ANALYZER_CLUSTERING_SMALL_SET	_T("Set too small - The analyzer module can not execute the clustering algorithm on data set smaller than 10 points!!\n\nAborting clustering.\n")
#define ANALYZER_CLUSTERING_ERROR_INIT	_T("An Error occured during clustering process!\nAborting clustering\n(Initialization stage)\n")
#define ANALYZER_CLUSTERING_ERROR_CALC	_T("An Error occured during clustering process!\nAborting clustering\n(Calculation stage)\n")
#define ANALYZER_PROJECTION_ERROR_INIT	_T("An Error occured during projection process!\nAborting projection\n(Initialization stage)\n")
#define ANALYZER_PROJECTION_ERROR_CALC  _T("An Error occured during projection process!\nAborting projection\n(Calculation stage)\n")
#define ANALYZER_SAVE_SUCCESSFULY		_T("Analyzer - Data have been saved successfully!")
#define ANALYZER_SAVE_FAILED			_T("Analyzer - Failed to save data!\nAborting operation!")
