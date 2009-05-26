// General Headers
#include <iostream>
#include <cstdlib>
#include <climits>

// Class Headers
#include <flashhdf5.h>

using namespace std;

void
FlashHDFFile::_ResetSettings()
{

	_sim_params.total_blocks = -1;
	_sim_params.nsteps = -1;
	_sim_params.nxb = -1;
	_sim_params.nyb = -1;
	_sim_params.nzb = -1;
	_sim_params.time = -1.0;
	_sim_params.timestep = -1.0;

	for (int i=0; i<3; i++) _real_run_params.min[i] = -1.0;
	for (int i=0; i<3; i++) _real_run_params.max[i] = -1.0;

	for (int i=0; i<3; i++) _int_run_params.nblock[i] = -1;
	_int_run_params.lrefine_max = -1;
	_int_run_params.lrefine_min = -1;

	_string_run_params.geometry.clear();
	_string_run_params.xl_boundary_type.clear();
	_string_run_params.xr_boundary_type.clear();
	_string_run_params.yl_boundary_type.clear();
	_string_run_params.yr_boundary_type.clear();
	_string_run_params.zl_boundary_type.clear();
	_string_run_params.zr_boundary_type.clear();

	_flashVersion = -1;
    _numberOfLeafs = -1;
	_datasetId = -1;
	_variableNames.clear();
    
}// End of _ResetSettings()

// FlashHDFFile Public Class Functions

FlashHDFFile::FlashHDFFile()
{

    _ResetSettings();
}// End of FlashHDFFile()

int
FlashHDFFile::Open(const char *filename)
{
    _ResetSettings();
    _datasetId = H5Fopen(filename,H5F_ACC_RDONLY,H5P_DEFAULT);
	if (_datasetId < 0) return(-1);

	if (_GetFlashVersion() < 0) return(-1);

	if (_GetSimParams() < 0) return(-1);
	if (_GetRealRunParams() < 0) return(-1);
	if (_GetIntRunParams() < 0) return(-1);
	if (_GetStringRunParams() < 0) return(-1);

	if (_GetNumberOfLeafs() < 0) return(-1);
	if (_GetVariableNames() < 0) return(-1);

    return 0;
    
}// End of Open(char *filename)

int
FlashHDFFile::Close()
{
    if (_datasetId >= 0) H5Fclose(_datasetId);
    _ResetSettings();
    return 0;
}

int FlashHDFFile::GetNumberOfDimensions()
const {
	int n = 0;
	if  (_sim_params.nxb > 0) n++;
	if  (_sim_params.nyb > 0) n++;
	if  (_sim_params.nzb > 0) n++;
	return(n);
}
		

#define LEAF_NODE 1

int
FlashHDFFile::GetNumberOfGlobalIds()
{
	int n = GetNumberOfDimensions();
	return ((2*n) + (int)powf(2.0, (double)n) + 1);
}

int
FlashHDFFile::GetGlobalIds(int idx,int gids[])
{
    hid_t dataspace, dataset, memspace;
    int rank;
    hsize_t dimens_1d, dimens_2d[2];
	herr_t ierr;
    
    hsize_t start_2d[2];
    hsize_t stride_2d[2], count_2d[2];
    
    
    int i;
    for(i=0;i<15;i++) gids[i] = -2;

    rank = 2;
    dimens_2d[0] = GetNumberOfBlocks();
    dimens_2d[1] = 15; // WRONG Hardcoded value;
    
    /* define the dataspace -- as described above */
    start_2d[0]  = (hssize_t)idx;
    start_2d[1]  = 0;
    
    stride_2d[0] = 1;
    stride_2d[1] = 1;
    
    count_2d[0]  = 1;
    count_2d[1]  = 15; // WRONG Hardcoded value;
    
    
    dataspace = H5Screate_simple(rank, dimens_2d, NULL);
	if (dataspace < 0) return(-1);

    ierr = H5Sselect_hyperslab(dataspace, H5S_SELECT_SET,
			       start_2d, stride_2d, count_2d, NULL);
	if (ierr < 0) return(-1);

    /* define the memory space */
    rank = 1;
    dimens_1d = 15; // WRONG Hardcoded value;
    memspace = H5Screate_simple(rank, &dimens_1d, NULL);
	if (memspace < 0) return(-1);
    
    dataset = H5Dopen(_datasetId, "gid");
	if (dataset < 0) return(-1);
    herr_t status  = H5Dread(dataset, H5T_NATIVE_INT, memspace, dataspace, 
		      H5P_DEFAULT, gids);
    

    H5Sclose(memspace);
    H5Sclose(dataspace);
    H5Dclose(dataset);

	if (status < 0) return(-1);

    return 0;
}

int
FlashHDFFile::GetGlobalIds(int gids[], size_t n)
{
    hid_t dataspace, dataset, memspace;
    int rank;
    hsize_t dimens_1d, dimens_2d[2];
	herr_t ierr;
    
    hsize_t start_2d[2];
    hsize_t stride_2d[2], count_2d[2];
    
    
    rank = 2;
    dimens_2d[0] = GetNumberOfBlocks();
    dimens_2d[1] = 15; // WRONG Hardcoded value;
    
    /* define the dataspace -- as described above */
    start_2d[0]  = 0;
    start_2d[1]  = 0;
    
    stride_2d[0] = 1;
    stride_2d[1] = 1;
    
    count_2d[0]  = n;
    count_2d[1]  = 15; // WRONG Hardcoded value;
    
    
    dataspace = H5Screate_simple(rank, dimens_2d, NULL);
	if (dataspace < 0) return(-1);

    ierr = H5Sselect_hyperslab(dataspace, H5S_SELECT_SET,
			       start_2d, stride_2d, count_2d, NULL);
	if (ierr < 0) return(-1);

    /* define the memory space */
    rank = 1;
    dimens_1d = 15 * GetNumberOfBlocks(); // WRONG Hardcoded value;
    memspace = H5Screate_simple(rank, &dimens_1d, NULL);
	if (memspace < 0) return(-1);
    
    dataset = H5Dopen(_datasetId, "gid");
	if (dataset < 0) return(-1);
    herr_t status  = H5Dread(dataset, H5T_NATIVE_INT, memspace, dataspace, 
		      H5P_DEFAULT, gids);
    


    H5Sclose(memspace);
    H5Sclose(dataspace);
    H5Dclose(dataset);

	if (status < 0) return(-1);

    return 0;
}


int
FlashHDFFile::GetBoundingBoxes(float bboxes[], size_t n)
{
    hid_t dataspace, dataset, memspace;
    int rank;
    hsize_t dimens_1d, dimens_3d[3];
    
    hsize_t start_3d[3];
    hsize_t stride_3d[3], count_3d[3];
    
    rank = 3;
    dimens_3d[0] = GetNumberOfBlocks();
    dimens_3d[1] = 3; // WRONG Hardcoded value;
    dimens_3d[2] = 2; // WRONG Hardcoded value;
    
    /* define the dataspace -- as described above */
    start_3d[0]  = 0;
    start_3d[1]  = 0;
    start_3d[2]  = 0;
    
    stride_3d[0] = 1;
    stride_3d[1] = 1;
    stride_3d[2] = 1;
    
    count_3d[0]  = n;
    count_3d[1]  = 3; // WRONG Hardcoded value;
    count_3d[2]  = 2; // WRONG Hardcoded value;
    
    dataspace = H5Screate_simple(rank, dimens_3d, NULL);
	if (dataspace < 0) return(-1);

    herr_t ierr = H5Sselect_hyperslab(dataspace, H5S_SELECT_SET,
			       start_3d, stride_3d, count_3d, NULL);
	if (ierr < 0) return(-1);

    /* define the memory space */
    rank = 1;
    dimens_1d = 6 * GetNumberOfBlocks(); // WRONG Hardcoded value;
    memspace = H5Screate_simple(rank, &dimens_1d, NULL);
	if (memspace < 0) return(-1);
    
    dataset = H5Dopen(_datasetId, "bounding box");
	if (dataset < 0) return(-1);
    herr_t status  = H5Dread(dataset, H5T_NATIVE_FLOAT, memspace, dataspace, 
		      H5P_DEFAULT, bboxes);
    


    H5Sclose(memspace);
    H5Sclose(dataspace);
    H5Dclose(dataset);

	if (status < 0) return(-1);

    return 0;
}


// Gets the dimensions of an individual block, for viz its usually 9x9x9
//
void
FlashHDFFile::GetCellDimensions(int dims[3]) const
{

	dims[0] = _sim_params.nxb;
	dims[1] = _sim_params.nyb;
	dims[2] = _sim_params.nzb;

	for (int i = 0; i<3; i++) if (dims[0] < 1) dims[i] = 1;

}

void
FlashHDFFile::GetBaseDimensions(int dims[3]) const
{
	for (int i = 0; i<3; i++) {
		dims[i] = _int_run_params.nblock[i];
		if (dims[0] < 1) dims[i] = 1;
	}

}


int
FlashHDFFile::GetNodeType(int idx)
{
    int ntype;
    
    int rank;
    hsize_t dimens_1d;
    hid_t ierr;
    
    hid_t dataspace, memspace, dataset;
    
    hsize_t start_1d;
    
    hsize_t stride_1d, count_1d;
    
    rank = 1;
    dimens_1d = GetNumberOfBlocks();

    /* define the dataspace -- same as above */
    start_1d  = (hssize_t)idx;
    stride_1d = 1;
    count_1d  = 1;
    
    dataspace = H5Screate_simple(rank, &dimens_1d, NULL);
	if (dataspace <  0) return(-1);

    ierr = H5Sselect_hyperslab(
		dataspace, H5S_SELECT_SET,
		&start_1d, &stride_1d, &count_1d, NULL
	);
	if (ierr <  0) return(-1);
    
    /* define the memory space */
    dimens_1d = 1;
    memspace = H5Screate_simple(rank, &dimens_1d, NULL);
    
    /* create the dataset from scratch only if this is our first time */
    dataset = H5Dopen(_datasetId, "node type");
	if (dataset <  0) return(-1);
    herr_t status  = H5Dread(dataset, H5T_NATIVE_INT, memspace, dataspace, 
		      H5P_DEFAULT, (void *)&ntype);
  
	if (status < 0) return(-1);
    H5Sclose(memspace);
    H5Sclose(dataspace);
    H5Dclose(dataset);
    
    return ntype;
    
}

int
FlashHDFFile::GetRefineLevels(int refine_levels[], size_t n)
{
    
    int rank;
    hsize_t dimens_1d;
    hid_t ierr;
    
    hid_t dataspace, memspace, dataset;
    
    hsize_t start_1d;
    
    hsize_t stride_1d, count_1d;
    
    rank = 1;
    dimens_1d = GetNumberOfBlocks();

    /* define the dataspace -- same as above */
    start_1d  = 0;
    stride_1d = 1;
    count_1d  = n;
    
    dataspace = H5Screate_simple(rank, &dimens_1d, NULL);
	if (dataspace < 0) return(-1);

    ierr = H5Sselect_hyperslab(dataspace, H5S_SELECT_SET,
			     &start_1d, &stride_1d, &count_1d, NULL);
	if (ierr < 0) return(-1);
    
    /* define the memory space */
    dimens_1d = GetNumberOfBlocks();
    memspace = H5Screate_simple(rank, &dimens_1d, NULL);
	if (memspace < 0) return(-1);
    
    /* create the dataset from scratch only if this is our first time */
    dataset = H5Dopen(_datasetId, "refine level");
	if (dataset < 0) return(-1);
    herr_t status  = H5Dread(dataset, H5T_NATIVE_INT, memspace, dataspace, 
		      H5P_DEFAULT, (void *)refine_levels);
  
    H5Sclose(memspace);
    H5Sclose(dataspace);
    H5Dclose(dataset);
    
	if (status < 0) return(-1);
    return 0;
    
}

int
FlashHDFFile::GetRefinementLevel(int idx)
{
    int refinelevel;
    
    int rank;
    hsize_t dimens_1d;
    hid_t ierr;
    
    hid_t dataspace, memspace, dataset;
    
    hsize_t start_1d;
    hsize_t stride_1d, count_1d;
    
    rank = 1;
    dimens_1d = GetNumberOfBlocks();

    /* define the dataspace -- same as above */
    start_1d  = (hssize_t)idx;
    stride_1d = 1;
    count_1d  = 1;
    
    dataspace = H5Screate_simple(rank, &dimens_1d, NULL);
	if (dataspace < 0) return(-1);

    ierr = H5Sselect_hyperslab(dataspace, H5S_SELECT_SET,
			     &start_1d, &stride_1d, &count_1d, NULL);
	if (ierr < 0) return(-1);
    
    /* define the memory space */
    dimens_1d = 1;
    memspace = H5Screate_simple(rank, &dimens_1d, NULL);
	if (memspace < 0) return(-1);
    
    /* create the dataset from scratch only if this is our first time */
    dataset = H5Dopen(_datasetId, "refine level");
	if (dataset < 0) return(-1);
    herr_t status  = H5Dread(dataset, H5T_NATIVE_INT, memspace, dataspace, 
		      H5P_DEFAULT, (void *)&refinelevel);
  
    H5Sclose(memspace);
    H5Sclose(dataspace);
    H5Dclose(dataset);

	if (status < 0) return(-1);
    
    return refinelevel;
}

#define nguard (0)// 4

int
FlashHDFFile::GetScalarVariable(
	string variableName, int dataPointIndex, int sizeofrun, float *variable
) {
	if (variableName.length() > 4) {
		return(-1);
	}

	// Flash vars have to be exactly 4 characters long, padded with
	// spaces if necessary. 
	//
	char flash_var_name[5];
	strcpy(flash_var_name, variableName.c_str());
	for (int i=variableName.length(); i<4; i++) flash_var_name[i] = ' ';
	flash_var_name[4] = '\0';
 
    int rank;
    hsize_t dimens_4d[5];
    hid_t ierr;
    
    hid_t dataspace, memspace, dataset;
    
    hsize_t start_4d[5];
    hsize_t stride_4d[5], count_4d[5];
    

	int cellDimensions[3];
	GetCellDimensions(cellDimensions);
    
    rank = 4;
    dimens_4d[0] = GetNumberOfBlocks();    
    dimens_4d[1] = cellDimensions[2]; 
    dimens_4d[2] = cellDimensions[1];
    dimens_4d[3] = cellDimensions[0];

  /* define the dataspace -- as described above */
    start_4d[0]  = (hssize_t)dataPointIndex;
    start_4d[1]  = 0;
    start_4d[2]  = 0;
    start_4d[3]  = 0;
    
    stride_4d[0] = 1;
    stride_4d[1] = 1;
    stride_4d[2] = 1;
    stride_4d[3] = 1;
    
    count_4d[0]  = sizeofrun;// was 1, now the number of blocks grabbed
    count_4d[1]  = cellDimensions[2];
    count_4d[2]  = cellDimensions[1];
    count_4d[3]  = cellDimensions[0];
    
    dataspace = H5Screate_simple(rank, dimens_4d, NULL);
	if (dataspace < 0) return(-1);

    ierr = H5Sselect_hyperslab(dataspace, H5S_SELECT_SET,
			       start_4d, stride_4d, count_4d, NULL);
	if (ierr < 0) return(-1);

    int k3d,k2d;
    k3d = k2d = 1;// Hard coded bad bad bad
    
    /* 
       define the memory space -- the unkt array that was passed includes all
       the variables and guardcells for a single block.  We want to cut out the
       portion that just has the interior cells and the variable specified by 
       index
    */
    rank = 5;
    dimens_4d[0] = GetNumberOfBlocks();
    dimens_4d[1] = cellDimensions[2]+(nguard)*2*k3d;
    dimens_4d[2] = cellDimensions[1]+(nguard)*2*k2d;
    dimens_4d[3] = cellDimensions[0]+(nguard)*2;
    dimens_4d[4] = 1; // Number of Variables 1

    memspace = H5Screate_simple(rank, dimens_4d, NULL);
	if (memspace < 0) return(-1);

    /* exclude the guardcells and take only the desired variable */
    start_4d[0] = 0;
    start_4d[1] = (nguard)*k3d;
    start_4d[2] = (nguard)*k2d;  
    start_4d[3] = (nguard);
    start_4d[4] = 0;//variableIndex;  /* should be 0 *//* remember: 0 based indexing */
    
    stride_4d[0] = 1;
    stride_4d[1] = 1;  
    stride_4d[2] = 1;
    stride_4d[3] = 1;
    stride_4d[4] = 1;
    
    count_4d[0] = sizeofrun;
    
    count_4d[1] = cellDimensions[2];
    count_4d[2] = cellDimensions[1]; 
    count_4d[3] = cellDimensions[0]; 
    count_4d[4] = 1; 
    
    ierr = H5Sselect_hyperslab(memspace, H5S_SELECT_SET,
			       start_4d, stride_4d, count_4d, NULL);
	if (ierr < 0) return(-1);
    
    dataset = H5Dopen(_datasetId,flash_var_name);
	if (dataset < 0) return(-1);
    herr_t status  = H5Dread(dataset, H5T_NATIVE_FLOAT, memspace, dataspace, 
		      H5P_DEFAULT, variable);
    
    H5Sclose(memspace);
    H5Sclose(dataspace);
    H5Dclose(dataset);

	if (status < 0) return(-1);

    return 0;
}

int
FlashHDFFile::Get3dMinimumBounds(int idx, double coords[3])
{
    double coord[3];
    double bound[3];
    
    if (Get3dCoordinate(idx,coord)< 0) return(-1);
    if (Get3dBlockSize(idx,bound)< 0) return(-1);

    coords[0] = coord[0] - (bound[0]/2.0);
    coords[1] = coord[1] - (bound[1]/2.0);
    coords[2] = coord[2] - (bound[2]/2.0);

	return 0;
}// End of

int
FlashHDFFile::Get3dMaximumBounds(int idx, double coords[3])
{
    double coord[3];
    double bound[3];
    
    if (Get3dCoordinate(idx,coord)< 0) return(-1);
    if (Get3dBlockSize(idx,bound)< 0) return(-1);

    coords[0] = coord[0] + (bound[0]/2.0);
    coords[1] = coord[1] + (bound[1]/2.0);
    coords[2] = coord[2] + (bound[2]/2.0);
	return 1;
}// End of

int
FlashHDFFile::GetCoordinateRangeEntireDataset(double ranges[6])
{
    int i;
    double coord[3];
    double bounds[3];
    double emin[3];
    double emax[3];
    
    for(i=0;i<GetNumberOfBlocks();i++) {
		if (Get3dCoordinate(i,coord) < 0) return(0);
		if (Get3dBlockSize(i,bounds) < 0) return(0);

		
		emin[0] = coord[0] - bounds[0]/2.0;
		emax[0] = coord[0] + bounds[0]/2.0;
		
		emin[1] = coord[1] - bounds[1]/2.0;
		emax[1] = coord[1] + bounds[1]/2.0;
		
		emin[2] = coord[2] - bounds[2]/2.0;
		emax[2] = coord[2] + bounds[2]/2.0;

		if (i == 0) {
			ranges[0] = emin[0]; ranges[2] = emin[1]; ranges[4] = emin[2];
			ranges[1] = emax[0]; ranges[3] = emax[1]; ranges[5] = emax[2];
		}
		
		if(emin[0] < ranges[0])
			ranges[0] = (double)emin[0];
		if(emin[1] < ranges[2])
			ranges[2] = (double)emin[1];
		if(emin[2] < ranges[4])
			ranges[4] = (double)emin[2];
		
		if(emax[0] > ranges[1])
			ranges[1] = (double)emax[0];
		if(emax[1] > ranges[3])
			ranges[3] = (double)emax[1];
		if(emax[2] > ranges[5])
			ranges[5] = (double)emax[2];
	}

    return 0;
      
}

int
FlashHDFFile::Get3dCoordinate(int idx, double coords[3])
{
    double coord[3];
    int rank;
    hsize_t dimens_1d, dimens_2d[2];
    hid_t ierr;
    
    hid_t dataspace, memspace, dataset;
    
    hsize_t start_2d[2];
    hsize_t stride_2d[2], count_2d[2];
    
    rank = 2;
    dimens_2d[0] = GetNumberOfBlocks();
    dimens_2d[1] = 3;//NDIM;
    
    /* define the dataspace -- as described above */
    start_2d[0]  = (hssize_t)idx;
    start_2d[1]  = 0;
    
    stride_2d[0] = 1;
    stride_2d[1] = 1;
    count_2d[0]  = 1;
    count_2d[1]  = 3;//NDIM;
    
    dataspace = H5Screate_simple(rank, dimens_2d, NULL);
	if (dataspace < 0) return(-1);
    
    ierr = H5Sselect_hyperslab(dataspace, H5S_SELECT_SET,
  			       start_2d, stride_2d, count_2d, NULL);
	if (ierr < 0) return(-1);
    
    /* define the memory space */
    rank = 1;
    dimens_1d = 3;//NDIM;
    memspace = H5Screate_simple(rank, &dimens_1d, NULL);
	if (memspace < 0) return(-1);
    
    dataset = H5Dopen(_datasetId, "coordinates");
	if (dataset < 0) return(-1);
    herr_t status  = H5Dread(dataset, H5T_NATIVE_DOUBLE, memspace, dataspace, 
  		      H5P_DEFAULT, coord);
    
    H5Sclose(memspace);
    H5Sclose(dataspace);
    H5Dclose(dataset);

	if (status < 0) return(-1);
    
    coords[0] = (double)coord[0];
    coords[1] = (double)coord[1];
    coords[2] = (double)coord[2];
    
    return 0;
    
}

int
FlashHDFFile::Get3dBlockSize(int idx, double coords[3])
{
    double size[3];
    
    int rank;
    hsize_t dimens_1d, dimens_2d[2];
    hid_t ierr;
    
    hid_t dataspace, memspace, dataset;

    hsize_t start_2d[2];
    hsize_t stride_2d[2], count_2d[2];
    
    
    rank = 2;
    dimens_2d[0] = GetNumberOfBlocks();
    dimens_2d[1] = 3;//NDIM;
    
    /* define the dataspace -- as described above */
    start_2d[0]  = (hssize_t)idx;
    start_2d[1]  = 0;
    
    stride_2d[0] = 1;
    stride_2d[1] = 1;
    
    count_2d[0]  = 1;
    count_2d[1]  = 3;//NDIM;
    
    dataspace = H5Screate_simple(rank, dimens_2d, NULL);
	if (dataspace < 0) return(-1);
    
    ierr = H5Sselect_hyperslab(dataspace, H5S_SELECT_SET,
			       start_2d, stride_2d, count_2d, NULL);
	if (ierr < 0) return(-1);
    
    /* define the memory space */
    rank = 1;
    dimens_1d = 3;//NDIM;
    memspace = H5Screate_simple(rank, &dimens_1d, NULL);
	if (memspace < 0) return(-1);
    
    dataset = H5Dopen(_datasetId, "block size");
	if (dataset < 0) return(-1);
    herr_t status  = H5Dread(dataset, H5T_NATIVE_DOUBLE, memspace, dataspace, 
  		      H5P_DEFAULT, size);
    
    H5Sclose(memspace);
    H5Sclose(dataspace);
    H5Dclose(dataset);

	if (status < 0) return(-1);
    
    coords[0] = (double)size[0];
    coords[1] = (double)size[1];
    coords[2] = (double)size[2];

	return 0;
    
}

int 
FlashHDFFile::_GetVariableNames() {

	const int varnamelen = 4;	// Hardcoded to 4?
	_variableNames.clear();
    

    /* manually set the string size */
    
    hid_t dataset = H5Dopen(_datasetId, "unknown names");
	if (dataset < 0) return(-1);
    hid_t dataspace = H5Dget_space(dataset);
	if (dataspace < 0) return(-1);

    hsize_t fields[2];
    hsize_t maxfields[2];
    
    H5Sget_simple_extent_dims (dataspace, fields, maxfields);
	int numberOfVariables = fields[0];

    char *tname = new char[varnamelen*numberOfVariables]; 
//	char buf[varnamelen+1];


	hid_t string_type = H5Dget_type(dataset);
	 
    herr_t status = H5Dread(dataset, string_type, H5S_ALL, H5S_ALL, 
                       H5P_DEFAULT, tname);
	if (status < 0) return(-1);

    for(int i=0;i<numberOfVariables;i++) {	
		//strncpy(buf,&tname[i*varnamelen],varnamelen);
		string s;
		s.assign(&tname[i*varnamelen],varnamelen);
		for (int j=0; j<varnamelen; j++) {
			if (s[j] == ' ') {
				s.erase(s.begin()+j, s.end()); 
				break;
			}
		}
		_variableNames.push_back(s);
    }
	delete [] tname;
    
    H5Sclose(dataspace);
    H5Dclose(dataset);

    return (0);
}

int FlashHDFFile::_GetSimParams() {
	if (_flashVersion == 2) {
		return(_GetSimParams2());
	}
	else if (_flashVersion == 3) {
		return(_GetSimParams3());
	}
	return(-1);
}
	

int FlashHDFFile::_GetSimParams2() {

	hid_t sp_type;
	
	hid_t dataset = H5Dopen(_datasetId, "simulation parameters");
	if (dataset < 0) return(-1);

	/* create the HDF 5 compound data type to describe the record */
	sp_type = H5Tcreate(H5T_COMPOUND, sizeof(sim_params_t));

	H5Tinsert(sp_type, 
		  "total blocks", 
		  offsetof(sim_params_t, total_blocks),
		  H5T_NATIVE_INT);
 
	H5Tinsert(sp_type,
		  "time",
		  offsetof(sim_params_t, time),
		  H5T_NATIVE_DOUBLE);
	
	H5Tinsert(sp_type,
		  "timestep",
		  offsetof(sim_params_t, timestep),
		  H5T_NATIVE_DOUBLE);
	
	H5Tinsert(sp_type,
		  "number of steps",
		  offsetof(sim_params_t, nsteps),
		  H5T_NATIVE_INT);
	
	H5Tinsert(sp_type,
		  "nxb",
		  offsetof(sim_params_t, nxb),
		  H5T_NATIVE_INT);
	
	H5Tinsert(sp_type,
		  "nyb",
		  offsetof(sim_params_t, nyb),
		  H5T_NATIVE_INT);
	
	H5Tinsert(sp_type,
		  "nzb",
		  offsetof(sim_params_t, nzb),
		  H5T_NATIVE_INT);

  
	herr_t status = H5Dread(dataset, sp_type, H5S_ALL, H5S_ALL, H5P_DEFAULT, 
			 &_sim_params);

	if (_sim_params.nxb < 0) _sim_params.nxb = 0;
	if (_sim_params.nyb < 0) _sim_params.nyb = 0;
	if (_sim_params.nzb < 0) _sim_params.nzb = 0;

	H5Tclose(sp_type);
	H5Dclose(dataset);

	return((int) status);
}

int FlashHDFFile::_GetSimParams3() {

	const int strlen = 80;

	hid_t sp_type;
	herr_t rc;

	typedef struct int_scalars_t {
	    char name[strlen];	// Ugh. Shouldn't be hard-coded
	    int value;
	} int_scalars_t;

	int_scalars_t *int_scalars = NULL;

	hid_t dataset = H5Dopen(_datasetId, "integer scalars");
	if (dataset < 0) return(-1);

    hid_t dataspace = H5Dget_space(dataset);
	if (dataspace < 0) return(-1);

    hsize_t fields[2];
    hsize_t maxfields[2];
    H5Sget_simple_extent_dims (dataspace, fields, maxfields);
	size_t nelem = fields[0];

	sp_type = H5Tcreate(H5T_COMPOUND, sizeof(int_scalars_t));

	hid_t strid = H5Tcopy(H5T_C_S1);
	H5Tset_size(strid, sizeof(int_scalars[0].name));
	H5Tinsert(sp_type, 
		  "name", 
		  offsetof(int_scalars_t, name),
		  strid
	);
 
	H5Tinsert(sp_type,
		  "value",
		  offsetof(int_scalars_t, value),
		  H5T_NATIVE_INT);


	int_scalars = (int_scalars_t *) new unsigned char[nelem * H5Tget_size(sp_type)];

	rc = H5Dread(
		dataset, sp_type, H5S_ALL, H5S_ALL, H5P_DEFAULT, int_scalars
	);

	H5Tclose(sp_type);
	H5Dclose(dataset);

	if (rc<0) return((int) rc);

	for (int i = 0; i<nelem; i++) {
		string s;
		s.assign(int_scalars[i].name, strlen);

		for (int j=0; j<strlen; j++) {
			if (s[j] == ' ') {
				s.erase(s.begin()+j, s.end()); 
				break;
			}
		}

		if (s.compare("nxb") == 0) {
			_sim_params.nxb = int_scalars[i].value;
		} else if (s.compare("nyb") == 0) {
			_sim_params.nyb = int_scalars[i].value;
		} else if (s.compare("nzb") == 0) {
			_sim_params.nzb = int_scalars[i].value;
		} else if (s.compare("nstep") == 0) {
			_sim_params.nsteps = int_scalars[i].value;
		} else if (s.compare("globalnumblocks") == 0) {
			_sim_params.total_blocks = int_scalars[i].value;
		}
	}



	typedef struct real_scalars_t {
	    char name[strlen];	// Ugh. Shouldn't be hard-coded
	    double value;
	} real_scalars_t;

	real_scalars_t *real_scalars = NULL;

	dataset = H5Dopen(_datasetId, "real scalars");
	if (dataset < 0) return(-1);

    dataspace = H5Dget_space(dataset);
	if (dataspace < 0) return(-1);

    H5Sget_simple_extent_dims (dataspace, fields, maxfields);
	nelem = fields[0];

	sp_type = H5Tcreate(H5T_COMPOUND, sizeof(real_scalars_t));

	strid = H5Tcopy(H5T_C_S1);
	H5Tset_size(strid, sizeof(real_scalars[0].name));
	H5Tinsert(sp_type, 
		  "name", 
		  offsetof(real_scalars_t, name),
		  strid
	);
 
	H5Tinsert(sp_type,
		  "value",
		  offsetof(real_scalars_t, value),
		  H5T_NATIVE_DOUBLE);


	real_scalars = (real_scalars_t *) new unsigned char[nelem * H5Tget_size(sp_type)];

	rc = H5Dread(
		dataset, sp_type, H5S_ALL, H5S_ALL, H5P_DEFAULT, real_scalars
	);

	H5Tclose(sp_type);
	H5Dclose(dataset);

	if (rc<0) return((int) rc);

	for (int i = 0; i<nelem; i++) {
		string s;
		s.assign(real_scalars[i].name, strlen);

		for (int j=0; j<strlen; j++) {
			if (s[j] == ' ') {
				s.erase(s.begin()+j, s.end()); 
				break;
			}
		}

		if (s.compare("time") == 0) {
			_sim_params.time = real_scalars[i].value;
		} else if (s.compare("dt") == 0) {
			_sim_params.timestep = real_scalars[i].value;
		}
	}

	return(0);
}

int FlashHDFFile::_GetIntRunParams() {

	const int strlen = 80;

	hid_t sp_type;
	herr_t rc;

	typedef struct run_params_t {
	    char name[strlen];	// Ugh. Shouldn't be hard-coded
	    int value;
	} run_params_t;

	run_params_t *run_params = NULL;

	hid_t dataset = H5Dopen(_datasetId, "integer runtime parameters");
	if (dataset < 0) return(-1);

    hid_t dataspace = H5Dget_space(dataset);
	if (dataspace < 0) return(-1);

    hsize_t fields[2];
    hsize_t maxfields[2];
    H5Sget_simple_extent_dims (dataspace, fields, maxfields);
	size_t nelem = fields[0];

	sp_type = H5Tcreate(H5T_COMPOUND, sizeof(run_params_t));

	hid_t strid = H5Tcopy(H5T_C_S1);
	H5Tset_size(strid, sizeof(run_params[0].name));
	H5Tinsert(sp_type, 
		  "name", 
		  offsetof(run_params_t, name),
		  strid
	);
 
	H5Tinsert(sp_type,
		  "value",
		  offsetof(run_params_t, value),
		  H5T_NATIVE_INT);


	run_params = (run_params_t *) new unsigned char[nelem * H5Tget_size(sp_type)];

	rc = H5Dread(
		dataset, sp_type, H5S_ALL, H5S_ALL, H5P_DEFAULT, run_params
	);

	H5Tclose(sp_type);
	H5Dclose(dataset);

	if (rc<0) return((int) rc);

	for (int i = 0; i<nelem; i++) {
		string s;
		s.assign(run_params[i].name, strlen);

		for (int j=0; j<strlen; j++) {
			if (s[j] == ' ') {
				s.erase(s.begin()+j, s.end()); 
				break;
			}
		}

		if (s.compare("nblockx") == 0) {
			_int_run_params.nblock[0] = run_params[i].value;
		} else if (s.compare("nblocky") == 0) {
			_int_run_params.nblock[1] = run_params[i].value;
		} else if (s.compare("nblockz") == 0) {
			_int_run_params.nblock[2] = run_params[i].value;
		} else if (s.compare("lrefine_min") == 0) {
			_int_run_params.lrefine_min = run_params[i].value;
		} else if (s.compare("lrefine_max") == 0) {
			_int_run_params.lrefine_max = run_params[i].value;
		}
	}

	return(0);
}

int FlashHDFFile::_GetRealRunParams()
{
	const int strlen = 80;

	hid_t sp_type;
	herr_t rc;

	typedef struct run_params_t {
	    char name[strlen];	// Ugh. Shouldn't be hard-coded
	    double value;
	} run_params_t;

	run_params_t *run_params = NULL;

	hid_t dataset = H5Dopen(_datasetId, "real runtime parameters");
	if (dataset < 0) return(-1);

    hid_t dataspace = H5Dget_space(dataset);
	if (dataspace < 0) return(-1);

    hsize_t fields[2];
    hsize_t maxfields[2];
    H5Sget_simple_extent_dims (dataspace, fields, maxfields);
	size_t nelem = fields[0];

	sp_type = H5Tcreate(H5T_COMPOUND, sizeof(run_params_t));

	hid_t strid = H5Tcopy(H5T_C_S1);
	H5Tset_size(strid, sizeof(run_params[0].name));
	H5Tinsert(sp_type, 
		  "name", 
		  offsetof(run_params_t, name),
		  strid
	);
 
	H5Tinsert(sp_type,
		  "value",
		  offsetof(run_params_t, value),
		  H5T_NATIVE_DOUBLE);

	run_params = (run_params_t *) new unsigned char[nelem * H5Tget_size(sp_type)];

	rc = H5Dread(dataset, sp_type, H5S_ALL, H5S_ALL, H5P_DEFAULT, run_params);

	H5Tclose(sp_type);
	H5Dclose(dataset);

	if (rc < 0) return (rc);

	for (int i = 0; i<nelem; i++) {
		string s;
		s.assign(run_params[i].name, strlen);

		for (int j=0; j<strlen; j++) {
			if (s[j] == ' ') {
				s.erase(s.begin()+j, s.end()); 
				break;
			}
		}

		if (s.compare("xmin") == 0) {
			_real_run_params.min[0] = run_params[i].value;
		} else if (s.compare("xmax") == 0) {
			_real_run_params.max[0] = run_params[i].value;
		} else if (s.compare("xmin") == 0) {
			_real_run_params.min[1] = run_params[i].value;
		} else if (s.compare("ymax") == 0) {
			_real_run_params.max[1] = run_params[i].value;
		} else if (s.compare("zmin") == 0) {
			_real_run_params.min[2] = run_params[i].value;
		} else if (s.compare("zmax") == 0) {
			_real_run_params.max[2] = run_params[i].value;
		}
	}

	return(0);
}

int FlashHDFFile::_GetStringRunParams() 

{

	const int strlen = 80;

	hid_t sp_type;
	herr_t rc;

	typedef struct run_params_t {
	    char name[strlen];	// Ugh. Shouldn't be hard-coded
	    char value[strlen];	// Ugh. Shouldn't be hard-coded
	} run_params_t;

	run_params_t *run_params = NULL;

	hid_t dataset = H5Dopen(_datasetId, "string runtime parameters");
	if (dataset < 0) return(-1);

    hid_t dataspace = H5Dget_space(dataset);
	if (dataspace < 0) return(-1);

    hsize_t fields[2];
    hsize_t maxfields[2];
    H5Sget_simple_extent_dims (dataspace, fields, maxfields);
	size_t nelem = fields[0];

	sp_type = H5Tcreate(H5T_COMPOUND, sizeof(run_params_t));

	hid_t strid1 = H5Tcopy(H5T_C_S1);
	H5Tset_size(strid1, sizeof(run_params[0].name));
	H5Tinsert(sp_type, "name", offsetof(run_params_t, name), strid1);
 
	hid_t strid2 = H5Tcopy(H5T_C_S1);
	H5Tset_size(strid2, sizeof(run_params[0].value));
	H5Tinsert(sp_type, "value", offsetof(run_params_t, value), strid2);

	run_params = (run_params_t *) new unsigned char[nelem * H5Tget_size(sp_type)];

	rc = H5Dread(dataset, sp_type, H5S_ALL, H5S_ALL, H5P_DEFAULT, run_params);

	H5Tclose(sp_type);
	H5Dclose(dataset);

	if (rc < 0) return(-1);

	for (int i = 0; i<nelem; i++) {
		string s;
		s.assign(run_params[i].name, strlen);

		for (int j=0; j<strlen; j++) {
			if (s[j] == ' ') {
				s.erase(s.begin()+j, s.end()); 
				break;
			}
		}

		if (s.compare("geometry") == 0) {
			_string_run_params.geometry.assign(run_params[i].value);
		} else if (s.compare("xl_boundary_type") == 0) {
			_string_run_params.xl_boundary_type.assign(run_params[i].value);
		} else if (s.compare("xr_boundary_type") == 0) {
			_string_run_params.xr_boundary_type.assign(run_params[i].value);
		} else if (s.compare("yl_boundary_type") == 0) {
			_string_run_params.yl_boundary_type.assign(run_params[i].value);
		} else if (s.compare("yr_boundary_type") == 0) {
			_string_run_params.yr_boundary_type.assign(run_params[i].value);
		} else if (s.compare("zl_boundary_type") == 0) {
			_string_run_params.zl_boundary_type.assign(run_params[i].value);
		} else if (s.compare("zr_boundary_type") == 0) {
			_string_run_params.zr_boundary_type.assign(run_params[i].value);
		}
	}

	return(0);
}

int FlashHDFFile::_GetNumberOfLeafs()
{
	_numberOfLeafs = 0;

	for(int i=0;i<GetNumberOfBlocks();i++) {
		int rc = GetNodeType(i);
		if (rc<0) return(-1);
		if (rc == LEAF_NODE) _numberOfLeafs++;
	}
	return(0);
    
}

int FlashHDFFile::_GetFlashVersion()
{

	// See if we are dealing with Flash 2 or 3

	if (_member_present(_datasetId, "file format version" )) { 
		_flashVersion = 2;
		return(0);
	}
	else if (_member_present(_datasetId, "sim info")) {
		_flashVersion = 3;
		return(0);
	}

	cerr << "Could not determine Flash file version number" << endl;
	return(-1);
}



bool FlashHDFFile::_member_present(const hid_t container_id, const string &name) {
	bool ret_val = false;  // Default state

	if (_member_index(container_id, name) >= 0) ret_val = true;

	return ret_val;
}

int FlashHDFFile::_member_index(const hid_t container_id, const ::string & name)
{
	int ret_val = -1;  // Error state

	if (container_id >= 0) {
		const int num_members = _get_num_members(container_id);

		if (num_members > 0) {
			int member_index = 0;

			bool present = false;

			while ((member_index < num_members) && (!present)) {
				const int string_length
				= H5Gget_objname_by_idx(container_id, member_index, 0, 0) + 1;

				if (string_length > 0) {
					char * string_container = new char[string_length];

					H5Gget_objname_by_idx(
						container_id, member_index, string_container,
						string_length
					);

					if (name == string_container) {
						present = true;
						ret_val = member_index;
					}

					delete[] string_container;
					string_container = 0;
				}

				member_index++;
			}
		}
	}

	return ret_val;
}
  
int FlashHDFFile::_get_num_members(const hid_t container_id)
{
	int ret_val = -1;  // Error state

	if (container_id >= 0) {
		hsize_t num_members = 0;

		if (H5Gget_num_objs(container_id, &num_members) >= 0) {
			ret_val = static_cast<int>(num_members);
		}
	}

	return ret_val;
}
