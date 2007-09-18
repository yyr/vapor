#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/Metadata.h>
#include <vapor/MetadataSpherical.h>
#include <vapor/TransferFunctionLite.h>

using namespace VetsUtil;
using namespace VAPoR;


struct opt_t {
	char *omap;
	char *cmap;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"cmap",	1,	"",				"Path to ascii file containing color map"},
	{"omap",	1,	"",				"Path to ascii file containing opacity map"},
	{"help",	0,	"",				"Print this message and exit"},
	{"quiet",	0,	"",				"Operate quietly"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"cmap", VetsUtil::CvtToString, &opt.cmap, sizeof(opt.cmap)},
	{"omap", VetsUtil::CvtToString, &opt.omap, sizeof(opt.omap)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{NULL}
};


const char *ProgName;

void Usage(OptionParser &op, const char * msg) {

	if (msg) {
		cerr << ProgName << " : " << msg << endl;
	}
	cerr << "Usage: " << ProgName << " [options] file.vtf" << endl;
	op.PrintOptionHelp(stderr);

}

void ErrMsgCBHandler(const char *msg, int) {
	cerr << ProgName << " : " << msg << endl;
}

int	main(int argc, char **argv) {

	OptionParser op;

	ProgName = Basename(argv[0]);

	if (op.AppendOptions(set_opts) < 0) {
		cerr << ProgName << " : " << op.GetErrMsg();
		exit(1);
	}

	if (op.ParseOptions(&argc, argv, get_options) < 0) {
		cerr << ProgName << " : " << OptionParser::GetErrMsg();
		exit(1);
	}

	MyBase::SetErrMsgCB(ErrMsgCBHandler);


	if (opt.help) {
		Usage(op, NULL);
		exit(0);
	}

	if (argc != 2) {
		Usage(op, "Wrong number of arguments");
		exit(1);
	}


	TransferFunctionLite tf(8);
	if (MyBase::GetErrCode() !=0) exit(1);

	tf.setMinColorMapValue(0.0);
	tf.setMaxColorMapValue(1.0);
	tf.setMinOpacMapValue(0.0);
	tf.setMaxOpacMapValue(1.0);

	string vtffile(argv[1]);


	int rc;


	if (strlen(opt.cmap) != 0) {
		vector <float> pvec, hvec, svec, vvec;
		float point, h, s, v;

		ColorMapBase *cmap = tf.getColormap();
		cmap->clear();

		FILE *fp = fopen(opt.cmap, "r");
		if (! fp) {
			MyBase::SetErrMsg("fopen(%s) : %M", opt.cmap);
			exit(1);
		}

		const char *format = "%f %f %f %f";

		while ((rc = fscanf(fp, format, &point, &h, &s, &v)) == 4) { 
			pvec.push_back(point);
			hvec.push_back(h);
			svec.push_back(s);
			vvec.push_back(v);
		}


		vector <float> tmpvec = pvec;
		sort(tmpvec.begin(), tmpvec.end());
		tf.setMinColorMapValue(tmpvec[0]);
		tf.setMaxColorMapValue(tmpvec[tmpvec.size()-1]);

		ColorMapBase::Color color;
		for (int i=0; i<pvec.size(); i++) {
			color.hue(hvec[i]);
			color.sat(svec[i]);
			color.val(vvec[i]);
//			cmap->addNormControlPoint(pvec[i],  color);
			cmap->addControlPointAt(pvec[i],  color);
		}

		if (ferror(fp)) {
			MyBase::SetErrMsg("Error parsing file %s", opt.cmap);
			exit(1);
		}
		fclose(fp);
	}

	if (strlen(opt.omap) != 0) {
		float point, o;
		vector <float> pvec, ovec;

		OpacityMapBase *omap = tf.getOpacityMap(0);
		omap->clear();

		FILE *fp = fopen(opt.omap, "r");
		if (! fp) {
			MyBase::SetErrMsg("fopen(%s) : %M", opt.omap);
			exit(1);
		}

		const char *format = "%f %f";

		while ((rc = fscanf(fp, format, &point, &o)) == 2) { 
			pvec.push_back(point);
			ovec.push_back(o);
		}

		vector <float> tmpvec = pvec;
		sort(tmpvec.begin(), tmpvec.end());
		tf.setMinOpacMapValue(tmpvec[0]);
		tf.setMaxOpacMapValue(tmpvec[tmpvec.size()-1]);

		for (int i=0; i<pvec.size(); i++) {
//			omap->addNormControlPoint(pvec[i],  ovec[i]);
			omap->addControlPoint(pvec[i],  ovec[i]);
		}

		if (ferror(fp)) {
			MyBase::SetErrMsg("Error parsing file %s", opt.omap);
			exit(1);
		}
		fclose(fp);
	}

	ofstream fileout;
	fileout.open(vtffile.c_str());
	if (! fileout) {
		MyBase::SetErrMsg(
			"Can't open file \"%s\" for writing", vtffile.c_str()
		);
		exit(1);
	}

	if (! (tf.saveToFile(fileout))) exit(1);

	exit(0);
}
