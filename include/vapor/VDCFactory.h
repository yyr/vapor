//
// $Id$
//


#ifndef	_VDCFactory_h_
#define	_VDCFactory_h_

#include <vector>

#include <cstdio>
#include <vapor/MyBase.h>
#include <vapor/MetadataVDC.h>

namespace VAPoR {

//
//! \class VDCFactory
//! \brief Creates a MetadaVDC object
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//

class VDF_API VDCFactory : public VetsUtil::MyBase {
public:
 
 VDCFactory();
 int Parse(int *argv, char **argv);
 MetadataVDC *New(const size_t dims[3]) const;
 void Usage (FILE *fp) ;

private:
	VetsUtil::OptionParser _op;

	VetsUtil::OptionParser::Dimension3D_T _bs;
	bool _vdc2;
	int _level;
	int _nfilter;
	int _nlifting;
	std::vector <int> _cratios;
	string _wname;
    std::vector <string> _vars3d;
    std::vector <string> _vars2dxy;
    std::vector <string> _vars2dxz;
    std::vector <string> _vars2dyz;
	float _missing;
	string _comment;
	std::vector <int> _order;
	std::vector <int> _periodic;
	int _numts;
	double _startt;
	double _deltat;
	string _gridtype;
	std::string _usertimes;
	std::string _xcoords;
	std::string _ycoords;
	std::string _zcoords;
	std::string _mapprojection;
	std::string _coordsystem;
	std::vector <float> _extents;

	int _ReadDblVec(string path, vector <double> &vec) const;

};
};

#endif
