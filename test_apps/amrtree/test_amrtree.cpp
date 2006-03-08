#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdio>

#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/AMRTree.h>

using namespace VetsUtil;
using namespace VAPoR;


struct {
	OptionParser::Boolean_T	help;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"help",	0,	"",	"Print this message and exit"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{NULL}
};

const char	*ProgName;


main(int argc, char **argv) {

	OptionParser op;
	AMRTree	*tree;


	MyBase::SetDiagMsgFilePtr(stderr);
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
		cerr << "Usage: " << ProgName << " " << endl;
		op.PrintOptionHelp(stderr);
		exit(0);
	}

	if (argv[1]) {
		tree = new AMRTree(argv[1]);
		if (AMRTree::GetErrCode() != 0) {
			fprintf(stderr, "AMRTree() : %s\n", AMRTree::GetErrMsg());
			exit(1);
		}
	}
	else {

		//size_t basedim[3] = {4,4,4};
		size_t basedim[3] = {1,1,1};
		double min[3] = {4.0,4.0,4.0};
		double max[3] = {8.0,8.0,8.0};

		tree = new AMRTree(basedim, min, max);
		if (AMRTree::GetErrCode() != 0) {
			fprintf(stderr, "AMRTree() : %s\n", AMRTree::GetErrMsg());
			exit(1);
		}


		tree->RefineCell(0);

		AMRTree::CellID cell0 = tree->GetCellChildren(0);
		tree->RefineCell(cell0);
		tree->RefineCell(cell0+1);
		tree->RefineCell(cell0+2);
		tree->RefineCell(cell0+3);

		AMRTree::CellID cell7 = tree->GetCellChildren(cell0) + 3;
		AMRTree::CellID cell10 = tree->GetCellChildren(cell0+1) + 2;
		AMRTree::CellID cell13 = tree->GetCellChildren(cell0+2) + 1;
		AMRTree::CellID cell16 = tree->GetCellChildren(cell0+3) + 0;

		tree->RefineCell(cell7);
		tree->RefineCell(cell10);
		tree->RefineCell(cell10+1);
		tree->RefineCell(cell13);
		tree->RefineCell(cell13+2);
		tree->RefineCell(cell16);
		tree->RefineCell(cell16+1);
		tree->RefineCell(cell16+3);

		AMRTree::CellID cell23 = tree->GetCellChildren(cell7) + 3;
		AMRTree::CellID cell37 = tree->GetCellChildren(cell13+2) + 1;
		AMRTree::CellID cell44 = tree->GetCellChildren(cell16+1);
		AMRTree::CellID cell48 = tree->GetCellChildren(cell16+3);
		

		tree->RefineCell(cell23);
		tree->RefineCell(cell37);
		tree->RefineCell(cell44);
		tree->RefineCell(cell48);

	}

	tree->Write("tree.xml");

	AMRTree::CellID cellid;

	double ucoord[3] = {4.5, 4.5, 4.5};
	cellid = tree->FindCell(ucoord);
	if (cellid < 0) {
		fprintf(stderr, "AMRTree::FindCell() : %s\n", AMRTree::GetErrMsg());
		exit(1);
	}
	fprintf(
		stdout, "Found cell %d at location [%f %f %f]\n", 
		cellid, ucoord[0], ucoord[1], ucoord[2]
	);

		

	exit(0);
}

