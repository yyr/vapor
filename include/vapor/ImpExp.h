//
//      $Id$
//


#ifndef	_ImpExp_h_
#define	_ImpExp_h_

#include <stack>
#include <expat.h>
#include <vapor/MyBase.h>
#include <vaporinternal/common.h>
#include <vapor/XmlNode.h>

namespace VAPoR {


//
//! \class ImpExp
//! \brief A class for managing data set metadata
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//! The ImpExp class is used to import/export state to/from a VAPoR session
//!
class VDF_API ImpExp : public VetsUtil::MyBase {
public:

 //! Create an Import Export object
 //!
 //
 ImpExp();


 ~ImpExp();


 //! Export a volume subregion description for use by another application
 //!
 //! This method exports data set state information to facilitate sharing
 //! with another application. In particular, the path name of the VDF file,
 //! a volume time step, a named volume variable, and volumetric region 
 //! of interest are exported. The exported information may be retreive
 //! via the Import() method
 //!
 //! \note The region bounds are specified relative to the finest 
 //! resolution volume
 //!
 //! \note Presently the medium of exchange is an XML file, the path to 
 //! which is internally hardwired and based on the user's uid. 
 //! Thus it is not possible for applications running under different uid's
 //! to share data. Furthermore, there exists only one XML file per uid
 //! on a system. This will undoubtedly change in the future.
 //!
 //! \param[in] path Path to the metadata file
 //! \param[in] ts Time step of exported volume
 //! \param[in] varname Variable name of exported volume
 //! \param[in] min Minimum region extents in voxel coordinates relative
 //! to the \b finest resolution 
 //! \param[in] max Maximum region extents in voxel coordinates relative
 //! to the \b finest resolution 
 //! \param[in] timeseg Time segment range
 //! \retval status Returns a non-negative integer on success
 //! \sa Import()
 //
 int Export(
	const string &path, size_t ts, const string &varname, 
	const size_t min[3], const size_t max[3], const size_t timeseg[2]
 );

 //! Import a volume subregion description for use by another application
 //!
 //! This method imports data set state information to facilitate sharing
 //! with another application. In particular, the path name of the VDF file,
 //! a volume time step, a named volume variable, and volumetric region 
 //! of interest are imported. The imported information is assumed to 
 //! have been generated via the the most recent call to the Export() method.
 //!
 //! \param[out] path Path to the metadata file
 //! \param[out] ts Time step of exported volume
 //! \param[out] varname Variable name of exported volume
 //! \param[out] min Minimum region extents in voxel coordinates relative
 //! to the \b finest resolution 
 //! \param[out] max Maximum region extents in voxel coordinates relative
 //! to the \b finest resolution 
 //! \param[out] timeseg Time segment range
 //! \retval status Returns a non-negative integer on success
 //! \sa Export()
 //
 int Import(
	string *path, size_t *ts, string *varname, 
	size_t min[3], size_t max[3], size_t timeseg[2]
 );

private:
 XmlNode	*_rootnode;		// root node of the xml tree

 XML_Parser _expatParser;	// XML Expat parser handle
 int	_xmlState;


 // Structure used for parsing metadata files
 //
 class VDF_API _expatStackElement {
	public:
	string tag;         // xml element tag
	string data_type;   // Type of element data (string, double, or long)
	int has_data;       // does the element have data?
 };
 stack  <VDF_API _expatStackElement *> _expatStateStack;

 string _expatStringData;	// temp storage for XML element character data
 vector <long> _expatLongData;	// temp storage for XML long data
 vector <double> _expatDoubleData;	// temp storage for XML double  data


 static const string _rootTag;
 static const string _pathNameTag;
 static const string _timeStepTag;
 static const string _varNameTag;
 static const string _regionTag;
 static const string _timeSegmentTag;

 // known xml attribute values
 //
 static const string _stringType;
 static const string _longType;
 static const string _doubleType;


 // known xml attribute names
 //
 static const string _typeAttr;

 // XML Expat element handlers
 friend void	impExpStartElementHandler(
	void *userData, const XML_Char *tag, const char **attrs
 ) {
	ImpExp *meta = (ImpExp *) userData;
	meta->_startElementHandler(tag, attrs);
 }


 friend void impExpEndElementHandler(void *userData, const XML_Char *tag) {
	ImpExp *meta = (ImpExp *) userData;
	meta->_endElementHandler(tag);
 }

 friend void	impExpCharDataHandler(
	void *userData, const XML_Char *s, int len
 ) {
	ImpExp *meta = (ImpExp *) userData;
	meta->_charDataHandler(s, len);
 }

 void _startElementHandler(const XML_Char *tag, const char **attrs);
 void _endElementHandler(const XML_Char *tag);
 void _charDataHandler(const XML_Char *s, int len);

 // XML Expat element handler helps. A different handler is defined
 // for each possible state (depth of XML tree) from 0 to 3
 //
 void _startElementHandler0(const string &tag, const char **attrs);
 void _startElementHandler1(const string &tag, const char **attrs);
 void _endElementHandler0(const string &tag);
 void _endElementHandler1(const string &tag);


 // Report an XML parsing error
 //
 void _parseError(const char *format, ...);

 string _getpath();


};


};

#endif	//	_ImpExp_h_
