#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <sstream>

#include <vapor/OptionParser.h>
#include <vapor/CFuncs.h>
#include <vapor/WASP.h>

using namespace VetsUtil;
using namespace VAPoR;


struct opt_t {
	vector <string> dimnames;
	vector <int> dimlens;
	string ofile;
	OptionParser::Boolean_T	help;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"dimnames",1, "",	"Colon delimited list of dimension names"},
	{"dimlens",1, "",	"Colon delimited list of dimension lengths"},
	{"ofile",1, "test.nc",	"Output file"},
	{"help",	0,	"",	"Print this message and exit"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
    {"dimnames", VetsUtil::CvtToStrVec, &opt.dimnames, sizeof(opt.dimnames)},
    {"dimlens", VetsUtil::CvtToIntVec, &opt.dimlens, sizeof(opt.dimlens)},
    {"ofile", VetsUtil::CvtToCPPStr, &opt.ofile, sizeof(opt.ofile)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{NULL}
};

string ProgName;

vector <string> split(string s, string delim) {

	size_t pos = 0;
	vector <string> tokens;
	while ((pos = s.find(delim)) != std::string::npos) {
		tokens.push_back(s.substr(0, pos));
		s.erase(0, pos + delim.length());
	}
	if (! s.empty()) tokens.push_back(s);

	return(tokens);
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

	if (opt.help) {
		cerr << "Usage: " << ProgName << " [options] [name:type:count[:dimname]+]+" << endl;
		op.PrintOptionHelp(stderr, 80, false);
		exit(0);
	}

	if (opt.dimnames.size() != opt.dimlens.size()) { 
		MyBase::SetErrMsg("dimension name and dimension length vector size mismatch");
		exit(1);
	}

	WASP   wasp;
	vector <size_t> bs;
	bs.push_back(64);
	bs.push_back(64);
	bs.push_back(64);
	size_t chunksize = 1024*1024*4;

	vector <size_t> cratios3D;
	cratios3D.push_back(1);
	cratios3D.push_back(10);
	cratios3D.push_back(100);
	cratios3D.push_back(500);

	vector <size_t> cratios2D;
	cratios2D.push_back(1);
	cratios2D.push_back(5);
	cratios2D.push_back(25);
	cratios2D.push_back(100);

	vector <size_t> cratios1D;
	cratios1D.push_back(1);
	cratios1D.push_back(2);
	cratios1D.push_back(4);
	cratios1D.push_back(8);

	int rc = wasp.Create(
		opt.ofile, NC_64BIT_OFFSET, 0, chunksize,
		string("bior3.3"), bs, cratios3D.size(), true
	);
	if (rc<0) exit(1);

	int dummy;
	rc = wasp.SetFill(NC_NOFILL, dummy);
	if (rc<0) exit(1);

	for (int i=0; i<opt.dimnames.size(); i++) {
		int rc = wasp.DefDim(opt.dimnames[i], opt.dimlens[i]);
		if (rc<0) exit(1);
	}

	argc--;
	argv++;
	for (int i=0; i<argc; i++) {
		string s = argv[i];
		vector <string> vardef = split(s, ":");
		if (vardef.size() < 4) {
			MyBase::SetErrMsg("Invalid variable definition : %s", s.c_str());
			exit(1);
		}

		string name = vardef[0]; 
		vardef.erase(vardef.begin());

		string xtypestr = vardef[0];
		vardef.erase(vardef.begin());
		if (xtypestr.compare("NC_FLOAT") != 0) {
			MyBase::SetErrMsg("Unsupported xtype : %s", xtypestr.c_str());
			exit(1);
		}
		int xtype = NC_FLOAT;

		size_t ncdims;
		std::istringstream(vardef[0]) >> ncdims;
		vardef.erase(vardef.begin());

		// Everything else should be dimension name
		//
		vector <string> dimnames = vardef;

		if (ncdims == 0) {
			int rc = wasp.DefVar(name, xtype, dimnames);
			if (rc<0) exit(1);
			
		}
		else {
			vector <size_t> cratios;
			if (ncdims == 3) cratios = cratios3D;
			if (ncdims == 2) cratios = cratios2D;
			if (ncdims == 1) cratios = cratios1D;
			int rc = wasp.DefVar(name, xtype, dimnames, cratios);
			if (rc<0) exit(1);
		}

	}

	wasp.EndDef();
	wasp.Close();
}
