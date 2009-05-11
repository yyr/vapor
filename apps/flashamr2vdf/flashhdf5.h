/*  Wrappers for FLASH HDF Files 
*/
#ifndef __FLASH_HDF__
#define __FLASH_HDF__


#include <vector>
#include <cmath>
#include <cstdio>
#include <cstring>

// HDF Header Files
#include <hdf5.h>


using namespace std;

void HDF_ERROR_CHECK(int errorflag, char *message);

typedef char FlashVariableNames[5];

class FlashHDFFile
{
 public:
    // Default Constructor
    FlashHDFFile();
    // Constructor that opens a given HDF file
    FlashHDFFile(char *filename); 

    // Open given HDF File
    int Open(const char *filename);
    // Close currently opened HDF File
    int Close();

    // Get the number of dimensions in the current dataset {i.e. 1, 2, 3}
    int GetNumberOfDimensions() const;

    // Get the number of blocks in the current dataset
    int GetNumberOfBlocks() const {return(_sim_params.total_blocks); };

    // Get the number of leaf blocks in the current dataset
    int GetNumberOfLeafs() const {return(_numberOfLeafs); };
    
    // Get the number of cells per block
    void GetCellDimensions(int dimensions[3]) const;

	// Get dimesion of base grid in blocks
    void GetBaseDimensions(int dimensions[3]) const;

	// Return the user time for the current file
	//
	float GetUserTime() const { return (_sim_params.time); };

	int GetMaxRefineParam() { return (_int_run_params.lrefine_max); };

	// Get the user extents for the entire grid
	void GetExtents(double extents[6]) const {
		extents[0] = _real_run_params.min[0]; 
		extents[1] = _real_run_params.min[1]; 
		extents[2] = _real_run_params.min[2];
		extents[3] = _real_run_params.max[0]; 
		extents[4] = _real_run_params.max[1]; 
		extents[5] = _real_run_params.max[2];
	}

    int GetNodeType(int index);

    int GetRefineLevels(int refine_levels[], size_t n);
    
    int Get3dMinimumBounds(int index, double mbbox[3]);
    int Get3dMaximumBounds(int index, double mbbox[3]);
    
    // Spatial extent of dataset (Boundaries)
    int GetCoordinateRangeEntireDataset(double ranges[6]);

    // Spatial location of a given block (index)
    int Get3dCoordinate(int index, double coords[3]);

    // Size of a given block (index)
    int Get3dBlockSize(int index, double coords[3]);

    int GetNumberOfVariables() const { return(_variableNames.size()); };
    void GetVariableNames(vector <string> &varNames) const {
		varNames = _variableNames;
	};
    
    // variableIndex is the number (1-23) chooses variable, index is block number
    // Vector data is returned in an interleaved format (x,y,z,x,y,z) and 
    // the the dimensions are automatically generated based on the 
    // dimensionallity of the dataset
//    int GetVectorVariable(int variableIndex, int index, double *variable);

    // Get scalar data -> variable name, index to block, 
	// pointer variable, runlength is for grabbing multiple blocks 
	//
    int GetScalarVariable(
		string variableName, int index, int runlength, float *variable
	);
    int GetScalarVariable(
		string variableName, int index, int runlength, double *variable
	);

	
    

    int GetRefinementLevel(int index);

    int GetGlobalIds(int index,int globalIds[]);
    int GetGlobalIds(int globalIds[], size_t n);

    int GetBoundingBoxes(float boxes[], size_t n);

    int GetNumberOfGlobalIds();

 private:

	typedef struct {
		int total_blocks;
		int nsteps;
		int nxb;
		int nyb;
		int nzb;
		double time;
		double timestep;
	} sim_params_t;
	sim_params_t _sim_params;

	typedef struct {
		double min[3];
		double max[3];
	} real_run_params_t;

	real_run_params_t _real_run_params;

	typedef struct {
		int nblock[3];
		int lrefine_max;
		int lrefine_min;
	} int_run_params_t;

	int_run_params_t _int_run_params;

	typedef struct {
		string geometry;
		string xl_boundary_type;
		string xr_boundary_type;
		string yl_boundary_type;
		string yr_boundary_type;
		string zl_boundary_type;
		string zr_boundary_type;
	} string_run_params_t;

	string_run_params_t _string_run_params;

	vector <string> _variableNames;

    hid_t _datasetId;  // Pointer to opened HDF5 File

    int _numberOfLeafs;

	int _flashVersion;

	int _GetSimParams();
	int _GetSimParams2();
	int _GetSimParams3();
	int _GetRealRunParams();
	int _GetIntRunParams();
	int _GetStringRunParams();

	int _GetNumberOfLeafs();
	int _GetVariableNames();
    void _ResetSettings();

	int _GetFlashVersion();

	bool _member_present(const hid_t container_id, const string &name);

	int _member_index(const hid_t container_id, const ::string & name);

	int _get_num_members(const hid_t container_id);

    
};

#endif
