#include <iostream>
#include <fstream>
#include <sstream>
#include <vapor/OptionParser.h>
#include <vapor/WaveCodecIO.h>
#include <vapor/VDCFactory.h>
#include <vapor/MetadataSpherical.h>

using namespace VAPoR;
using namespace VetsUtil;
using namespace std;

VDCFactory::VDCFactory(bool vdc2) {
	_vdc2 = vdc2;
	_bs.nx = _bs.ny = _bs.nz = -1;
	_level = 2;
	_nfilter = 1;
	_nlifting = 1;
	_cratios.push_back(1);
	_cratios.push_back(10);
	_cratios.push_back(100);
	_cratios.push_back(500);
	_wname = "bior3.3";
    _vars3d.clear();
    _vars2dxy.clear();
    _vars2dxz.clear();
    _vars2dyz.clear();
	_missing = 0.0;
	_comment = "";
	_order.push_back(0);
	_order.push_back(1);
	_order.push_back(2);
	_periodic.push_back(0);
	_periodic.push_back(0);
	_periodic.push_back(0);
	_numts = 1;
	_startt = 0.0;
	_deltat = 0.0;
	_gridtype = "regular";
	_usertimes.clear();
	_xcoords.clear();
	_ycoords.clear();
	_zcoords.clear();
	_mapprojection.clear();
	_coordsystem = "cartesian";
	_extents.push_back(0.0);
	_extents.push_back(0.0);
	_extents.push_back(0.0);
	_extents.push_back(0.0);
	_extents.push_back(0.0);
	_extents.push_back(0.0);

	_removeOptions.clear();
}

int VDCFactory::Parse(int *argc, char **argv) {
	OptionParser::OptDescRec_T  set_opts[] = {
		{
			"bs", 1, "-1x-1x-1",
			"Internal storage blocking factor expressed in grid points "
			"(NXxNYxNZ). Defaults: 32x32x32 (VDC type 1), 64x64x64 (VDC type 2)"
		},
		{
			"level", 1, "2", "Number of approximation levels in hierarchy. "
			"0 => no approximations, 1 => one approximation, and so on "
			"(VDC 1 only)"
		},
		{
			"nfilter", 1, "1",
			"Number of wavelet filter coefficients (VDC 1 only)"
		},
		{
			"nlifting", 1, "1",
			"Number of wavelet lifting coefficients (VDC 1 only)"
		},
		{
			"cratios",1, "", "Colon delimited list compression ratios. "
			"The default is 1:10:100:500. The maximum compression ratio "
			"is wavelet and block size dependent."
		},
		{	
			"wname", 1,"bior3.3", "Wavelet family used for compression "
			"(VDC type 2, only). Recommended values are bior1.1, bior1.3, "
			"bior1.5, bior2.2, bior2.4 ,bior2.6, bior2.8, bior3.1, bior3.3, "
			"bior3.5, bior3.7, bior3.9, bior4.4"
		},
		{
			"varnames",1,	"", "Deprecated. Use -vars3d instead"
		},
		{
			"vars3d",1, "", 
			"Colon delimited list of 3D variable names to be included in "
			"the VDF"
		},
		{
			"vars2dxy",1, "", "Colon delimited list of 2D XY-plane variable "
			"names to be included in the VDF"
		},
		{
			"vars2dxz",1, "", "Colon delimited list of 3D XZ-plane variable "
			"names to be included in the VDF"
		},
		{
			"vars2dyz",1, "", "Colon delimited list of 3D YZ-plane variable "
			"names to be included in the VDF"
		},
		{
			"missing", 1, "0.0", 
			"Missing data value. If 0.0, there are no missing data"
		},
		{
			"comment", 1, "", "Top-level comment to be included in VDF"
		},
		{
			"order", 1, "0:1:2", "Colon delimited 3-element vector specifying "
			"permutation ordering of raw data on disk "
		},
		{
			"periodic",	1,	"0:0:0",	"Colon delimited 3-element boolean "
			"(0=>nonperiodic, 1=>periodic) vector specifying periodicity of "
			"X,Y,Z coordinate axes (X:Y:Z)"
		},
		{
			"numts", 1, "1", "Number of timesteps in the data set"
		},
		{
			"startt", 1, "0.0",
			"Time, in user coordinates, of the first time step"
		},
		{
			"deltat", 1, "1.0",
			"Increment between time steps expressed in user time coordinates"
		},
		{
			"gridtype",	1, "regular",
			"Data grid type (regular|layered|stretched|block_amr)"
		}, 
		{
			"usertimes", 1,	"",	"Path to a file containing a whitespace "
			"delineated list of user times. If present, -numts "
			"option is ignored."
		}, 
		{
			"xcoords",	1,	"",	"Path to a file containing a whitespace "
			"delineated list of X-axis user coordinates. Ignored unless "
			"gridtype is stretched."
		},
		{
			"ycoords",	1,	"",	"Path to a file containing a whitespace "
			"delineated list of Y-axis user coordinates. Ignored unless "
			"gridtype is stretched."
		},
		{
			"zcoords",	1,	"",	"Path to a file containing a whitespace "
			"delineated list of Z-axis user coordinates. Ignored unless "
			"gridtype is stretched."
		},
		{
			"mapprojection",	1,	"",	"A whitespace delineated, quoted list "
			"of PROJ key/value pairs of the form '+paramname=paramvalue'. "
			"vdfcreate does not validate the string for correctness."
		},
		{
			"coordsystem",	1,	"cartesian","Data coordinate system "
			"(cartesian|spherical)"
		},
		{
			"extents",	1,	"0:0:0:0:0:0",	"Colon delimited 6-element vector "
			"specifying domain extents in user coordinates (X0:Y0:Z0:X1:Y1:Z1)"
		},
		{NULL}
	};

	OptionParser::Option_T  get_options[] = {
		{"bs", VetsUtil::CvtToDimension3D, &_bs, sizeof(_bs)},
		{"level", VetsUtil::CvtToInt, &_level, sizeof(_level)},
		{"nfilter", VetsUtil::CvtToInt, &_nfilter, sizeof(_nfilter)},
		{"nlifting", VetsUtil::CvtToInt, &_nlifting, sizeof(_nlifting)},
		{"cratios", VetsUtil::CvtToIntVec, &_cratios, sizeof(_cratios)},
		{"wname", VetsUtil::CvtToCPPStr, &_wname, sizeof(_wname)},
		{"varnames", VetsUtil::CvtToStrVec, &_vars3d, sizeof(_vars3d)},
		{"vars3d", VetsUtil::CvtToStrVec, &_vars3d, sizeof(_vars3d)},
		{"vars2dxy", VetsUtil::CvtToStrVec, &_vars2dxy, sizeof(_vars2dxy)},
		{"vars2dxz", VetsUtil::CvtToStrVec, &_vars2dxz, sizeof(_vars2dxz)},
		{"vars2dyz", VetsUtil::CvtToStrVec, &_vars2dyz, sizeof(_vars2dyz)},
		{"missing", VetsUtil::CvtToFloat, &_missing, sizeof(_missing)},
		{"comment", VetsUtil::CvtToCPPStr, &_comment, sizeof(_comment)},
		{"order", VetsUtil::CvtToIntVec, &_order, sizeof(_order)},
		{"periodic", CvtToIntVec, &_periodic, sizeof(_periodic)},
		{"numts", VetsUtil::CvtToInt, &_numts, sizeof(_numts)},
		{"startt", VetsUtil::CvtToDouble, &_startt, sizeof(_startt)},
		{"deltat", VetsUtil::CvtToDouble, &_deltat, sizeof(_deltat)},
		{"gridtype", VetsUtil::CvtToCPPStr, &_gridtype, sizeof(_gridtype)},
		{"usertimes", VetsUtil::CvtToCPPStr, &_usertimes, sizeof(_usertimes)},
		{"xcoords", VetsUtil::CvtToCPPStr, &_xcoords, sizeof(_xcoords)},
		{"ycoords", VetsUtil::CvtToCPPStr, &_ycoords, sizeof(_ycoords)},
		{"zcoords", VetsUtil::CvtToCPPStr, &_zcoords, sizeof(_zcoords)},
		{"mapprojection", VetsUtil::CvtToCPPStr, &_mapprojection, sizeof(_mapprojection)},
		{"coordsystem", VetsUtil::CvtToCPPStr, &_coordsystem, sizeof(_coordsystem)},
		{"extents", VetsUtil::CvtToFloatVec, &_extents, sizeof(_extents)},
		{NULL}
	};

	if (_op.AppendOptions(set_opts) < 0) return(-1);
	if (_removeOptions.size()) _op.RemoveOptions(_removeOptions);
    if (_op.ParseOptions(argc, argv, get_options) < 0) return(-1);

	if (_order.size() != 3) {
		SetErrMsg("Invalid argument to -order");
		return(-1);
	}
	if (_periodic.size() != 3) {
		SetErrMsg("Invalid argument to -periodic");
		return(-1);
	}
	if (_extents.size() != 6) {
		SetErrMsg("Invalid argument to -extents");
		return(-1);
	}


	return(0);
}

void VDCFactory::Usage(FILE *fp) {
	_op.PrintOptionHelp(stderr);
}
	

MetadataVDC *VDCFactory::New(const size_t dims[3]) const {

	MetadataVDC *file;

	if (_vdc2) {

		string wmode;
		if ((_wname.compare("bior1.1") == 0) ||
			(_wname.compare("bior1.3") == 0) ||
			(_wname.compare("bior1.5") == 0) ||
			(_wname.compare("bior3.3") == 0) ||
			(_wname.compare("bior3.5") == 0) ||
			(_wname.compare("bior3.7") == 0) ||
			(_wname.compare("bior3.9") == 0)) {

			wmode = "symh"; 
		}
		else if ((_wname.compare("bior2.2") == 0) ||
			(_wname.compare("bior2.4") == 0) ||
			(_wname.compare("bior2.6") == 0) ||
			(_wname.compare("bior2.8") == 0) ||
			(_wname.compare("bior4.4") == 0)) {

			wmode = "symw"; 
		}
		else {
			wmode = "sp0"; 
		}

		size_t bs[3];
		if (_bs.nx < 0) {
			for (int i=0; i<3; i++) bs[i] = 64;
		}
		else {
			bs[0] = _bs.nx; bs[1] = _bs.ny; bs[2] = _bs.nz;
		}

		vector <size_t> cratios;
		for (int i=0; i<_cratios.size(); i++) cratios.push_back(_cratios[i]);
		if (cratios.size() == 0) {
			cratios.push_back(1);
			cratios.push_back(10);
			cratios.push_back(100);
			cratios.push_back(500);
		}

		size_t maxcratio = WaveCodecIO::GetMaxCRatio(bs, _wname, wmode);
		for (int i=0;i<cratios.size();i++) {
			if (cratios[i] == 0 || cratios[i] > maxcratio) {
				MyBase::SetErrMsg(
					"Invalid compression ratio (%d) for configuration "
					"(block_size, wavename)", cratios[i]
				);
				return(NULL);
			}
		}

		file = new MetadataVDC(dims,bs,cratios,_wname,wmode);
	}
	else if (_coordsystem.compare("spherical") == 0) {
		size_t perm[] = {_order[0], _order[1], _order[2]};

		size_t bs[3];
		if (_bs.nx < 0) {
			for (int i=0; i<3; i++) bs[i] = 32;
		}
		else {
			bs[0] = _bs.nx; bs[1] = _bs.ny; bs[2] = _bs.nz;
		}

		file = new MetadataSpherical(dims,_level,bs,perm, _nfilter,_nlifting);
	}
	else {
		size_t bs[3];
		if (_bs.nx < 0) {
			for (int i=0; i<3; i++) bs[i] = 32;
		}
		else {
			bs[0] = _bs.nx; bs[1] = _bs.ny; bs[2] = _bs.nz;
		}
		file = new MetadataVDC(dims,_level,bs,_nfilter,_nlifting);
	}

	if (MyBase::GetErrCode()) return(NULL);

	if (_usertimes.length()) {

		vector <double> usertimes;
		if (_ReadDblVec(_usertimes, usertimes)<0) return(NULL);

		if (file->SetNumTimeSteps(usertimes.size()) < 0) return(NULL);

		for (size_t t=0; t<usertimes.size(); t++) {
			vector <double> vec(1,usertimes[t]);
			if (file->SetTSUserTime(t, vec) < 0) return(NULL);
		}

	} else {
		if (file->SetNumTimeSteps(_numts) < 0) return(NULL);

		double usertime = _startt;
		for (size_t t=0; t < _numts; t++) {
			vector <double> vec(1,usertime);
			if(file->SetTSUserTime(t,vec) < 0) return(NULL);

			usertime += _deltat;
		}
	}

	if (_mapprojection.size()) {
		if (file->SetMapProjection(_mapprojection) < 0) return(NULL);
	}

	if (file->SetComment(_comment) < 0) return(NULL);

	if (file->SetGridType(_gridtype) < 0) return(NULL);

	{
		vector <long> periodic_vec;

		for (int i=0; i<3; i++) periodic_vec.push_back(_periodic[i]);

		if (file->SetPeriodicBoundary(periodic_vec) < 0) return(NULL);
	}


	vector <double> xcoords;
	vector <double> ycoords;
	vector <double> zcoords;
	if (_gridtype.compare("layered") == 0) {
		vector <string> vars3d = file->GetVariables3D();
		if (find(vars3d.begin(), vars3d.end(), string("ELEVATION")) == vars3d.end()) {
			vars3d.push_back("ELEVATION");
			file->SetVariables3D(vars3d);
		}
	}
	else if (file->GetGridType().compare("stretched") == 0){
		if (_xcoords.size()) {
			if (_ReadDblVec(_xcoords, xcoords)<0) return(NULL);

			if (xcoords.size() != 0) {
				for (size_t ts=0; ts<file->GetNumTimeSteps(); ts++) {
					if (file->SetTSXCoords(ts, xcoords)<0) return(NULL);
				}
			}
		}
		if (_ycoords.size()) {
			if (_ReadDblVec(_ycoords, ycoords)<0) return(NULL);
			if (ycoords.size() != 0) {
				for (size_t ts=0; ts<file->GetNumTimeSteps(); ts++) {
					if (file->SetTSYCoords(ts, ycoords)<0) return(NULL);
				}
			}
		}
		if (_zcoords.size()) {
			if (_ReadDblVec(_zcoords, zcoords)<0) return(NULL);

			if (zcoords.size() != 0) {
				for (size_t ts=0; ts<file->GetNumTimeSteps(); ts++) {
					if (file->SetTSZCoords(ts, zcoords)<0) return(NULL);
				}
			}
		}
	}

	// 
	// default user extents
	//
	vector <double> extents;
    double maxdim = max(dims[0], max(dims[1],dims[2]));
	extents.push_back(0.0);
	extents.push_back(0.0);
	extents.push_back(0.0);
	extents.push_back((double) dims[0] / maxdim);
	extents.push_back((double) dims[1] / maxdim);
	extents.push_back((double) dims[2] / maxdim);

	bool doExtents = false;
	for(int i=0; i<5; i++) {
		if (_extents[i] != _extents[i+1]) doExtents = true;
	}
	if (doExtents) {
		for(int i=0; i<6; i++) {
			extents[i] = _extents[i];
		}
	}
	if (xcoords.size()) {
		extents[0] = xcoords[0];
		extents[0+3] = xcoords[xcoords.size()-1];
	}
	if (ycoords.size()) {
		extents[1] = ycoords[0];
		extents[1+3] = ycoords[ycoords.size()-1];
	}
	if (zcoords.size()) {
		extents[2] = zcoords[0];
		extents[2+3] = zcoords[zcoords.size()-1];
	}
	if (file->SetExtents(extents) < 0) return(NULL);

	if (_missing != 0.0) {
		file->SetMissingValue(_missing);
	}

	if (file->SetVariables3D(_vars3d) < 0) return(NULL);

	if (file->SetVariables2DXY(_vars2dxy) < 0) return(NULL);
	if (file->SetVariables2DXZ(_vars2dxz) < 0) return(NULL);
	if (file->SetVariables2DYZ(_vars2dyz) < 0) return(NULL);

	return(file);

}

int	VDCFactory::_ReadDblVec(string path, vector <double> &vec) const {

	ifstream fin(path.c_str());
	if (! fin) { 
		MyBase::SetErrMsg("Error opening file %s", path.c_str());
		return(-1);
	}

	vec.clear();

	double d;
	while (fin >> d) {
		vec.push_back(d);
	}
	fin.close();

	// Make sure times are monotonic.
	//
	int mono = 1;
	if (vec.size() > 1) {
		if (vec[0]>=vec[vec.size()-1]) {
			for(int i=0; i<vec.size()-1; i++) {
				if (vec[i]<vec[i+1]) mono = 0;
			}
		} else {
			for(int i=0; i<vec.size()-1; i++) {
				if (vec[i]>vec[i+1]) mono = 0;
			}
		}
	}
	if (! mono) {
		MyBase::SetErrMsg("Sequence contained in %s must be monotonic", path.c_str());
		return(-1);
	}

	return(0);
}
