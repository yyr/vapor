//
// $Id$
//

#ifndef	_NetCDFSimple_h_
#define	_NetCDFSimple_h_

#include <vector>

#include <sstream>
#include <vapor/MyBase.h>

namespace VAPoR {

//
//! \class NetCDFSimple
//! \brief NetCDFSimple API interface
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//! This class presents a simplified interface for reading netCDF 
//! data files.
//!
//! The specification of dimensions and coordinates in this class
//! follows the netCDF API convention of ordering from slowest
//! varying dimension to fastest varying dimension. For example, if 
//! 'dims' is a vector of dimensions, then dims[0] is the slowest varying
//! dimension, dim[1] is the next slowest, and so on. This ordering is the 
//! opposite of the ordering used by most of the VAPoR API.
//
class VDF_API NetCDFSimple : public VetsUtil::MyBase {
public:
 NetCDFSimple();

 //! \class Variable
 //! \brief NetCDFSimple API interface
 //! 
 //! A NetCDFSimple data variable
 //
 class Variable {
 public:
	Variable();

	//! Constructor for NetCDFSimple::Variable class
	//!
	//! \param[in] varname	Name of the netCDF variable
	//! \param[in] dimnames A vector dimension names, ordered from
	//! slowest-varying to fastest.
	//! \param[in] dim A vector of dimensions, ordered from
    //! slowest-varying to fastest.
	//! \param[in] varid The netCDF variable ID
	//! \param[in] type The netCDF external data type for the variable
	Variable(
		string varname, std::vector <string> dimnames, 
		std::vector <size_t> dims, int varid, int type
	);

	//! Return the variable's name
	//
	string GetName() const {return (_name);};

	//! Return variable's attribute names
	//!
	//! This method returns a vector containing the names of all of the 
	//! attributes associated with this variable
	//
	std::vector <string> GetAttNames() const;

	//! Return variable's dimension names
	//!
	//! Returns an ordered list of the variable's netCDF dimension names.
	//! 
	std::vector <string> GetDimNames() const {return(_dimnames); };

	//! Return variable's dimensions
	//!
	//! Returns an ordered list of the variable's netCDF dimensions.
	//! 
	std::vector <size_t> GetDims() const {return(_dims); };

	//! Return the netCDF external data type for an attribute
	//!
	//! Returns the nc_type of the named variable attribute.
	//! \param[in] name Name of the attribute
	//!
	//! \retval If an attribute named by \p name does not exist, a 
	//! negative value is returned.
	//
	int GetAttType(string name) const;

	//! Return the netCDF variable ID for this variable
	//
	int GetVarID() const {return(_varid); };

	//! Return attribute values for attribute of type float
	//!
	//! Return the values of the named attribute converted to type float. 
	//!
	//!	\note Attributes of type int are cast to float
	//!
	//! \note All attributes with floating point representation of 
	//! any precision are returned by this method. Attributes that
	//! do not have floating point internal representations can not
	//! be returned
	//!
	//! \param[in] name Name of the attribute
	//! \param[out] values A vector of attribute values
	//
	void GetAtt(string name, std::vector <double> &values) const;
	void GetAtt(string name, std::vector <long long> &values) const;
	void GetAtt(string name, string &values) const;

	//! Set an attribute
	//!
	//! Set the floating point attribute, \p name, to the values
	//! given by \p values
	//
	void SetAtt(string name, const std::vector <double> &values) {
		_flt_atts.push_back(make_pair(name, values));
	}
	void SetAtt(string name, const std::vector <long long> &values) {
		_int_atts.push_back(make_pair(name, values));
	}
	void SetAtt(string name, const string &values) {
		_str_atts.push_back(make_pair(name, values));
	}

	//! Returns true if two variables have the same name, netCDF data
	//! type, and shape. Two variables need not have the same
	//! dimension names to match.
	//!
	//! \param[in] v Variable to compare against this instance
	//
	bool Match(const NetCDFSimple::Variable &v) const;


	VDF_API friend std::ostream &operator<<(std::ostream &o, const Variable &var);

	
 private:
	string _name;	// variable name
	std::vector <string> _dimnames;	// order list of dimension names
	std::vector <size_t> _dims;	// order list of dimensions 
	std::vector <std::pair <string, std::vector <double> > > _flt_atts;
	std::vector <std::pair <string, std::vector <long long> > > _int_atts;
	std::vector <std::pair <string, string> > _str_atts;
	int _type;	// netCDF variable type
	int _varid;	// netCDF variable id
 };

 //! Initialize the class instance for a netCDF file
 //!
 //! This method initializes (or reinitializes) a class instance with
 //! the name of a netCDF file
 //!
 //! \param[in] path Path to the netCDF file
 //!
 //! \retval status A negative int is returned on failure
 //!
 int Initialize(string path);

 //! Open the named variable for reading
 //!
 //! This method prepares a netCDF variable
 //! for subsequent read operations by
 //! methods of this class.
 //!
 //! \param[in] variable A variable object returned by GetVariables()
 //!
 //! \retval status Returns a non-negative value on success
 //!
 //! \sa Read(), GetVariables()
 //!
 int OpenRead(const NetCDFSimple::Variable &variable);

 //! Read an array of values from a variable
 //!
 //! The method allows the readying of a hyperslab of data from the
 //! currently opened variable. 
 //!
 //! \param[in] start Start vector with one element for each dimension to
 //! specify a hyperslab
 //! \param[in] count Count vector with one element for each dimension to 
 //! specify a Hyperslab
 //! \param[out] data A pointer to an area of memory containing sufficent
 //! space to contain the copied hyperslab.
 //!
 //! \retval status A negative int is returned on failure
 //!
 //! \sa OpenRead()
 //! \sa NetCDF documentation for nc_get_vara
 //!
 int Read(const size_t start[], const size_t count[], float *data) const;
 int Read(const size_t start[], const size_t count[], int *data) const;

 //! Close the currently opened variable
 //!
 //! \retval status Returns a non-negative value on success
 //
 int Close();

 //! Return a vector of the Variables contained in the file
 //!
 //! This method returns a vector of Variable objects containing
 //! the metadata for each variable in the netCDF file
 //!
 //! \sa Initialize(), NetCDFSimple::Variable
 //!
 const std::vector <NetCDFSimple::Variable> &GetVariables() const {
	return(_variables);
 };

 //! Return all dimensions and dimension names defined in the file
 //!
 //! This method returns in \p names a vector of all dimension names,
 //! and in \p dims a vector of all dimension lengths. Thus, for example,
 //! the lenght of the dimension named by names[i] is given by dims[i]
 //!
 //! \params[out] names Vector of dimension names
 //! \params[out] dims Vector of dimension lengths
 //!
 //! \sa Initialize()
 //!
 void GetDimensions(std::vector <string> &names, std::vector <size_t> &dims) const;

 //! Return the name of the netCDF dimension with a given dimension id
 //!
 //! Returns the name of dimension for the netCDF dimension id specified by
 //! \p id
 //!
 //! \param[in] id a valid netCDF dimension ID for the current file
 //! \retval name Name of the dimension associated with the identifier \p id.
 //! if \p id is invalid an empty string is returned.
 //!
 //! \sa Initialize()
 //
 string DimName(int id) const;

 //! Return the netCDF dimension id for a named dimension
 //!
 //! Returns the netCDF dimension id for the named dimension specified by
 //! \p name
 //!
 //! \param[in] name a valid netCDF dimension name for the current file
 //! \retval id netCDF identifier for the dimension \p name.
 //! if \p name is invalid a negative int is returned.
 //!
 //! \sa Initialize()
 //
 int DimId(string name) const;

 //! Return global attribute names
 //! 
 //! This method returns a vector of all the global netCDF attributes 
 //! defined in the file
 //!
 //! \retval vector A list of global attribute names
 //!
 //! \sa Initialize()
 //
 std::vector <string> GetAttNames() const;

 VDF_API friend std::ostream &operator<<(std::ostream &o, const NetCDFSimple &nc);

private:
 int _ovr_ncid;
 int _ovr_varid;
 string _path;
 size_t _chsz;
 std::vector <string> _dimnames;
 std::vector <size_t> _dims;
 std::vector <string> _unlimited_dimnames;
 std::vector <std::pair <string, std::vector <double> > > _flt_atts;
 std::vector <std::pair <string, std::vector <long long> > > _int_atts;
 std::vector <std::pair <string, string> > _str_atts;
 std::vector <NetCDFSimple::Variable> _variables;

 int _GetAtts(
	int ncid, int varid,
	std::vector <std::pair <string, std::vector <double> > > &flt_atts,
	std::vector <std::pair <string, std::vector <long long> > > &int_atts,
	std::vector <std::pair <string, string> > &str_atts
 ); 

};

};
#endif
