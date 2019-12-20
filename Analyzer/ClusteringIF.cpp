/*****************************************************************************
*
*	Project: Analyzer.dll
*
*	Description: Implementation of Clustering and Mapping algorithms.
*	The file includes the Gustafsson and Kessel clustering algorithm and
*	the Fuzzy Sammon mapping (projection) algorithm.
*
*	Date     |  Description
*	Jan 2011	Created
*	May 2011	Fixed bugs in Fuzzy Sammon Mapping algorithm (CFuzSam)
*
*****************************************************************************/

#include "stdafx.h"
#include "ClusteringIF.h"
#include <conio.h>

#include <io.h>
#include <Fcntl.h>
#include <iostream>


#define ZERO_EPSILON		0.0000001
#define DIVISION_EPSILON	1e-10

#if WRITE_TO_FILE

ofstream CFileOutput::m_foutput;

// Create an instance
CFileOutput Out;

CFileOutput::CFileOutput()
{
	// Every time the class is instantiated, the output file is truncated
	m_foutput.open(FILE_PATH, ios::out | ios::trunc);
	m_foutput.close();
}

CFileOutput::~CFileOutput()
{
	// Close the file, just in case!
	if ( m_foutput.is_open() )
		m_foutput.close();
}
#endif

CGKCluster::CGKCluster() :
m_bTestMode(false),
m_bStop(false)
{
	try
	{
		applyTestMode();

		m_pPartitionMatrix = new MatrixXd();
		m_pPartitionMatrixPrev = new MatrixXd();

#if OPEN_CONSOLE
		openConsole();
#endif

	}
	catch (...)
	{
		delete m_pPartitionMatrix;
		delete m_pPartitionMatrixPrev;
		m_pPartitionMatrix = m_pPartitionMatrixPrev = nullptr;

		TRACE(_T("Exception caught in CGKCluster::CGKCluster()"));
		_ASSERT(nullptr);
	}
}

CGKCluster::~CGKCluster()
{
	delete m_pPartitionMatrix;
	delete m_pPartitionMatrixPrev;
	m_pPartitionMatrix = m_pPartitionMatrixPrev = nullptr;

#if OPEN_CONSOLE
	closeConsole();
#endif
}

void CGKCluster::SetTestMode(bool i_bIsTestMode)
{
	m_bTestMode = i_bIsTestMode;
}

void CGKCluster::StopCalcs(bool i_bStop)
{
	m_bStop = i_bStop;
}

bool CGKCluster::Initialize(MatrixXd& i_DataSet, UINT i_uNumOfClusters /*=2*/, double i_dWE /*=2*/,
							double i_dEpsilon/*=0.001*/, double i_dGamma/*=0*/, double i_dBeta/*=1e15*/)
{
	bool bRet = true;

	if ( m_bStop == true )
		return bRet;

	try
	{
#if OPEN_CONSOLE
		cout << "Starting GK Initialization..." << endl ;
#endif

		m_uNumOfClusters = i_uNumOfClusters;
		m_dWeightingExponent = i_dWE;

		(m_dWeightingExponent >= 2.0) ? true : throw 0 ;

		m_dTerminationTolerance = i_dEpsilon;
		m_dGamma = i_dGamma;
		m_dBeta = i_dBeta;

		(m_dBeta != 0.0) ? true : throw 0;
		
		m_DataSetMatrix = i_DataSet;
		
		preprocessData();

		m_uSizeOfDataSet = m_DataSetMatrix.rows();
		m_uDimension = m_DataSetMatrix.cols();

		if( m_uDimension <= 0)
		{
			throw 1;
		}

		m_ScaledIdentityMatrix = MatrixXd::Zero( m_DataSetMatrix.cols(), m_DataSetMatrix.cols());// .1, 1);

		// Calculate Identity-like matrix (based on the implementation of algorithm in MatLab)
		covarianceMatrix(m_DataSetMatrix, m_ScaledIdentityMatrix);

		m_ScaledIdentityMatrix = (MatrixXd::Identity(m_uDimension, m_uDimension) * m_ScaledIdentityMatrix.determinant()).array().pow(1.0/m_uDimension);

		m_DistancesMatrices = MatrixXd::Zero(m_uSizeOfDataSet, m_uNumOfClusters);

		m_pPartitionMatrix->setConstant(m_uSizeOfDataSet, m_uNumOfClusters, 0.0);
		m_pPartitionMatrixPrev->setConstant(m_uSizeOfDataSet, m_uNumOfClusters, 0.0);

		if ( !SIMPLE_SET )
		{
			generateRandomPartitionMatrix();
		}
		else
		{
			//(*m_pPartitionMatrix).setConstant(1.0/m_uNumOfClusters);

			(*m_pPartitionMatrix)(0,0) = 1.0/6.0;
			(*m_pPartitionMatrix)(0,1) = 2.0/6.0;
			(*m_pPartitionMatrix)(0,2) = 3.0/6.0;

			(*m_pPartitionMatrix)(1,0) = 2.0/7.0;
			(*m_pPartitionMatrix)(1,1) = 4.0/7.0;
			(*m_pPartitionMatrix)(1,2) = 1.0/7.0;

			(*m_pPartitionMatrix)(2,0) = 3.0/8.0;
			(*m_pPartitionMatrix)(2,1) = 1.0/8.0;
			(*m_pPartitionMatrix)(2,2) = 4.0/8.0;

			(*m_pPartitionMatrix)(3,0) = 1.0/3.0;
			(*m_pPartitionMatrix)(3,1) = 1.0/3.0;
			(*m_pPartitionMatrix)(3,2) = 1.0/3.0;

			(*m_pPartitionMatrix)(4,0) = 1.0/4.0;
			(*m_pPartitionMatrix)(4,1) = 1.0/2.0;
			(*m_pPartitionMatrix)(4,2) = 1.0/4.0;

			(*m_pPartitionMatrix)(5,0) = 1.0/10.0;
			(*m_pPartitionMatrix)(5,1) = 3.0/10.0;
			(*m_pPartitionMatrix)(5,2) = 6.0/10.0;
		}

		*m_pPartitionMatrixPrev = *m_pPartitionMatrix;

#if OPEN_CONSOLE
	#if WRITE_TO_FILE
		cout << "Dumping data to output file" << endl ;
	#endif
#endif

#if WRITE_TO_FILE
		CFileOutput::m_foutput.open(FILE_PATH, ios::out | ios::app);
 		CFileOutput::m_foutput << "Input:" << endl ;
		CFileOutput::m_foutput << "Number of clusters:" << "=" << m_uNumOfClusters << endl;
		CFileOutput::m_foutput << "Weighting Exponent:" << "=" << m_dWeightingExponent << endl;
		CFileOutput::m_foutput << "Epsilon:" << "=" << m_dTerminationTolerance << endl;

		CFileOutput::m_foutput << "Scaled Identity Matrix:" << endl;
		CFileOutput::m_foutput << m_ScaledIdentityMatrix << endl << endl;

		CFileOutput::m_foutput << "Partition Matrix:" << endl;
		CFileOutput::m_foutput << *m_pPartitionMatrix << endl << endl;

		CFileOutput::m_foutput << "Prev Partition Matrix:" << endl;
		CFileOutput::m_foutput << *m_pPartitionMatrixPrev << endl << endl;

		CFileOutput::m_foutput << "Data Set Matrix:" << endl;
		CFileOutput::m_foutput << m_DataSetMatrix << endl << endl;
		CFileOutput::m_foutput.close();
#endif

#if OPEN_CONSOLE
	#if WRITE_TO_FILE
		cout << "Completed Dumping!" << endl ;
	#endif
		cout << "Completed Initialization!!!" << endl ;
#endif

	}
	catch (...)
	{
		TRACE(_T("Exception caught in CGKCluster::Initialize()"));
		_ASSERT(nullptr);
		bRet = false;
	}

	return bRet;
}

bool CGKCluster::FindClusters()
{
	bool bRet = true;

	if ( m_bStop == true )
		return bRet;

	try
	{
#if OPEN_CONSOLE
		cout << "Starting GK Clustering..." << endl ;
		clock_t start_t, finish_t, loop_start, loop_finish;
		double  duration_t;
		start_t = clock();
#endif

		UINT uIters = 0;
		double dCond = 0.0, dTemp = 0.0;

		VectorXd RowSum(m_uNumOfClusters, 1);
		MatrixXd clusterCovMatrix, setMinusCenter, tempMatrix;
		clusterCovMatrix = setMinusCenter = tempMatrix = MatrixXd::Zero(m_uDimension, m_uDimension);

		VectorXd tempVector(m_uNumOfClusters, 1);
		FullPivLU<MatrixXd> luMatrix(m_uDimension, m_uDimension); // for inversion

		SelfAdjointEigenSolver<MatrixXd> saEigenSolver(m_uDimension*m_uDimension);

		MatrixXd* pTemp = nullptr;
		MatrixXd TranspPartMatPow(m_uNumOfClusters, m_uSizeOfDataSet);
		MatrixXd tempDistancesMatrices(m_uSizeOfDataSet, m_uNumOfClusters);
		while( m_bStop == false )
		{

#if OPEN_CONSOLE
			loop_start = clock();
#endif
			// First swap between the pointers to partition matrices
			m_pPartitionMatrix		= (MatrixXd*) ((int)(m_pPartitionMatrix) ^ (int)(m_pPartitionMatrixPrev));
			m_pPartitionMatrixPrev	= (MatrixXd*) ((int)m_pPartitionMatrixPrev ^ (int)m_pPartitionMatrix);
			m_pPartitionMatrix		= (MatrixXd*) ((int)m_pPartitionMatrix ^ (int)m_pPartitionMatrixPrev);

			////////// Step 1 - Clusters centers ///////////
			// To calculate normalized centers, we use operations on matrices based on matrices operations
			// Prepare partitions matrix: transpose and power by exponent to get cxN matrix
			TranspPartMatPow = (m_pPartitionMatrixPrev->transpose()).array().pow(m_dWeightingExponent);

			// cxN * Nxn => cxn
			m_CentersMatrix.noalias() = TranspPartMatPow * m_DataSetMatrix;

			// To normalize the centers, create a vector which holds in each row,
			// the sum of powered partitions of each cluster
			RowSum = TranspPartMatPow.rowwise().sum();

			// Normalize centers
			for(UINT uClusterIndex = 0 ; uClusterIndex < m_uNumOfClusters ; ++uClusterIndex )
			{
				m_CentersMatrix.row(uClusterIndex) = m_CentersMatrix.row(uClusterIndex) / RowSum(uClusterIndex,0);
			}

			//////// Step 2 - Compute the cluster covariance matrices /////////

			//////// Step 2.1 - covariance matrices calculation /////////
			// For each cluster, a covariance matrix is calculated:
			// 1) For each data point, subtract the cluster's center to get a Subtract vector
			// 2) Multiply the 'subtract' vector by it's transposed vector to get a matrix (nxn)
			// 3) Multiply each element in the matrix by the weighted partition factor
			//    of the data point of the cluster
			// 4) Sum up all the matrices to one matrix
			// 5) Divide each element in the matrix by the summed weighted partition
			//    factors of all data points of the cluster

			for(UINT uClusterIndex = 0 ; uClusterIndex < m_uNumOfClusters ; ++uClusterIndex )
			{
				// Here we go...
				// 1) For each data point, subtract the cluster's center to get a Subtract vector (in one operation)
				setMinusCenter.noalias() = m_DataSetMatrix.rowwise() - m_CentersMatrix.row(uClusterIndex);

				// Used to sum the covariance "sub-matrices" along the computation
				tempMatrix.setZero(m_uDimension, m_uDimension);

				for ( UINT uDataPointIndex = 0 ; uDataPointIndex < m_uSizeOfDataSet ; ++ uDataPointIndex )
				{
					tempVector = setMinusCenter.row(uDataPointIndex);

					// 2) Multiply the 'subtract' vector by it's transposed vector to get a matrix (nxn)
					// 3) Multiply each element in the matrix by the weighted partition factor
					//    of the data point of the cluster 
					// 4) Sum up all the matrices to one matrix
 					tempMatrix.noalias() += (tempVector * tempVector.transpose()) * TranspPartMatPow(uClusterIndex, uDataPointIndex);
				}
				// 5) Divide each element in the matrix by the summed weighted partition
				//    factors of all data points of the cluster
				clusterCovMatrix.noalias() = tempMatrix / RowSum(uClusterIndex,0);

				//////// Step 2.2 - Add scaled identity matrix /////////
				clusterCovMatrix = clusterCovMatrix * ( 1.0 - m_dGamma ) + (m_ScaledIdentityMatrix.array().pow(1.0/m_uDimension) * m_dGamma).matrix() ;

				// If the ratio between the largest singular value of the matrix and the smallest
				// then, limit the ratio using eigenvalues
				dCond = calculateSingularRatio(clusterCovMatrix);
				if ( dCond > m_dBeta ) // if singular then dCond = DBL_MAX
				{					
					//////// Step 2.3 - Extract eigenvalues and eigenvectors /////////

					// Note: clusterCovMatrix is self-adjoint matrix (actually a symmetric matrix)
					// and it is better to use SelfAdjointEigenSolver class than EigenSolver
					// Quoting from Eigen's documentation: "The algorithm exploits the fact that the matrix
					// is self-adjoint, making it faster and more accurate than the general purpose
					// eigenvalue algorithms implemented in EigenSolver and ComplexEigenSolver"

					// Note: eigenvalues and eigenvectors are computed inside the constructor!
					//SelfAdjointEigenSolver<MatrixXd> saEigenSolver(clusterCovMatrix);
					saEigenSolver.compute(clusterCovMatrix);
					const MatrixXd& eigenvectors = saEigenSolver.eigenvectors();

					tempVector = saEigenSolver.eigenvalues();

					// At this point m_dBeta cannot be zero
					_ASSERT(m_dBeta != 0.0);
					dTemp = saEigenSolver.eigenvalues().maxCoeff() / m_dBeta;
					
					// For each eigenvalue, if it is less than maximal eigenvalue divided by beta, 
					// set it to this ratio, otherwise leave it as is.
					tempVector = tempVector.cwiseMax( VectorXd::Constant(tempVector.rows(), tempVector.cols(), dTemp) );

					//////// Step 2.4 - Reconstruct covariance matrices /////////
					clusterCovMatrix = eigenvectors * tempVector.asDiagonal() * eigenvectors.transpose();
				}

				//////// Step 3 - Compute Distances /////////
				luMatrix.compute(clusterCovMatrix);

				if ( luMatrix.isInvertible() == false )
				{
					throw 1;
				}

				// Used for the inverse matrix
				tempMatrix = luMatrix.inverse();
				
				// Currently, rho is set to 1 for all clusters
				tempMatrix = pow(1.0 * clusterCovMatrix.determinant() , 1.0/m_uDimension ) * tempMatrix;
				
				// Each row is the substitution of one data set from cluster's center
				m_DistancesMatrices.col(uClusterIndex) = (setMinusCenter*tempMatrix*setMinusCenter.transpose()).diagonal();
			}
			
			//////// Step 4 - Update partition matrix /////////
			// Note m_dWeightingExponent must be at least 2.0
			// The last step is to update the partition matrix. 
			// The equation described in the article can be simplified as described below 
			tempDistancesMatrices = (m_DistancesMatrices.array() + DIVISION_EPSILON).array().pow(-1.0/(m_dWeightingExponent-1.0));
			(*m_pPartitionMatrix) = tempDistancesMatrices.array() / ((tempDistancesMatrices.rowwise().sum()).rowwise().replicate(m_uNumOfClusters)).array();

			++uIters;

#if OPEN_CONSOLE
			loop_finish = clock();
			duration_t = (double)(loop_finish - loop_start) / CLOCKS_PER_SEC * 1000; // milliseconds
			cout << "	GK clustering - Completed iteration " << uIters << ". Loop time: " << duration_t << " ms" << endl;
#endif		
			// Check stop criterion
			if ( (*m_pPartitionMatrix - *m_pPartitionMatrixPrev).maxCoeff() <= m_dTerminationTolerance )
				break;
		}

		// Calculate the real distances (square root of m_DistancesMatrices) 
		m_DistancesMatrices = m_DistancesMatrices.array().sqrt();

#if OPEN_CONSOLE
		finish_t = clock();
		duration_t = (double)(finish_t - start_t) / CLOCKS_PER_SEC * 1000; // milliseconds

		cout << "GK Clustering completed " << endl ;
		cout << "Total time: " << duration_t << " ms" << endl;
	
	#if WRITE_TO_FILE
		cout << "Dumping data to output file" << endl;
	#endif
#endif

#if WRITE_TO_FILE
		CFileOutput::m_foutput.open(FILE_PATH, ios::out | ios::app);

		CFileOutput::m_foutput << "Number of iterations:" << endl;
		CFileOutput::m_foutput << uIters << endl << endl;

		CFileOutput::m_foutput << "Cluster's centers matrix:" << endl ;
		CFileOutput::m_foutput << m_CentersMatrix << endl << endl;

		CFileOutput::m_foutput << "Partition Matrix:" << endl;
		CFileOutput::m_foutput << *m_pPartitionMatrix << endl << endl;

		CFileOutput::m_foutput << "Distances matrix:" << endl ;
		CFileOutput::m_foutput << m_DistancesMatrices << endl << endl;

		CFileOutput::m_foutput.close();
#endif

#if OPEN_CONSOLE
	#if WRITE_TO_FILE
		cout << "Completed Dumping!" << endl ;
	#endif
		cout << "Completed clustering!!!" << endl << endl;
#endif
	}
	catch (...)
	{
		TRACE(_T("Exception caught in CGKCluster::FindClusters()"));
		_ASSERT(nullptr);
		bRet = false;
	}

	return bRet;
}

const MatrixXd& CGKCluster::GetPartitionMatrix()
{
	return *m_pPartitionMatrix;
}

const MatrixXd& CGKCluster::GetDistanceMatrix()
{
	return m_DistancesMatrices;
}

const MatrixXd& CGKCluster::GetCentersMatrix()
{
	return m_CentersMatrix;
}

bool CGKCluster::SaveDataSet()
{
	bool bRet = true;
	try
	{
		ofstream fDataSetFile;
		fDataSetFile.open(DATA_SET_PATH, ios::out | ios::app);
		fDataSetFile << m_DataSetMatrix;
		fDataSetFile.close();
	}
	catch (...)
	{
		bRet = false;
		TRACE(_T("Exception caught in CGKCluster::SaveDataSet()"));
		_ASSERT(nullptr);
	}

	return bRet;
}

bool CGKCluster::SavePartitionMatrix()
{
	bool bRet = true;
	try
	{
		ofstream fPartitionMatrixOutputFile;
		fPartitionMatrixOutputFile.open(PARTITION_MATRIX_PATH, ios::out | ios::trunc);
		fPartitionMatrixOutputFile << *m_pPartitionMatrix;
		fPartitionMatrixOutputFile.close();
	}
	catch (...)
	{
		bRet = false;
		TRACE(_T("Exception caught in CGKCluster::SavePartitionMatrix()"));
		_ASSERT(nullptr);
	}

	return bRet;
}

void CGKCluster::preprocessData()
{
	// First calculate Standard deviation to find atrophied dimensions
	RowVectorXd meanVec = m_DataSetMatrix.colwise().mean();
	RowVectorXd varVec = ( ( (m_DataSetMatrix.rowwise() - meanVec).array().pow(2.0) ).colwise().sum() )  / (m_DataSetMatrix.rows() - 1) ;
	
	// Second, remove dimensions with variance = 0 
	Array<bool, 1, Dynamic> boolArr = varVec.array() > (RowVectorXd::Constant(1, varVec.cols(), ZERO_EPSILON)).array();

	// If there are 'false' columns it means there are atrophied dimensions which need to be removed
	int iSize = boolArr.cols();
	if ( boolArr.count() < iSize )
	{
		MatrixXd TempMat = MatrixXd::Zero(m_DataSetMatrix.rows(), boolArr.count());

		int i = 0 , j = 0;
		for ( int i = 0 ; i < iSize ; ++i )
		{
			if ( boolArr(i) == true  )
				TempMat.col(j++) = m_DataSetMatrix.col(i);
		}
		m_DataSetMatrix = TempMat;
	}

	// Next, normalize each dimension
	RowVectorXd maxVec = (m_DataSetMatrix.array().abs()).colwise().maxCoeff();
  	iSize = m_DataSetMatrix.cols();
  	for ( int i = 0 ; i < iSize ; ++i )
  	{
 		m_DataSetMatrix.col(i) = (m_DataSetMatrix.col(i)).array() / maxVec(i);
	}
}

void CGKCluster::generateRandomPartitionMatrix()
{
	applyTestMode();

	// First, set all coefficients to random values
	(*m_pPartitionMatrix).setRandom();
	(*m_pPartitionMatrix).noalias() = (*m_pPartitionMatrix).cwiseAbs();

	// Second, "normalize" each row such that the sum of coefficients is 1.0
	// Sum of each row cannot be zero sue to absolute value 
	VectorXd rowsFrac = m_pPartitionMatrix->rowwise().sum();
	*m_pPartitionMatrix = (*m_pPartitionMatrix).cwiseQuotient(rowsFrac.replicate(1,m_uNumOfClusters));

#if WRITE_PARTITION_MATRIX
	SavePartitionMatrix();
#endif
}

void CGKCluster::covarianceMatrix(MatrixXd& i_Matrix, MatrixXd& o_CovMatrix)
{
	try
	{
		RowVectorXd Temp = i_Matrix.colwise().mean();
		o_CovMatrix.noalias() = i_Matrix.rowwise() - Temp;

		int iSetSize = i_Matrix.rows();

		if ( iSetSize > 1 )
		{
			o_CovMatrix.noalias() = ( o_CovMatrix.transpose() * o_CovMatrix ) / (iSetSize - 1);
		}
		else if ( iSetSize > 0 )
		{
			o_CovMatrix.noalias() = ( o_CovMatrix.transpose() * o_CovMatrix ) / iSetSize ;
		}
		else
		{
			o_CovMatrix.setZero();			
		}
	}
	catch (...)
	{
		TRACE(_T("Exception caught in CGKCluster::covarianceMatrix()"));
		_ASSERT(nullptr);
	}
}

double CGKCluster::calculateSingularRatio(MatrixXd& i_refMatrix)
{
	JacobiSVD<MatrixXd> svd(i_refMatrix);
	VectorXd singularVec = svd.singularValues();

	if ( (singularVec.array() == ArrayXd::Zero(singularVec.size())).any() )
		return DBL_MAX;
	else
		return singularVec.maxCoeff() / singularVec.minCoeff();
}

void CGKCluster::openConsole()
{
	if (!AllocConsole())
	{
		AfxMessageBox(_T("Failed to create the console!"), MB_ICONEXCLAMATION);
	}
	else
	{
		std::cout.sync_with_stdio(true);
		FILE *hf;
		int hCrt = _open_osfhandle(	(long) GetStdHandle(STD_OUTPUT_HANDLE),	_O_TEXT);
		hf = _fdopen( hCrt, "w" );
		*stdout = *hf;
		setvbuf( stdout, nullptr, _IONBF, 0 );

		std::cout << "Console Created successfully\n";
	}
}

void CGKCluster::closeConsole()
{
	if (!FreeConsole())
		AfxMessageBox(_T("Failed to free the console!"), MB_ICONEXCLAMATION);
}

void CGKCluster::applyTestMode()
{
	if ( m_bTestMode == true )
		srand(1);
	else
		srand( (unsigned)time( nullptr ) );
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CFuzSam::CFuzSam() :
m_uNumOfPoints(0),
m_uNumOfClusters(0),
m_dGradientStepSize(0.4),
m_uNumOfSteps(2000),
m_uDimension(2),
m_bTestMode(false),
m_bStop(false)
{
	applyTestMode();
}

CFuzSam::~CFuzSam()
{
}

void CFuzSam::SetTestMode(bool i_bIsTestMode)
{
	m_bTestMode = i_bIsTestMode;
}


void CFuzSam::StopCalcs(bool i_bStop)
{
	m_bStop = i_bStop;
}

bool CFuzSam::Initialize(const MatrixXd& i_refDistancesMatrices, const MatrixXd& i_refPartitionMatrix,
						 double i_dWE /*=2*/, UINT i_uDimension /*=2*/, double i_dGradientStepSize /*=0.4*/,
						 UINT i_uNumOfSteps /*= 2000*/)
{
	bool bRet = true;

	if ( m_bStop == true )
		return bRet;

	try
	{
#if OPEN_CONSOLE
		cout << "Starting Fuzzy Sammon Initialization..." << endl ;
#endif

		m_PartitionMatrix = i_refPartitionMatrix;
		m_DistancesMatrix = i_refDistancesMatrices;

		m_uNumOfPoints = m_DistancesMatrix.rows();
		m_uNumOfClusters = m_DistancesMatrix.cols();

		m_dGradientStepSize = i_dGradientStepSize;
		m_uDimension = i_uDimension;
		m_uNumOfSteps = i_uNumOfSteps;

		//////////////////////////////////////////////////////////////////////////
		// Step 1 - Initialization 
		//////////////////////////////////////////////////////////////////////////
		
		// Step 1.1 - Generate a matrix of random projected points between -0.5 and 0.5
		m_ProjectedDataSet = MatrixXd::Zero(m_uNumOfPoints, i_uDimension);	// = x Or P or y
		m_ProjectedCenters = MatrixXd::Zero(m_uNumOfClusters, i_uDimension);  // = xv Or or z

		if ( !SIMPLE_SET )
		{
			//srand( (unsigned)time( nullptr ) );
			applyTestMode();
			for ( UINT i = 0 ; i < m_uNumOfPoints ; ++i )
			{
				for ( UINT j = 0 ; j < i_uDimension ; ++j )
				{
					m_ProjectedDataSet(i,j)	= (double)rand() / (RAND_MAX + 1) - 0.5;
				}
			}

			#if WRITE_PROJECTION_MATRIX
			SaveProjectionMatrices();
			#endif
		}
		else
		{
			m_ProjectedDataSet(0,0) = 0.25;
			m_ProjectedDataSet(0,1) = 0.45;

			m_ProjectedDataSet(1,0) = 0.02;
			m_ProjectedDataSet(1,1) = 0.003;

			m_ProjectedDataSet(2,0) = 0.38;
			m_ProjectedDataSet(2,1) = 0.17;

			m_ProjectedDataSet(3,0) = 12.0;
			m_ProjectedDataSet(3,1) = 11.0;

			m_ProjectedDataSet(4,0) = 13.2;
			m_ProjectedDataSet(4,1) = 12.25;

			m_ProjectedDataSet(5,0) = 11.2;
			m_ProjectedDataSet(5,1) = 11.5;
		}

		// Step 1.2 - from this point on, the partition matrix is used in the form of 
		// power-by-weighting exponent so it is prepared here. 
		m_PartitionMatrix = m_PartitionMatrix.array().pow(i_dWE);

		// Step 1.3 - Calculate the initial projected centers of all clusters
		// sum up each cluster's powered partitions
		m_SumPoweredPartitions = m_PartitionMatrix.colwise().sum();
		
		// sum up all multiplied points by the powered partition (numerator)
		m_ProjectedCenters = m_PartitionMatrix.transpose() * m_ProjectedDataSet ;

		// Step 1.4 - Normalize the projected centers
		for(UINT uClusterIndex = 0 ; uClusterIndex < m_uNumOfClusters ; ++uClusterIndex )
		{
			m_ProjectedCenters.row(uClusterIndex) /= m_SumPoweredPartitions(uClusterIndex, 0);
		}

		// Step 1.5 - move the centers of mass to the center
		RowVectorXd AvrgProjectedPoints = m_ProjectedDataSet.colwise().mean();
		m_ProjectedDataSet.noalias() = m_ProjectedDataSet.rowwise() - AvrgProjectedPoints;
		m_ProjectedCenters.noalias() = m_ProjectedCenters.rowwise() - AvrgProjectedPoints;

  		// Initialize data members required for projection
		m_EuclideanDisVec = m_DiffDistancesVec = VectorXd::Zero(m_uNumOfClusters, 1);
		m_TransposedPoweredDistVec = m_TransposedPoweredPartVec = VectorXd::Zero(m_uNumOfClusters, 1);
		m_FirstDerivative = m_SecondDerivative = VectorXd::Zero(m_uDimension, 1);
		m_DiffPointToCentersMatrix = m_PoweredDiffPointToCentersMatrix = MatrixXd::Zero(m_uNumOfClusters, m_uDimension);
		m_TempRowVec = RowVectorXd::Constant(1, m_uDimension, 0);

#if OPEN_CONSOLE
		cout << "Completed Fuzzy Sammon Initialization!" << endl ;
#endif

	}
	catch (...)
	{
		bRet = false;
		TRACE(_T("Exception caught in CFuzSam::Initialize()"));
		_ASSERT(nullptr);
	}

	return bRet;
}

bool CFuzSam::StepProjection(UINT i_uIndex /*= UINT_MAX*/)
{
	bool bRet = true;

	if ( m_bStop == true )
		return bRet;

	try
	{	
#if OPEN_CONSOLE
		clock_t loop_finish, loop_start;
		double  duration_t;
		loop_start = clock();
#endif

		double dVal = 0.0;
		VectorXd TempVec = VectorXd::Zero(m_uNumOfClusters, 1);

		for( UINT uPointIndex = 0 ; uPointIndex < m_uNumOfPoints ; ++uPointIndex )
		{
			m_TransposedPoweredDistVec = (m_DistancesMatrix.row(uPointIndex)).transpose();
			m_TransposedPoweredPartVec = (m_PartitionMatrix.row(uPointIndex)).transpose();

			m_DiffPointToCentersMatrix.noalias() = (m_ProjectedCenters.rowwise() - m_ProjectedDataSet.row(uPointIndex)) * -1.0;
			m_PoweredDiffPointToCentersMatrix = m_DiffPointToCentersMatrix.array().pow(2);
			m_EuclideanDisVec.noalias() = (m_DiffPointToCentersMatrix.rowwise().norm());
			m_DiffDistancesVec.noalias() = m_TransposedPoweredDistVec - m_EuclideanDisVec;
#if 1 
			m_FirstDerivative.setZero();
			m_SecondDerivative.setZero();
			TempVec.setZero();

			// j = cluster index
			for( UINT j = 0 ; j < m_uNumOfClusters ; ++j )
			{
				// If a distance between a cluster and a point in q-dimension is zero, move to next cluster
				if ( m_EuclideanDisVec(j,0) < ZERO_EPSILON )
					continue;

				// Otherwise continue calculations
				// Calculate first derivative
				// Note - m_PartitionMatrix is already powered! 
				TempVec(j,0) = m_DiffDistancesVec(j,0) * m_TransposedPoweredPartVec(j,0) / m_EuclideanDisVec(j,0);
				m_FirstDerivative.noalias() += m_DiffPointToCentersMatrix.row(j) * TempVec(j,0);

				// Calculate Second Derivative 
				// Devision by Zero is not possible here duo to the check above!
				dVal = m_EuclideanDisVec(j,0) * m_EuclideanDisVec(j,0) * m_EuclideanDisVec(j,0);
				m_SecondDerivative.noalias() += m_PoweredDiffPointToCentersMatrix.row(j) * ( m_TransposedPoweredPartVec(j,0) * (m_EuclideanDisVec(j,0) + m_DiffDistancesVec(j,0))/dVal);
			}

			// Complete second derivative calculations
			m_SecondDerivative.noalias() = -1.0 * ( m_SecondDerivative.rowwise() - TempVec.colwise().sum());

			m_ProjectedDataSet.row(uPointIndex) = m_ProjectedDataSet.row(uPointIndex) + (m_dGradientStepSize * ( m_FirstDerivative.cwiseQuotient(m_SecondDerivative.array().abs().matrix()))).transpose();

#elif 0
			m_FirstDerivative.setZero();
			m_SecondDerivative.setZero();

			// j = cluster index
			for( UINT j = 0 ; j < m_uNumOfClusters ; ++j )
			{
				// If a distance between a cluster and a point in q-dimension is zero, move to next cluster
				if ( m_EuclideanDisVec(j,0) < ZERO_EPSILON )
					continue;

				// Otherwise continue calculations
				// Calculate first derivative
				// Note - m_PartitionMatrix is already powered! 
				m_FirstDerivative.noalias() += m_DiffPointToCentersMatrix.row(j) * ( m_DiffDistancesVec(j,0)*m_TransposedPoweredPartVec(j,0) / m_EuclideanDisVec(j,0) );

				// Calculate Second Derivative
				dVal = m_TransposedPoweredDistVec(j,0) * m_TransposedPoweredPartVec(j,0) / pow(m_EuclideanDisVec(j,0), 3.0);
				m_TempRowVec.setConstant(pow(m_EuclideanDisVec(j,0), 2.0));
				m_SecondDerivative.noalias() = m_SecondDerivative +
											 ( 
												((m_TempRowVec - m_PoweredDiffPointToCentersMatrix.row(j)).transpose() * dVal).array() - m_TransposedPoweredPartVec(j,0)
											 ).matrix();
			}
						
 			cout << "First Dev:\n";
 			cout << m_FirstDerivative << endl << endl;
 			cout << "Second Dev:\n";
 			cout << m_SecondDerivative << endl << endl;

			//m_ProjectedDataSet.row(uPointIndex) = m_ProjectedDataSet.row(uPointIndex) + (m_dGradientStepSize * ( m_FirstDerivative.cwiseQuotient(m_SecondDerivative.cwise().abs()))).transpose();
			TempProjectedDataSet.row(uPointIndex) = m_ProjectedDataSet.row(uPointIndex) + (m_dGradientStepSize * ( m_FirstDerivative.cwiseQuotient(m_SecondDerivative.cwise().abs()))).transpose();
			
			VectorXd temp = VectorXd::Zero(m_uDimension, 1);
			temp = (m_dGradientStepSize * ( m_FirstDerivative.cwiseQuotient(m_SecondDerivative.cwise().abs()))).transpose();
			cout << "Temp:\n" ;
			cout << temp << endl << endl;

			cout << "Projected Data Set After internal loop:\n";
			cout << TempProjectedDataSet << endl << endl;
 			int jkjjk = 0;

#else
			// This section of code does the same as the above but here it does not check for zero distances and 
			// all calculations are done per data point for all clusters.
			// This code is riskier and surprisingly (or not) slower.
			// The code is left here just to demonstrate another way of calculating the derivaties and the new data set
			VectorXd tempVec = VectorXd::Zero(m_uNumOfClusters, 1);
			MatrixXd tempDerivative = MatrixXd::Zero(m_uNumOfClusters, m_uDimension);
			_ASSERT( (m_EuclideanDisVec.array() == ArrayXd::Zero(m_EuclideanDisVec.size())).any() == false );

			// Calculate first derivative
			// Note - m_PartitionMatrix is already powered! 
			tempVec.noalias() = m_DiffDistancesVec.cwiseProduct( m_TransposedPoweredPartVec  );
			tempVec.noalias() = tempVec.cwiseQuotient(m_EuclideanDisVec);
			m_FirstDerivative.noalias() = (m_DiffPointToCentersMatrix.cwiseProduct(tempVec.replicate(1, m_uDimension))).colwise().sum();

 			//cout << "First derivative:\n";
 			//cout << m_FirstDerivative << endl << endl;

			tempVec.noalias() = m_TransposedPoweredDistVec.cwiseProduct(m_TransposedPoweredPartVec.cwiseQuotient( m_EuclideanDisVec.cwise().pow(3) ));
			tempDerivative.noalias() = (m_EuclideanDisVec.cwise().pow(2)).replicate(1, m_uDimension) - m_PoweredDiffPointToCentersMatrix ;

			m_SecondDerivative.noalias() = ( (tempDerivative.cwiseProduct(tempVec.replicate(1, m_uDimension))) - m_TransposedPoweredPartVec.replicate(1, m_uDimension)  ).colwise().sum();

 			//cout << "Second derivative:\n";
 			//cout << m_SecondDerivative << endl << endl;

			m_ProjectedDataSet.row(uPointIndex) = m_ProjectedDataSet.row(uPointIndex) + (m_dGradientStepSize * ( m_FirstDerivative.cwiseQuotient(m_SecondDerivative))).transpose();
#endif
		}
		m_TempRowVec = m_ProjectedDataSet.colwise().mean();
		m_ProjectedDataSet.noalias() = m_ProjectedDataSet.rowwise() - m_TempRowVec;
		m_ProjectedCenters.noalias() = (m_PartitionMatrix.transpose() * m_ProjectedDataSet ).cwiseQuotient( m_SumPoweredPartitions.replicate(1, m_uDimension) );

#if OPEN_CONSOLE
		loop_finish = clock();
		duration_t = (double)(loop_finish - loop_start) / CLOCKS_PER_SEC * 1000; // milliseconds
		if ( i_uIndex != UINT_MAX )
			cout << "	Fuzzy Sammon - Completed iteration " << i_uIndex << ". Loop time: " << duration_t << " ms" << endl;
		else
			cout << "	Fuzzy Sammon - Step iteration. Time: " << duration_t << " ms" << endl;
#endif

	}
	catch (...)
	{
		TRACE(_T("Exception caught in CFuzSam::StepProjection()"));
		_ASSERT(nullptr);
		bRet = false;
	}
	return bRet;
}

bool CFuzSam::CompleteProjection()
{
	bool bRet = true;

	if ( m_bStop == true )
		return bRet;

	try
	{

#if OPEN_CONSOLE
		cout << "Starting complete Fuzzy Sammon projection calculation..." << endl;
		clock_t start_t, finish_t;
		double  duration_t;
		start_t = clock();
#endif
		for ( UINT uIndex = 0 ; uIndex < m_uNumOfSteps && m_bStop == false ; ++uIndex )
		{
			if( (bRet = StepProjection(uIndex)) == false )
				break;		
		}
#if OPEN_CONSOLE
		finish_t = clock();
		duration_t = (double)(finish_t - start_t) / CLOCKS_PER_SEC * 1000; // milliseconds
		cout << "Fuzzy Sammon projection completed " << endl ;
		cout << "Total time: " << duration_t << " ms" << endl;
#endif
		ReduceMemUsage();

#if WRITE_TO_FILE
	
	#if OPEN_CONSOLE
		cout << "Fuzzy Sammon - Dumping data to output file" << endl ;
	#endif

		CFileOutput::m_foutput.open(FILE_PATH, ios::out | ios::app);

 		CFileOutput::m_foutput << "Projected data set:\n";
 		CFileOutput::m_foutput << m_ProjectedDataSet << endl << endl;
 
 		CFileOutput::m_foutput << "Projected centers:\n";
 		CFileOutput::m_foutput << m_ProjectedCenters << endl << endl;			

		CFileOutput::m_foutput.close();

	#if OPEN_CONSOLE
		cout << "Fuzzy Sammon - Completed Dumping!" << endl ;
	#endif
#endif
	}
	catch (...)
	{
		TRACE(_T("Exception caught in CFuzSam::CompleteProjection()"));
		_ASSERT(nullptr);
		bRet = false;
	}

	return bRet;
}

void CFuzSam::ReduceMemUsage()
{
	m_EuclideanDisVec.setIdentity(1,1);
	m_DiffDistancesVec.setIdentity(1,1);
 	m_TransposedPoweredDistVec.setIdentity(1,1);
	m_TransposedPoweredPartVec.setIdentity(1,1);
	m_FirstDerivative.setIdentity(1,1);
	m_SecondDerivative.setIdentity(1,1);
	m_DiffPointToCentersMatrix.setIdentity(1,1);
	m_PoweredDiffPointToCentersMatrix.setIdentity(1,1);
	m_TempRowVec.setIdentity(1,1);
}

const MatrixXd& CFuzSam::GetProjectedCenters()
{
	return m_ProjectedCenters;
}

const MatrixXd& CFuzSam::GetProjectedDataSet()
{
	return m_ProjectedDataSet;
}

bool CFuzSam::SaveProjectionMatrices()
{
	bool bRet = true;
	try
	{
		ofstream fProjectionMatrixOutputFile;
		fProjectionMatrixOutputFile.open(PROJECTION_MATRIX_PATH, ios::out | ios::trunc);
		fProjectionMatrixOutputFile << m_ProjectedDataSet;
		fProjectionMatrixOutputFile.close();
	}
	catch (...)
	{
		bRet = false;
		TRACE(_T("Exception caught in CFuzSam::SaveProjectionMatrices()"));
		_ASSERT(nullptr);
	}

	return bRet;
}

void CFuzSam::applyTestMode()
{
	if ( m_bTestMode == true )
		srand(1);
	else
		srand( (unsigned)time( nullptr ) );
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#if 0
// OR - Mathematically it gives the same result BUT IT IS MUCH SLOWER!!!
start_t = clock();
GetSystemTime(&start);
//To normalize the centers, create a vector which holds in each row, the sum of powered partitions of each cluster
//VectorXd RowSum2 = TranspPartMatPow.rowwise().sum();
//RowSum2 = RowSum2.cwise().pow(-1); 

CentersMatrix2 = ( (TranspPartMatPow.rowwise().sum()).cwise().pow(-1) ).asDiagonal() *CentersMatrix2; //RowSum2.asDiagonal()*CentersMatrix2;
finish_t = clock();
GetSystemTime(&finish);
duration = finish.wMilliseconds - start.wMilliseconds ;
duration_t = (double)(finish_t - start_t) / CLOCKS_PER_SEC * 1000;

cout << "Normalized Centers matrix (2):\n" << CentersMatrix2 << endl << endl;
cout << "Sum rows:\n" << (TranspPartMatPow.rowwise().sum()).cwise().pow(-1) /*RowSum2*/ << endl << endl;
cout << "Structural calculation time - systemtime:\n" << duration << endl << endl; //"%2.1f seconds\n", duration 
cout << "Structural calculation time - clock:\n" << duration_t << endl << endl; //"%2.1f seconds\n", duration 



// 		(*m_pPartitionMatrix)(0,0) = 0.8;
// 		(*m_pPartitionMatrix)(0,1) = 0.1;
// 		(*m_pPartitionMatrix)(0,2) = 0.1;
// 
// 		(*m_pPartitionMatrix)(1,0) = 0.1;
// 		(*m_pPartitionMatrix)(1,1) = 0.8;
// 		(*m_pPartitionMatrix)(1,2) = 0.1;
// 
// 		(*m_pPartitionMatrix)(2,0) = 0.1;
// 		(*m_pPartitionMatrix)(2,1) = 0.1;
// 		(*m_pPartitionMatrix)(2,2) = 0.8;


// 	
// 		(*m_pPartitionMatrix)(0,0) = 4.0/10.0;
// 		(*m_pPartitionMatrix)(0,1) = 1.0/10.0;
// 		(*m_pPartitionMatrix)(0,2) = 5.0/10.0;
// 
// 		(*m_pPartitionMatrix)(1,0) = 2.0/7.0;
// 		(*m_pPartitionMatrix)(1,1) = 4.0/7.0;
// 		(*m_pPartitionMatrix)(1,2) = 1.0/7.0;
// 
// 		(*m_pPartitionMatrix)(2,0) = 3.0/12.0;
// 		(*m_pPartitionMatrix)(2,1) = 5.0/12.0;
// 		(*m_pPartitionMatrix)(2,2) = 4.0/12.0;
// 
// 		(*m_pPartitionMatrix)(3,0) = 23.0/30.0;
// 		(*m_pPartitionMatrix)(3,1) = 3.0/30.0;
// 		(*m_pPartitionMatrix)(3,2) = 4.0/30.0;
// 
// 		(*m_pPartitionMatrix)(4,0) = 9.0/24.0;
// 		(*m_pPartitionMatrix)(4,1) = 4.0/24.0;
// 		(*m_pPartitionMatrix)(4,2) = 11.0/24.0;
// 
// 		(*m_pPartitionMatrix)(5,0) = 6.0/27.0;
// 		(*m_pPartitionMatrix)(5,1) = 13.0/27.0;
// 		(*m_pPartitionMatrix)(5,2) = 8.0/27.0;


// 		(*m_pPartitionMatrix)(0,0) = 1.0/10.0;
// 		(*m_pPartitionMatrix)(0,1) = 2.0/10.0;
// 		(*m_pPartitionMatrix)(0,2) = 4.0/10.0;
// 		(*m_pPartitionMatrix)(0,3) = 1.0/10.0;
// 		(*m_pPartitionMatrix)(0,4) = 2.0/10.0;
// 
// 		(*m_pPartitionMatrix)(1,0) = 2.0/7.0;
// 		(*m_pPartitionMatrix)(1,1) = 1.0/7.0;
// 		(*m_pPartitionMatrix)(1,2) = 1.0/7.0;
// 		(*m_pPartitionMatrix)(1,3) = 1.0/7.0;
// 		(*m_pPartitionMatrix)(1,4) = 2.0/7.0;
// 
// 		(*m_pPartitionMatrix)(2,0) = 3.0/12.0;
// 		(*m_pPartitionMatrix)(2,1) = 1.0/12.0;
// 		(*m_pPartitionMatrix)(2,2) = 4.0/12.0;
// 		(*m_pPartitionMatrix)(2,3) = 2.0/12.0;
// 		(*m_pPartitionMatrix)(2,4) = 2.0/12.0;
// 
// 		(*m_pPartitionMatrix)(3,0) = 15.0/30.0;
// 		(*m_pPartitionMatrix)(3,1) = 2.0/30.0;
// 		(*m_pPartitionMatrix)(3,2) = 1.0/30.0;
// 		(*m_pPartitionMatrix)(3,3) = 7.0/30.0;
// 		(*m_pPartitionMatrix)(3,4) = 5.0/30.0;
// 
// 		(*m_pPartitionMatrix)(4,0) = 9.0/24.0;
// 		(*m_pPartitionMatrix)(4,1) = 4.0/24.0;
// 		(*m_pPartitionMatrix)(4,2) = 3.0/24.0;
// 		(*m_pPartitionMatrix)(4,3) = 1.0/24.0;
// 		(*m_pPartitionMatrix)(4,4) = 7.0/24.0;
// 
// 		(*m_pPartitionMatrix)(5,0) = 6.0/27.0;
// 		(*m_pPartitionMatrix)(5,1) = 4.0/27.0;
// 		(*m_pPartitionMatrix)(5,2) = 8.0/27.0;
// 		(*m_pPartitionMatrix)(5,3) = 2.0/27.0;
// 		(*m_pPartitionMatrix)(5,4) = 7.0/27.0;
// 
// 		(*m_pPartitionMatrix)(0,0) = 1.0/10.0;
// 		(*m_pPartitionMatrix)(0,1) = 2.0/10.0;
// 		(*m_pPartitionMatrix)(0,2) = 4.0/10.0;
// 		(*m_pPartitionMatrix)(0,3) = 1.0/10.0;
// 		(*m_pPartitionMatrix)(0,4) = 2.0/10.0;
// 
// 		(*m_pPartitionMatrix)(1,0) = 2.0/7.0;
// 		(*m_pPartitionMatrix)(1,1) = 1.0/7.0;
// 		(*m_pPartitionMatrix)(1,2) = 1.0/7.0;
// 		(*m_pPartitionMatrix)(1,3) = 1.0/7.0;
// 		(*m_pPartitionMatrix)(1,4) = 2.0/7.0;
// 
// 		(*m_pPartitionMatrix)(2,0) = 3.0/12.0;
// 		(*m_pPartitionMatrix)(2,1) = 1.0/12.0;
// 		(*m_pPartitionMatrix)(2,2) = 4.0/12.0;
// 		(*m_pPartitionMatrix)(2,3) = 2.0/12.0;
// 		(*m_pPartitionMatrix)(2,4) = 2.0/12.0;
// 
// 		(*m_pPartitionMatrix)(3,0) = 15.0/30.0;
// 		(*m_pPartitionMatrix)(3,1) = 2.0/30.0;
// 		(*m_pPartitionMatrix)(3,2) = 1.0/30.0;
// 		(*m_pPartitionMatrix)(3,3) = 7.0/30.0;
// 		(*m_pPartitionMatrix)(3,4) = 5.0/30.0;
// 
// 		(*m_pPartitionMatrix)(4,0) = 9.0/24.0;
// 		(*m_pPartitionMatrix)(4,1) = 4.0/24.0;
// 		(*m_pPartitionMatrix)(4,2) = 3.0/24.0;
// 		(*m_pPartitionMatrix)(4,3) = 1.0/24.0;
// 		(*m_pPartitionMatrix)(4,4) = 7.0/24.0;
// 
// 		(*m_pPartitionMatrix)(5,0) = 6.0/27.0;
// 		(*m_pPartitionMatrix)(5,1) = 4.0/27.0;
// 		(*m_pPartitionMatrix)(5,2) = 8.0/27.0;
// 		(*m_pPartitionMatrix)(5,3) = 2.0/27.0;
// 		(*m_pPartitionMatrix)(5,4) = 7.0/27.0;
// 



// 	m_PartitionMatrix(0,0) = 1.0/6.0;
// 	m_PartitionMatrix(0,1) = 2.0/6.0;
// 	m_PartitionMatrix(0,2) = 3.0/6.0;
// 
// 	m_PartitionMatrix(1,0) = 2.0/7.0;
// 	m_PartitionMatrix(1,1) = 4.0/7.0;
// 	m_PartitionMatrix(1,2) = 1.0/7.0;
// 
// 	m_PartitionMatrix(2,0) = 3.0/8.0;
// 	m_PartitionMatrix(2,1) = 1.0/8.0;
// 	m_PartitionMatrix(2,2) = 4.0/8.0;
// 
// 	m_PartitionMatrix(3,0) = 1.0/3.0;
// 	m_PartitionMatrix(3,1) = 1.0/3.0;
// 	m_PartitionMatrix(3,2) = 1.0/3.0;
// 
// 	m_PartitionMatrix(4,0) = 1.0/4.0;
// 	m_PartitionMatrix(4,1) = 1.0/2.0;
// 	m_PartitionMatrix(4,2) = 1.0/4.0;
// 
// 	m_PartitionMatrix(5,0) = 1.0/10.0;
// 	m_PartitionMatrix(5,1) = 3.0/10.0;
// 	m_PartitionMatrix(5,2) = 6.0/10.0;
#endif

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
//////////////////////////////////////////////////////

CClusterTest::CClusterTest()
{
	//m_Matrix = new MatrixXd;
	//m_Vector = new VectorXcd;
}

CClusterTest::~CClusterTest()
{
	//delete m_Matrix;
	//delete m_Vector;
}

void CClusterTest::Build2X2SymMatrix()
{
	m_Matrix = MatrixXd::Random(2,2);
	m_Matrix(0,0) = 1;
	m_Matrix(0,1) = 2;
	m_Matrix(1,0) = 2;
	m_Matrix(1,1) = 1;
}

void CClusterTest::BuildXMatrix(int i_rows, int i_cols)
{
	m_Matrix = MatrixXd::Random(i_rows, i_cols);
}

void CClusterTest::BuildOnesMatrix(int i_rows, int i_cols)
{
	m_Matrix = MatrixXd::Ones(i_rows, i_cols);
}

void CClusterTest::BuildIdentityMatrix(int i_rows, int i_cols)
{
	m_Matrix = MatrixXd::Identity(i_rows, i_cols);
}

void CClusterTest::EigenValues()
{
	//int a = min(1,2);
	//(*m_Vector) = (Vector2d)veci.cast();

	m_Vector = m_Matrix.eigenvalues();//m_Matrix->eigenvalues();
}

void CClusterTest::EigenVectorsAndPrint()
{
	EigenSolver<MatrixXd> es(m_Matrix);
	//MatrixXd D = es.pseudoEigenvalueMatrix();
	//MatrixXd V = es.pseudoEigenvectors();

	//_cprintf("Pseudo Eigenvalues Matrix:\n");
	//Print(D);

	//_cprintf("Pseudo Eigenvectors:\n");
	//Print(V);

	MatrixXcd R = es.eigenvectors();
	_cprintf("Eigenvector:\n");
	Print(R);
}

void CClusterTest::PowerMatrixAndPrint(double i_dPower)
{
	MatrixXd MatPower = m_Matrix.array().pow(i_dPower);
	PrintPowerMatrix(MatPower, i_dPower);
}

template< class T >
void CClusterTest::Print(T& m)
{
	cout << m << endl << endl;

	try
	{
		ofstream outfile;
		outfile.open("o:\\Output\\out.txt");//, ios::app);
		outfile << m << endl << endl;
		outfile.close();
	}
	catch(...)
	{
		TRACE( "Writing to output file - catched exeption!");
		_ASSERT(nullptr);
	}
}

void CClusterTest::PrintPowerMatrix(MatrixXd& i_Mat, double i_dPower)
{
	_cprintf("Power Matrix: (%f) \n", i_dPower);
	Print(i_Mat);
}

void CClusterTest::PrintMatrix()
{
	_cprintf("Matrix:\n");
	Print(m_Matrix);
}

void CClusterTest::PrintEigenvalues()
{
	_cprintf("Eigenvalues:\n");
	Print(m_Vector);
}
