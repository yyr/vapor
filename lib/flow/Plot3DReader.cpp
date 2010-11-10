#include "CPlot3DReader.h"

//////////////////////////////////////////////////////////////////////////
// used for static data
//////////////////////////////////////////////////////////////////////////
CPlot3DReader::CPlot3DReader(const char *slnFN, const char *nodeFN, const char*tetraFN)
{
	m_strSlnFN = new char[_MAX_PATH];
	m_strNodeFN = new char[_MAX_PATH];
	m_strTetraFN = new char[_MAX_PATH];

	strcpy(m_strSlnFN, slnFN);
	strcpy(m_strNodeFN, nodeFN);
	strcpy(m_strTetraFN, tetraFN);
	m_nTimeSteps = 1;
	m_nFromTime = 0;
	m_nToTime = 0;
	m_nTimevarying = 0;
}

//////////////////////////////////////////////////////////////////////////
// format
// flag for timevarying, fromTime, toTime, grid filename, solution file prefix
//////////////////////////////////////////////////////////////////////////
CPlot3DReader::CPlot3DReader(const char *fn)
{
	FILE *fIn;

	// input file
	fIn = NULL;
	fIn = fopen(fn, "r");
	assert(fIn != NULL);
	fscanf(fIn, "%d", &m_nTimevarying);
	fscanf(fIn, "%d", &m_nFromTime);
	fscanf(fIn, "%d", &m_nToTime);
	fscanf(fIn, "%s", m_strSlnFN);
	fscanf(fIn, "%s", m_strNodeFN);
	fscanf(fIn, "%s", m_strTetraFN);
	fclose(fIn);

	m_nTimeSteps = m_nToTime - m_nFromTime + 1;
}

CPlot3DReader::~CPlot3DReader()
{
	delete [] m_strSlnFN;
	delete [] m_strNodeFN;
	delete [] m_strTetraFN;
}

//////////////////////////////////////////////////////////////////////////
// read time-varying or static solution
//////////////////////////////////////////////////////////////////////////
Solution* CPlot3DReader::CreateSolution(void)
{
	Solution* pSolution;
	Vector** pData;
	int nodeNum;

	pData = ReadSolution(nodeNum);
	pSolution = new Solution(pData, nodeNum, m_nTimeSteps);
	return pSolution;
}

Vector** CPlot3DReader::ReadSolution(int& nodeNum)
{
	FILE *fIn;
	int iFor, xdim, ydim, zdim, numToRead;
	float refval[4];
	float *pDensity, *pMomentumU, *pMomentumV, *pMomentumW;
	char *slFn = new char[_MAX_PATH];
	Vector** pData;
	Vector* pDataOneStep;

	try
	{
		pData = new Vector*[m_nTimeSteps];
		if(m_nTimevarying == 1)					// time-varying dataset
		{
			// get the data size
			sprintf(slFn, "%s%d", m_strSlnFN, 0); 
			fIn = fopen(slFn, "rb");
			fread(&xdim, sizeof(int), 1, fIn);
			fread(&ydim, sizeof(int), 1, fIn);
			fread(&zdim, sizeof(int), 1, fIn);
			fread(refval, sizeof(float), 4, fIn);
			numToRead = xdim;
			nodeNum = numToRead;
			fclose(fIn);

			pDensity = new float[numToRead];
			pMomentumU = new float[numToRead];
			pMomentumV = new float[numToRead];
			pMomentumW = new float[numToRead];

			// read data
			for(iFor = m_nFromTime; iFor <= m_nToTime; iFor++)
			{
				// allocate memory
				pDataOneStep = new Vector[numToRead];

				sprintf(slFn, "%s%d", m_strSlnFN, iFor); 
				fIn = fopen(slFn, "rb");

				fread(&xdim, sizeof(int), 1, fIn);
				fread(&ydim, sizeof(int), 1, fIn);
				fread(&zdim, sizeof(int), 1, fIn);
				fread(refval, sizeof(float), 4, fIn);
				numToRead = xdim * ydim * zdim;
				
				// density, momentum
				fread(pDensity, sizeof(float), numToRead, fIn);
				fread(pMomentumU, sizeof(float), numToRead, fIn);
				fread(pMomentumV, sizeof(float), numToRead, fIn);
				fread(pMomentumW, sizeof(float), numToRead, fIn);

				fclose(fIn);
				printf("Load in flow data!\n");

				GetDataFromSln(pDataOneStep, pDensity, pMomentumU, pMomentumV, pMomentumW, numToRead);
				pData[iFor-m_nFromTime] = pDataOneStep;
			}

			delete[] pDensity;
			delete[] pMomentumU;
			delete[] pMomentumV;
			delete[] pMomentumW;
		}
		else										// static
		{
			// get the data size
			fIn = fopen(m_strSlnFN, "rb");
			fread(&xdim, sizeof(int), 1, fIn);
			fread(&ydim, sizeof(int), 1, fIn);
			fread(&zdim, sizeof(int), 1, fIn);
			fread(refval, sizeof(float), 4, fIn);
			numToRead = xdim;
			nodeNum = numToRead;

			// allocate memory
			pDataOneStep = new Vector[numToRead];
			pDensity = new float[numToRead];
			pMomentumU = new float[numToRead];
			pMomentumV = new float[numToRead];
			pMomentumW = new float[numToRead];

			// density, momentum
			fread(pDensity, sizeof(float), numToRead, fIn);
			fread(pMomentumU, sizeof(float), numToRead, fIn);
			fread(pMomentumV, sizeof(float), numToRead, fIn);
			fread(pMomentumW, sizeof(float), numToRead, fIn);

			fclose(fIn);
			printf("Load in flow data!\n");

			GetDataFromSln(pDataOneStep, pDensity, pMomentumU, pMomentumV, pMomentumW, numToRead);
			pData[0] = pDataOneStep;

			delete[] pDensity;
			delete[] pMomentumU;
			delete[] pMomentumV;
			delete[] pMomentumW;
		}
	}
	catch(...)
	{
		printf("Exceptions happen in function CPlot3DReader::ReadSolution()!\n");
	}
	return pData;
}

//////////////////////////////////////////////////////////////////////////
// compute vector field from solution
//////////////////////////////////////////////////////////////////////////
void CPlot3DReader::GetDataFromSln( Vector* pData, 
									const float* pDensity, 
									const float* pMomentumU,
									const float* pMomentumV,
									const float* pMomentumW,
									const int size)
{
	int iFor;

	assert(pData != NULL);
	try
	{
		for(iFor = 0; iFor < size; iFor++)
		{
			if(pDensity[iFor] == 0.0)
				pData[iFor].Set(0.0, 0.0, 0.0);
			else
				pData[iFor].Set(pMomentumU[iFor]/pDensity[iFor],
								pMomentumV[iFor]/pDensity[iFor],
								pMomentumW[iFor]/pDensity[iFor]);
		}
	}
	catch(...)
	{
		printf("Exceptions happen in function CPlot3DReader::GetDataFromSln()!\n");
	}
}

void CPlot3DReader::ReadCurvilinearGrid(void)
{
}

//////////////////////////////////////////////////////////////////////////
// input
// bVerTopoOn: whether to keep vertex topology information
// bTetraTopoOn: whether to keep tetra topology information
//////////////////////////////////////////////////////////////////////////
IrregularGrid* CPlot3DReader::CreateIrregularGrid(bool bVerTopoOn, bool bTetraTopoOn)
{
	int nodeNum = 0, tetraNum = 0;
	CVertex* pVertexGeom = NULL;
	CTetra* pTetra = NULL;
	TVertex* pVertexTopo = NULL;
	IrregularGrid* pGrid = NULL;

	GetSize(nodeNum, tetraNum);
	pVertexGeom = new CVertex[nodeNum];
	pTetra = new CTetra[tetraNum];
	if(bVerTopoOn) pVertexTopo = new TVertex[nodeNum];

	ReadIrregularGrid(bVerTopoOn, bTetraTopoOn, nodeNum, tetraNum, pVertexGeom, pTetra, pVertexTopo);
	pGrid = new IrregularGrid(nodeNum, tetraNum, pVertexGeom, pTetra, pVertexTopo);
	return pGrid;
}

void CPlot3DReader::GetSize(int& nodeNum, int& tetraNum)
{
	FILE *fIn;
	int surfTriNum;
	
	fIn = fopen(m_strNodeFN, "rb");
	fread(&nodeNum, sizeof(int), 1, fIn);
	fread(&surfTriNum, sizeof(int), 1, fIn);
	fread(&tetraNum, sizeof(int), 1, fIn);
	fclose(fIn);

	m_nNodeNum = nodeNum;
	m_nTetraNum = tetraNum;
}

void CPlot3DReader::ReadIrregularGrid(bool bVerTopoOn,
									  bool bTetraTopoOn,
									  int nodeNum,
									  int tetraNum,
									  CVertex* pVertexGeom,
									  CTetra* pTetra,
									  TVertex* pVertexTopo)
{
	FILE *fIn;
	float *fTemp;
	int *iTemp;
	int iFor;
	CVertex node;
	int surfTriNum;

	try
	{
		// node geometry info
		fIn = fopen(m_strNodeFN, "rb");
		fread(&nodeNum, sizeof(int), 1, fIn);
		fread(&surfTriNum, sizeof(int), 1, fIn);
		fread(&tetraNum, sizeof(int), 1, fIn);

		fTemp = (float *)malloc(sizeof(float) * nodeNum);
		fread(fTemp, sizeof(float), nodeNum, fIn);					// X for nodes
		for(iFor = 0; iFor < nodeNum; iFor++)
			pVertexGeom[iFor].position[0] = fTemp[iFor];
		fread(fTemp, sizeof(float), nodeNum, fIn);					// Y for nodes
		for(iFor = 0; iFor < nodeNum; iFor++)
			pVertexGeom[iFor].position[1] = fTemp[iFor];
		fread(fTemp, sizeof(float), nodeNum, fIn);					// Z for nodes
		for(iFor = 0; iFor < nodeNum; iFor++)
			pVertexGeom[iFor].position[2] = fTemp[iFor];
		free(fTemp);
		fclose(fIn);
		fIn = NULL;
		printf("Load in node information!\n");

		// topology info for tetrahedral volume grid
		fIn = fopen(m_strTetraFN, "rb");
		fread(&nodeNum, sizeof(int), 1, fIn);
		fread(&surfTriNum, sizeof(int), 1, fIn);
		fread(&tetraNum, sizeof(int), 1, fIn);
		iTemp = new int[tetraNum * 4];
		fread(iTemp, sizeof(int), tetraNum * 4, fIn);
		// construct tetra and vertex topo information
		ConstructTetraVolume(pTetra,pVertexTopo, nodeNum, tetraNum, iTemp, bVerTopoOn, bTetraTopoOn);
		delete[] iTemp;
		fclose(fIn);
		fIn = NULL;
		printf("Construct unstructured volume grid!\n");
	}
	catch(...)
	{
		printf("Exceptions happen in function FGrid::ReadData()!\n");
	}
}
