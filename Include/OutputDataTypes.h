#pragma once
#include <map>
#include <vector>

using namespace std;

class CDrugProtocol;
typedef map<int, CDrugProtocol> MapIntDrugProtocol;
typedef vector<float> FloatVector;

#define DRUG1	36
#define DRUG2	21

#define PARAM1	9000
#define PARAM2	9001
#define PARAM3	9002
#define PARAM4	2000
#define PARAM5	2503
#define PARAM6	9011
#define PARAM7	5000
#define PARAM10	7013
#define PARAM11	1287
#define PARAM12	18049
#define PARAM13	83585

#define NO_VALUE	-1.0f

class CTimeSeries
{
public:

	// <summary> Sets the internal map of different time series (experimental, simulation results,...) </summary>
	// <param name='mapIDVectorTimeSeries'> Maps Id to a Vector of time series</param>
	// <returns> Status (OK/FAIL) </returns>
	void SetTimeSeries( const map<int, FloatVector>& mapIDVectorTimeSeries )
	{
		map<int, FloatVector>::iterator	iterCurrent = m_arr.begin(), iterEnd = m_arr.end();
		
		// First clear current data
		if ( !m_arr.empty() )
		{
			for ( ;	iterCurrent != iterEnd; ++iterCurrent )
			{
				if ( !iterCurrent->second.empty() )
				{
					iterCurrent->second.resize( 0 );
				}
			}

			m_arr.clear();
		}

		// Next, set new data
		m_arr = mapIDVectorTimeSeries;

	}

	void SetTimeSeries( UINT uID, const FloatVector& fVector )
	{
		m_arr[uID] = fVector;
	}

	// Get reference to constant map of time series
	// Map Id x FloatVector of time series
	const map<int, FloatVector>& GetMatrix() const
	{
		return	m_arr;
	}

	int GetSize() noexcept
	{
		return (int)m_arr.size();
	}

	int GetSize() const noexcept
	{
		return (int)m_arr.size();
	}

	// DO NOT USE !!!!
	void ResetValues()
	{
		map<int, FloatVector >::iterator mapIter = m_arr.begin(), mapIterEnd = m_arr.end();
		for ( ; mapIter != mapIterEnd ; ++mapIter )
		{
			(mapIter->second).resize(1, 0.0f);
		}
	}

private:

	//We use map of the structure (Id x Vector of time series ):
	// Id - id of time series,
	// Vector of time series - vector of values
	map<int, FloatVector >	m_arr;
};


class CDrugProtocol
{
public:
	void SetGeneratedSet(const CTimeSeries& i_GeneratedSetOfParams)
	{
		m_GeneratedParams = i_GeneratedSetOfParams;
	}

	void SetGeneratedSet(const map< int, FloatVector >& i_mapGeneratedSetOfParams)
	{
		m_GeneratedParams.SetTimeSeries(i_mapGeneratedSetOfParams);
	}

	const CTimeSeries& GetGeneratedParames() const noexcept
	{
		return m_GeneratedParams;
	}

	CTimeSeries& GetGeneratedParames() noexcept
	{
		return m_GeneratedParams;
	}

	bool IsGeneratedParamsEmpty() const noexcept
	{
		return ( m_GeneratedParams.GetSize() == 0 ) ;
	}

	void ResetGeneratedParams()
	{
		m_GeneratedParams.ResetValues();
	}

	// DO NOT USE !!!!
	void ResetValues()
	{
		m_GeneratedParams.ResetValues();
	}

protected:
	// The generated set of parameters which defines the protocol - 
	// relevant only for Treatment Optimization
	CTimeSeries m_GeneratedParams;
};

class CCompositeProtocol
{
public:

	void InsertDrugProtocol(int i_iID,const CDrugProtocol& i_DrugProtocol)
	{
		m_DrugProtMap[i_iID] = i_DrugProtocol;
	}

	//remove all existing protocols
	void ClearAllDrugProtocols() noexcept
	{
		m_DrugProtMap.clear();
	}

	void ResetGeneratedParams()
	{
		ResetValues();
	}

	// DO NOT USE !!!!
	void ResetValues()
	{
		MapIntDrugProtocol::iterator mapIter = m_DrugProtMap.begin(), mapIterEnd = m_DrugProtMap.end();

		for ( ; mapIter != mapIterEnd ; ++mapIter)
		{
			(mapIter->second).ResetValues();
		}
	}

	//returns number of existing protocols
	int GetProtocolsCount() const noexcept
	{
		return (int)m_DrugProtMap.size();
	}

	//return CDrugProtocol according to supplied ProtocolID
	const CDrugProtocol* GetDrugProtocol(int i_iID)
	{
		if (m_DrugProtMap.end() == m_DrugProtMap.find(i_iID))
		{
			return nullptr;
		}

		return  &m_DrugProtMap[i_iID];
	}

	//return CDrugProtocol according to supplied ProtocolID
	CDrugProtocol* GetDrugProtocol2(int i_iID)
	{
		if (m_DrugProtMap.end() == m_DrugProtMap.find(i_iID))
		{
			return nullptr;
		}

		return  &m_DrugProtMap[i_iID];
	}

	//returns Protocols map
	const MapIntDrugProtocol* GetDrugProtocolsMap() const
	{
		return &m_DrugProtMap;
	}

protected:
	MapIntDrugProtocol m_DrugProtMap;
};

struct _ParameterStruct
{
	_ParameterStruct() noexcept :
		m_uID(0),
		m_fValue(-1.0f)
	{}

	_ParameterStruct(UINT i_uID, float i_fVal) noexcept :
		m_uID(i_uID),
		m_fValue(i_fVal)
	{}

	
	UINT	m_uID;
	float	m_fValue;
}; 

class CParameterSet
{
public:

	void Clear()
	{
		m_vecOptimizedParameterStructs.clear();
		m_CompositeProtocol.ClearAllDrugProtocols();
	}

	// vector (set) of optimized parameters for parameters optimization
	vector<_ParameterStruct>	m_vecOptimizedParameterStructs;

	// class of optimized treatment schedule (protocol) that is generated in treatment optimization
	CCompositeProtocol	m_CompositeProtocol;
};

