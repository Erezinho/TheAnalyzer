/*****************************************************************************
*
*	Project: Analyzer.dll
*
*	Description: Header file for Clustering and Mapping algorithms.
*
*	Date     |  Description
*	Jan 2011	Created
*	May 2011	Added definitions and some documentation.
*				Changed default values for methodsbugs in Fuzzy Sammon
*				Mapping algorithm (CFuzSam)
*
*****************************************************************************/
#pragma once
#include <fstream>

// In order to avoid collisions with windows.h definitions
// of min/max it is a must to undefine those definitions
// before including Eigen directories
#ifdef min 
#undef min
#endif
#ifdef max 
#undef max
#endif

#define SIMPLE_SET				0
#define OPEN_CONSOLE			0

#define WRITE_TO_FILE 			0
#define FILE_PATH				_T("Output\\ClusteringDumpAnalyzer .txt")

#define DATA_SET_PATH			_T("Output\\zDataSet.txt")

#define WRITE_PARTITION_MATRIX	0
#define PARTITION_MATRIX_PATH	_T("Output\\zPartitionMatrixAnalyzer.txt")

#define WRITE_PROJECTION_MATRIX	0
#define PROJECTION_MATRIX_PATH	_T("Output\\zProjectionMatrixAnalyzer.txt")

#include "../Eigen/Dense"
#include "../Eigen/Eigenvalues"

using namespace std;
using namespace Eigen;

/////////////////////////////////////////////////////////////////////////////////////////
// The following class implements the Gustafson-Kessel clustering algorithm
// Implementation is based on the article and thier implementation in Matlab
/////////////////////////////////////////////////////////////////////////////////////////
class AFX_CLASS_EXPORT CGKCluster
{
public:
	CGKCluster();
	virtual ~CGKCluster();

	void SetTestMode(bool i_bIsTestMode);

	bool Initialize(MatrixXd& i_DataSet, UINT i_uNumOfClusters = 2, double i_dWE = 2,
					double i_dEpsilon = 0.001, double i_dGamma = 0.2, double i_dBeta = 1e15);
	bool FindClusters();

	const MatrixXd& GetPartitionMatrix();
	const MatrixXd& GetDistanceMatrix();
	const MatrixXd& GetCentersMatrix();

	bool SaveDataSet();
	bool SavePartitionMatrix();

	void StopCalcs(bool i_bStop);
	
protected:
	// N - number of iterations
	// n - dimension (number of parameters)
	UINT	m_uNumOfClusters;			// c		; 1 < c < N
	double	m_dWeightingExponent;		// m		; m > 1
	double	m_dTerminationTolerance;	// epsilon	; epsilon > 0
	double	m_dGamma;					// Gamma
	double	m_dBeta;					// Beta

	MatrixXd	m_DataSetMatrix;			// Nxn - Input matrix (X)
	MatrixXd	m_ScaledIdentityMatrix;		// nxn - Scaled identity matrix
	MatrixXd	*m_pPartitionMatrixPrev;	// Nxc - "Belonging" matrix (U) of previous loop

	MatrixXd	*m_pPartitionMatrix;		// Nxc - "Belonging" matrix (U)
	MatrixXd	m_DistancesMatrices;		// Nxc - distance of each point from each cluster center
	MatrixXd	m_CentersMatrix;			// cxn - For each cluster, defines the centers  (V)

	// Test mode
	bool m_bTestMode;
	bool m_bStop;

private:
	// Removes atrophied dimensions (sd == 0) and normalizes each remaining dimension
	void preprocessData();

	// Generates a random partition matrix
	void generateRandomPartitionMatrix();

	// Calculates the covariance matrix of a given matrix
	void covarianceMatrix(MatrixXd& i_Matrix, MatrixXd& o_CovMatrix);

	// Calculates the ratio between the largest singular value of the matrix and the smallest
	double calculateSingularRatio(MatrixXd& i_refMatrix);

	void openConsole();
	void closeConsole();

	void applyTestMode();

private:
	UINT m_uDimension;
	UINT m_uSizeOfDataSet;
};

/////////////////////////////////////////////////////////////////////////////////////////
// The following class implements the Fuzzy Sammon projection algorithm for finding
// N points in a q-dimension data space where the original data are from a
// higher n-dimensional space.
// Implementation is based on the article and thier implementation in Matlab
// The algorithm is based on the Steepest Descent algorithm 
/////////////////////////////////////////////////////////////////////////////////////////
class AFX_CLASS_EXPORT CFuzSam
{
public:
	CFuzSam();
	virtual ~CFuzSam();

	void SetTestMode(bool i_bIsTestMode);

	// Initialization method. Note that weighing exponent (i_dWE) must be identical
	// to the one that was used by the clustering algorithm
	bool Initialize(const MatrixXd& i_refDistancesMatrices, const MatrixXd& i_refPartitionMatrix,
					double i_dWE = 2, UINT i_uDimension = 2, double i_dGradientStepSize = 0.4, UINT i_uNumOfSteps = 2000);

	// The class allows the user to find projection in two ways:
	// 1) StepProjection() - a step-by-step execution approach, the user is responsible to loop as much iteration
	// as he/she wants while the CFuzSam executes only one step of the algorithm at a time.
	// This way the user can display mid-results or/and have some knowledge about the algorithm progress
	// The i_uIndex argument is just for the sack of debugging/displaying information to GUI/console
	// 2) CompleteProjection() - a full execution approach, the CFuzSam runs the algorithm as many steps as
	// required/specified. The user can get the results only when the algorithm completes
	bool StepProjection(UINT i_uIndex = UINT_MAX);
	bool CompleteProjection();

	// Reduces memory usage of the class by resizing internal matrices and vectors
	void ReduceMemUsage();
	
	const MatrixXd& GetProjectedCenters();
	const MatrixXd& GetProjectedDataSet();

	bool SaveProjectionMatrices();

	void StopCalcs(bool i_bStop);

protected:
	UINT m_uNumOfPoints;
	UINT m_uNumOfClusters;
	UINT m_uDimension;
	UINT m_uNumOfSteps;

	// Step size for the steepest descant algorithm
	double m_dGradientStepSize;

	// The partition matrix is actually the powered-by-m form as this is the only form that
	// it is used right from the beginning
	MatrixXd m_PartitionMatrix;
	MatrixXd m_DistancesMatrix;	
	
	MatrixXd m_ProjectedCenters;
	MatrixXd m_ProjectedDataSet;

	// Test mode
	bool m_bTestMode;

	bool m_bStop;

private:

	void applyTestMode();

private:
	// The following data members are used for the step projection calculation.
	// It is a waist of time to instantiate them every time the method is called (StepProjection)
	// Instead they are part of the class.
	// In order to save space, once calculations are completed, there is the a method named
	// ReduceMemUsage() which squeezes matrices and vectors sizes. 
	// To the right of some members there is the equivalent symbol in the Matlab code

	// The sum of each column of the powered partition matrix
	VectorXd m_SumPoweredPartitions;

	// Vector of euclidean distances between the j point and each center in q-dimension
	VectorXd m_EuclideanDisVec; // == dpj , dr

	// Vector of differences between the distances of the j point to each center
	// in n-dimension ( from input ) and in q-dimension
	VectorXd m_DiffDistancesVec; // == dq in Matlab code
	
	// Hold first and second derivatives
	VectorXd m_FirstDerivative, m_SecondDerivative; // == e1, e2 in Matlab code

	// Transposed powered partitions of a specific point in n-dimension
	VectorXd m_TransposedPoweredPartVec;
	
	// Transposed powered distance of a specific point in n-dimension
	VectorXd  m_TransposedPoweredDistVec;
	
	// Matrix of differences between the j point and each center in q-dimension
	MatrixXd m_DiffPointToCentersMatrix; // == xd in Matlab code

	// Matrix of differences between the j point and each center powered by 2 in q-dimension
	MatrixXd m_PoweredDiffPointToCentersMatrix; // == xd2 in Matlab code

	// Temporary row vector
	RowVectorXd m_TempRowVec;
};

#if WRITE_TO_FILE

class CFileOutput
{
public:
	CFileOutput();
	~CFileOutput();

	static ofstream m_foutput;
};
#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class AFX_CLASS_EXPORT CClusterTest
{
public:
	CClusterTest();
	virtual ~CClusterTest();

	void Build2X2SymMatrix();
	void BuildXMatrix(int i_rows, int i_cols);
	void BuildOnesMatrix(int i_rows, int i_cols);
	void BuildIdentityMatrix(int i_rows, int i_cols);

	void EigenValues();
	void EigenVectorsAndPrint();

	void PowerMatrixAndPrint(double i_dPower);

	template< class T >
	void Print(T& m);

	void PrintMatrix();
	void PrintPowerMatrix(MatrixXd& i_Mat, double i_dPower);
	void PrintEigenvalues();

	MatrixXd m_Matrix;
	VectorXcd m_Vector;
};
