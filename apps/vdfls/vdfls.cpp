//
// $Id$
//
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <cstdio>
#include <cerrno>
#include <cassert>

#include <sys/types.h>
#include <sys/stat.h>


#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/MetadataVDC.h>
#include <vapor/VDFIOBase.h>
#ifdef WIN32
#pragma warning(disable : 4996)
#endif
using namespace VetsUtil;
using namespace VAPoR;


struct opt_t {
	int	level;
	char *sort;
	OptionParser::Boolean_T	l;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	debug;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"level",1, "-1","Refinement levels reported. 0=>coarsest, 1=>next refinement, etc. -1=>all"},
	{"sort",1, "time","Sort order, one of (time|level|varname)"},
	{"long",	0,	"",	"Use a long listing format"},
	{"help",	0,	"",	"Print this message and exit"},
	{"debug",	0,	"",	"Enable debugging"},
	{"quiet",	0,	"",	"Operate quitely"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"sort", VetsUtil::CvtToString, &opt.sort, sizeof(opt.sort)},
	{"long", VetsUtil::CvtToBoolean, &opt.l, sizeof(opt.l)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"debug", VetsUtil::CvtToBoolean, &opt.debug, sizeof(opt.debug)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{NULL}
};

const char	*ProgName;

class VarFileInfo {
 public:

	VarFileInfo();
	void Insert(int j, string &path, struct STAT64 &buf);
	bool Test(int j) const;
	const struct STAT64 &GetStat(int j) const;
	const string &GetPath(int j) const;
	bool Empty() const;
	void Clear();
	VDFIOBase::VarType_T vartype;
 private:
	struct STAT64 _emptyStat;
	string _emptyPath;
	map <int, struct STAT64> _varStatMap;
	map <int, string> _varPathMap;
};

VarFileInfo::VarFileInfo() {
	_varStatMap.clear();
	_varPathMap.clear();
	memset(&_emptyStat, 0, sizeof (_emptyStat));
	_emptyPath.clear();
}

void VarFileInfo::Insert(int j, string &path, struct STAT64 &buf) {
	_varStatMap[j] = buf;
	_varPathMap[j] = path;
}

bool VarFileInfo::Test(int j) const {
	map <int,  struct STAT64>::const_iterator iter = _varStatMap.find(j);

	return(iter != _varStatMap.end());
}

const struct STAT64 &VarFileInfo::GetStat(int j) const {
	map <int,  struct STAT64>::const_iterator iter = _varStatMap.find(j);

	if (iter != _varStatMap.end()) return (iter->second);

	return(_emptyStat);
}

const string &VarFileInfo::GetPath(int j) const {
	map <int,  string>::const_iterator iter = _varPathMap.find(j);

	if (iter != _varPathMap.end()) return (iter->second);

	return(_emptyPath);
}

bool VarFileInfo::Empty() const {
	return(_varStatMap.empty());
}

void VarFileInfo::Clear() {
	_varStatMap.clear();
	_varPathMap.clear();
}

void    mkpath(const string &basename, int level, string &path, int version) {
	ostringstream oss;

	if (version < 2) {
		oss << basename << ".wb" << level;
	}
	else {
		oss << basename << ".nc" << level;
	}
	path = oss.str();
}

int getStats(
	const MetadataVDC *metadata,
	map <long, map <string, VarFileInfo > > &statsvec
) {

	const vector <string> varNames = metadata->GetVariableNames();
	long numTimeSteps = metadata->GetNumTimeSteps();
	int vdfVersion = metadata->GetVDFVersion();
	int numApproximations = 0;
	if (metadata->GetVDCType() == 1) {
		numApproximations = metadata->GetNumTransforms() + 1;
	}
	else {
		numApproximations = metadata->GetCRatios().size();
	}

	for (long ts = 0; ts<numTimeSteps; ts++) {

		for (int i=0; i<varNames.size(); i++) {
			string varname = varNames[i];

			string basepath;
			if (metadata->ConstructFullVBase(ts, varname, &basepath) < 0) {
				MyBase::SetErrMsg(
					"Failed to construct path to  variable \"%s\"",
					varname.c_str()
				);
				return(-1); 
			}
			

			VarFileInfo vfi;
			vfi.vartype = metadata->GetVarType(varname);
			for (int j=0; j<numApproximations; j++) {
				string path;
				string relpath;
				struct STAT64 statbuf;
				int rc;

				mkpath(basepath, j, path, vdfVersion);

				rc = STAT64(path.c_str(), &statbuf);
				if (rc < 0) {
					if (errno != ENOENT) {
						MyBase::SetErrMsg(
							"Could not stat file \"%s\"", path.c_str()
						);
					}
					break;
				}
				const string &bp = metadata->GetVBasePath(ts, varname);
				mkpath(bp, j, relpath, vdfVersion);

				vfi.Insert(j, relpath, statbuf);
			}
			if (! vfi.Empty()) {
				map <string, VarFileInfo > &vref = statsvec[ts];
				vref[varname] = vfi;
			}
		}
	}
	return(0);
}

void ErrMsgCB (const char *msg, int) {
	cerr << ProgName << ": " << msg << endl;
}
//Windows has a different way of handling the mode bits
#ifdef WIN32
void print_mode(unsigned short st_mode) {
	if (_S_IREAD & st_mode) cout << "r"; else cout << "-";
	if (_S_IWRITE & st_mode) cout << "w"; else cout << "-";
	if (_S_IEXEC & st_mode) cout << "x"; else cout << "-";
}
#else
void print_mode(mode_t st_mode) {
	for (int i=2; i>=0; i--) {
		unsigned char o = st_mode >> i*3;
		if (o & 04) cout << "r"; else cout << "-";
		if (o & 02) cout << "w"; else cout << "-";
		if (o & 01) cout << "x"; else cout << "-";
	}
}
#endif

void print_dim(
	const MetadataVDC *metadata, int j, VDFIOBase::VarType_T vartype
) {

	ostringstream oss;
	size_t dim[3];

	// Find max width of field to output
	metadata->GetDim(dim, -1);
	if (vartype == VDFIOBase::VAR2D_XY) dim[2] = 1;
	if (vartype == VDFIOBase::VAR2D_XZ) dim[1] = 1;
	if (vartype == VDFIOBase::VAR2D_YZ) dim[0] = 1;
	oss << dim[0] << "x" << dim[1] << "x" << dim[2];
	int w = oss.str().length()+1;
	string empty;
	oss.str(empty);
	

	metadata->GetDim(dim, j);
	if (vartype == VDFIOBase::VAR2D_XY) dim[2] = 1;
	if (vartype == VDFIOBase::VAR2D_XZ) dim[1] = 1;
	if (vartype == VDFIOBase::VAR2D_YZ) dim[0] = 1;
	oss << dim[0] << "x" << dim[1] << "x" << dim[2];

	cout.setf(ios::right);
	cout.width(w);
	cout << oss.str();
	cout.setf(ios::left);
}

void print_cratios(
	const MetadataVDC *metadata, int j, VDFIOBase::VarType_T vartype
) {

	ostringstream oss;

	vector <size_t> cratios = metadata->GetCRatios();
	if (vartype != VDFIOBase::VAR3D) {
		for (int i=0; i<cratios.size(); i++) {
			cratios[i] = (size_t) pow((double) cratios[i], (double) (2.0 / 3.0));
		}
	}
	oss << cratios[j] << ":" << 1;

	int w = 8;
	cout.setf(ios::right);
	cout.width(w);
	cout << oss.str();
	cout.setf(ios::left);
}

void print_time(time_t t) {

	struct tm *tsptr = localtime(&t);
	const char *format = "%b %d %Y %H:%M";
	char buf[128];

	strftime(buf, sizeof(buf), format, tsptr);
	string str(buf);

	cout << str;
}

	
void PrintVariable(
	const MetadataVDC *metadata, const VarFileInfo &vfi, int j
) {

	if (vfi.Test(j)) {
		const struct STAT64 &statref = vfi.GetStat(j);
		const string &pathref = vfi.GetPath(j);
		if (opt.l) {
			print_mode(statref.st_mode);
			cout << " ";
			cout.setf(ios::right);
			cout.width(12);
			cout << statref.st_size;
			cout.setf(ios::left);
			cout << " ";
			if (metadata->GetVDCType() == 2) {
				print_cratios(metadata, j, vfi.vartype);
			} else {
				print_dim(metadata, j, vfi.vartype);
			}
			cout << " ";
			print_time(statref.st_mtime);
			cout << " ";

		}
			
		cout << pathref << endl;
	}
}

int	main(int argc, char **argv) {

	OptionParser op;
	const char	*metafile;

	string	s;

	MyBase::SetErrMsgCB(ErrMsgCB);

	ProgName = Basename(argv[0]);

	if (op.AppendOptions(set_opts) < 0) {
		cerr << ProgName << " : " << op.GetErrMsg();
		exit(1);
	}

	if (op.ParseOptions(&argc, argv, get_options) < 0) {
		cerr << ProgName << " : " << op.GetErrMsg();
		exit(1);
	}

	if (opt.help) {
		cerr << "Usage: " << ProgName << " [options] metafile datafile" << endl;
		op.PrintOptionHelp(stderr);
		exit(0);
	}

	if (argc != 2) {
		cerr << "Usage: " << ProgName << " [options] metafile " << endl;
		op.PrintOptionHelp(stderr);
		exit(1);
	}

	metafile = argv[1];
	map <long, map <string, VarFileInfo > > statsvec;


	const MetadataVDC *metadata = new MetadataVDC(metafile);
	if (MyBase::GetErrCode() != 0) exit(1);

	if (getStats(metadata, statsvec) < 0) {
		exit(1);
	}
	

	string sort = opt.sort;

	int numApproximations;
	if (metadata->GetVDCType() == 2) {
		if (opt.level < 0) numApproximations = metadata->GetCRatios().size();
		else numApproximations = opt.level+1;
	} else {
		if (opt.level < 0) numApproximations = metadata->GetNumTransforms()+1;
		else numApproximations = opt.level+1;
	}

	if (sort.compare("time") == 0) {
		map <long, map <string, VarFileInfo > >::iterator iter1;
		for (iter1 = statsvec.begin(); iter1 != statsvec.end(); iter1++) {
			map <string, VarFileInfo>::iterator iter2;

			for (iter2 = iter1->second.begin(); iter2 != iter1->second.end(); iter2++) {
				const VarFileInfo &vfiref = iter2->second;

				for(int j=0; j<numApproximations; j++) {
					PrintVariable(metadata,vfiref, j);
				}
			}
		}
	}
	else if (sort.compare("level") == 0) {
		for (int j=0; j<numApproximations; j++) {
			map <long, map <string, VarFileInfo > >::iterator iter1;
			for (iter1 = statsvec.begin(); iter1 != statsvec.end(); iter1++) {
				map <string, VarFileInfo>::iterator iter2;

				for (iter2 = iter1->second.begin(); iter2 != iter1->second.end(); iter2++) {
					const VarFileInfo &vfiref = iter2->second;

					PrintVariable(metadata,vfiref, j);
				}
			}
		}
	}
	else if (sort.compare("varname") == 0) {
		const vector <string> varNames = metadata->GetVariableNames();
		vector <string>::const_iterator iter1;
		for (iter1=varNames.begin(); iter1 != varNames.end(); iter1++) {
		
			for (int j=0; j<numApproximations; j++) {
				map <long, map <string, VarFileInfo > >::iterator iter2;
				for (iter2 = statsvec.begin(); iter2 != statsvec.end(); iter2++) {

					const VarFileInfo &vfiref = iter2->second[*iter1];;

					PrintVariable(metadata,vfiref, j);
				}
			}
		}
	}
	else {
		MyBase::SetErrMsg("Invalid sort \"%s\"", sort.c_str());
		exit(1);
	}

	exit(0);
}
