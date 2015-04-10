#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <sstream>

#include <vapor/OptionParser.h>
#include <vapor/CFuncs.h>
#include <vapor/VDCNetCDF.h>
#include <vapor/DCWRF.h>

using namespace VetsUtil;
using namespace VAPoR;


struct opt_t {
	int nthreads;
    std::vector <string> vars;
	OptionParser::Boolean_T	help;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{
		"nthreads",    1,  "0",    "Specify number of execution threads "
		"0 => use number of cores"
	},
	{
		"vars",1, "",
		"Colon delimited list of 3D variable names (compressed) "
		"to be included in "
		"the VDC"
	},
	{"help",	0,	"",	"Print this message and exit"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"nthreads", VetsUtil::CvtToInt, &opt.nthreads, sizeof(opt.nthreads)},
	{"vars", VetsUtil::CvtToStrVec, &opt.vars, sizeof(opt.vars)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{NULL}
};

string ProgName;

SmartBuf dataBuffer;

// Copy a variable with 2 or 3 spatial dimensions
//
int CopyVar2d3d(
	DC &dc, VDC &vdc, const vector <DC::Dimension> &dims,
	size_t ts, string varname, int lod
) {

	vector <DC::Dimension> mydims = dims;
	// See if time varying - time dimensions are always the slowest varying
	//
	if (mydims[mydims.size()-1].GetAxis() == 3) {
		mydims.pop_back();
	}
	assert(mydims.size()==2 || mydims.size() == 3);
	
	int rc = dc.OpenVariableRead(ts,varname, -1, -1);
	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to open variable %s for reading\n", varname.c_str()
		);
		return(-1);
	}

    rc = vdc.OpenVariableWrite(ts, varname, lod);
    if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to open variable %s for writing\n", varname.c_str()
		);
		return(-1);
	}

    size_t nz = mydims.size() > 2 ? mydims[mydims.size()-1].GetLength() : 1;

	size_t sz = mydims[0].GetLength() * mydims[1].GetLength();
	float *buf = (float *) dataBuffer.Alloc(sz*sizeof(*buf));

    for (size_t i=0; i<nz; i++) {
        rc = dc.ReadSlice(buf);
		if (rc<0) {
			MyBase::SetErrMsg(
				"Failed to read variable %s\n", varname.c_str()
			);
			return(-1);
		}

        int rc = vdc.WriteSlice(buf);
		if (rc<0) {
			MyBase::SetErrMsg(
				"Failed to write variable %s\n", varname.c_str()
			);
			return(-1);
		}
	}

    (void) dc.CloseVariable();
    rc = vdc.CloseVariable();
	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to write variable %s\n", varname.c_str()
		);
		return(-1);
	}

	return(0);
}

// Copy a variable with 0 or 1 spatial dimensions
//
int CopyVar0d1d(
	DC &dc, VDC &vdc, const vector <DC::Dimension> &dims,
	size_t ts, string varname, int lod
) {

	vector <DC::Dimension> mydims = dims;

	// See if time varying - time dimensions are always the slowest varying
	//
	if (mydims[mydims.size()-1].GetAxis() == 3) {
		mydims.pop_back();
	}
	assert(mydims.size()==0 || mydims.size() == 1);

	size_t sz=1;
	for (int i=0; i<mydims.size(); i++) {
		sz *= mydims[i].GetLength();
	}

	float *buf = (float *) dataBuffer.Alloc(sz * sizeof(*buf));

	int rc = dc.GetVar(ts, varname, -1, -1, buf);
	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to read variable %s\n", varname.c_str()
		);
		return(-1);
	}

	rc = vdc.PutVar(ts, varname, lod, buf);
	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to write variable %s\n", varname.c_str()
		);
		return(-1);
	}

	return(0);
}


int CopyVar(
	DC &dc, VDC &vdc,
	size_t ts, string varname, int lod
) {

	DC::BaseVar var;
	bool ok = vdc.GetBaseVarInfo(varname, var);
	assert(ok==true);

	vector <DC::Dimension> dims = var.GetDimensions();
	assert(dims.size() > 0 && dims.size() < 5);

	// Call appropriate copy method based on number of spatial dimensions
	//
	if (dims.size() == 1 || (dims.size()==2 && dims[1].GetAxis() == 3)) {
		return(CopyVar0d1d(dc, vdc, dims, ts, varname, lod));
	}
	else {
		return(CopyVar2d3d(dc, vdc, dims, ts, varname, lod));
	}
}
	

int	main(int argc, char **argv) {

	OptionParser op;

	MyBase::SetErrMsgFilePtr(stderr);
	//
	// Parse command line arguments
	//
	ProgName = Basename(argv[0]);


	if (op.AppendOptions(set_opts) < 0) {
		exit(1);
	}

	if (op.ParseOptions(&argc, argv, get_options) < 0) {
		exit(1);
	}

	if (argc < 3) {
		cerr << "Usage: " << ProgName << "wrffiles... master.nc" << endl;
		op.PrintOptionHelp(stderr, 80, false);
		exit(1);
	}
	

	if (opt.help) {
		cerr << "Usage: " << ProgName << " master.nc" << endl;
		op.PrintOptionHelp(stderr, 80, false);
		exit(0);
	}

	argc--;
	argv++;
	vector <string> wrffiles;
	for (int i=0; i<argc-1; i++) wrffiles.push_back(argv[i]);
	string master = argv[argc-1];

	VDCNetCDF    vdc(opt.nthreads);


	size_t chunksize = 1024*1024*4;
	int rc = vdc.Initialize(master, VDC::A, chunksize);
	if (rc<0) exit(1);

	DCWRF	dcwrf;
	rc = dcwrf.Initialize(wrffiles);
	if (rc<0) {
		exit(1);
	}

	// Necessary????
	//
//	rc = vdc.EndDefine();
//	if (rc<0) {
//		MyBase::SetErrMsg(
//			"Failed to write VDC master file : %s", master.c_str()
////		);
//		exit(1);
//	}

	vector <string> varnames = dcwrf.GetCoordVarNames();
	for (int i=0; i<varnames.size(); i++) {
		int nts = dcwrf.GetNumTimeSteps(varnames[i]);
		assert(nts >= 0);

		cout << "Copying variable " << varnames[i] << endl;

		for (int ts=0; ts<nts; ts++) {
			cout << "  Time step " << ts << endl;
			int rc = CopyVar(dcwrf, vdc, ts, varnames[i], -1);
			if (rc<0) exit(1);
		}
	}

	varnames = dcwrf.GetDataVarNames();
	for (int i=0; i<varnames.size(); i++) {
		int nts = dcwrf.GetNumTimeSteps(varnames[i]);
		assert(nts >= 0);

		cout << "Copying variable " << varnames[i] << endl;

		for (int ts=0; ts<nts; ts++) {

			cout << "  Time step " << ts << endl;

			int rc = CopyVar(dcwrf, vdc, ts, varnames[i], -1);
			if (rc<0) exit(1);
		}
	}


	return(0);

}
