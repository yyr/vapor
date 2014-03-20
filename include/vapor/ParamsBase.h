//************************************************************************
//									*
//		     Copyright (C)  2008				*
//     University Corporation for Atmospheric Research			*
//		     All Rights Reserved				*
//									*
//************************************************************************/
//
//	File:		ParamsBase.h
//
//	Author:		John Clyne, modified by Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		March 2008
//
//	Description:	
//		Defines the ParamsBase class
//		This is an abstract class for classes that rely on 
//		accessing an XML node for get/set
//

#ifndef ParamsBase_H
#define ParamsBase_H

#include <vapor/XmlNode.h>
#include <vapor/ParamNode.h>
#include <vapor/ExpatParseMgr.h>
#include "assert.h"
#include <vapor/common.h>


using namespace VetsUtil;

namespace VAPoR{

class Params;
//
//! \class ParamsBase
//! \brief Nodes with state in Xml tree representation
//! \author John Clyne
//! \version 3.0
//! \date    March 2014
//!
//! This is abstract parent of Params and related classes with state 
//! kept in an xml node.  Used with the ParamNode class to support
//! user-defined Params classes as well as other classes such as
//! the TransferFunction class.
//! 
//! Implementers of VAPOR extensions may find it useful to implement
//! classes that are contained in Params classes, and whose XML
//! representation will then be a subtree of the root ParamNode of the Params class.
//! These classes should be derived from ParamsBase, but are in most aspects similar
//! to Params classes.  Examples of ParamsBase classes include TransferFunction, 
//! Box, and Viewpoint.
//! \sa Box, Viewpoint, TransferFunction
//!

class XmlNode;
class ParamNode;
class ParamsBase;

class PARAMS_API ParamsBase : public ParsedXml {
	

typedef ParamsBase* (*BaseCreateFcn)();

public: 
ParamsBase(
	XmlNode *parent, const string &name
 );
typedef int ParamsBaseType;
//! Default constructor 
ParamsBase(const string& name) {
	_currentParamNode = _rootParamNode = 0;
	_parseDepth = 0;
	_paramsBaseName = name;
}
//! Copy constructor.  
ParamsBase(const ParamsBase &pbase);
//! destructor..destroys the xml tree based at the root node.
virtual ~ParamsBase();

 //! Make a copy of a ParamBase that optionally uses specified 
 //! clone of the ParamNode as its root node.  If the root
 //! is null, the copy ignores any ParamNodes.  
 //! Derived classes must clone themselves, and then reassign
 //! newRoot to the clone root node, and also call SetParamsBase()
 //! on the new root to set its ParamsBase node to this. 
 //!
 //! \param[in] newRoot Root of cloned ParamsBase instance
 //! \retval instance Pointer to cloned instance
 //
 virtual ParamsBase* deepCopy(ParamNode* newRoot = 0) = 0;
 
 //! Set the parent node of the XmlNode tree.
 //!
 //! Sets a new parent node for the XmlNode tree parameter 
 //! data base. The parent node, \p parent, must have been
 //! previously initialized by passing it as an argument to
 //! the class constructor ParamBase(). This method permits
 //! wholesale changing of the entire parameter database.
 //!
 //! \param[in] parent Parent XmlNode.
 //
 void SetParent(XmlNode *parent);


 //! Xml start tag parsing method
 //!
 //! This method is called to handle parsing of an XML file. The contents
 //! of the file will replace any current parameter settings. The method
 //! is virtual so that derived classes may receive notification when
 //! an object instance is reseting state from an XML file
 //!
 //! Override this method if you are not using the ParamNode API to
 //! specify the state in terms of
 //! the xml representation of the class
 //! \sa elementEndHandler
 //! \retval status False indicates parse error
 //
 virtual bool elementStartHandler(
	ExpatParseMgr* pm, int depth, string& tag, const char ** attribs
 );

 //! Xml end tag parsing method
 //!
 //! This method is called to handle parsing of an XML file. The contents
 //! of the file will replace any current parameter settings. The method
 //! is virtual so that derived classes may receive notification when
 //! an object instance has finished reseting state from an XML file.
 //! Override the default method if the class is not based on
 //! a hierarchy of ParamNode objects representing 
 //! the XML representation of the class
 //! \sa elementStartHandler
 //! \retval status False indicates parse error
 //
 virtual bool elementEndHandler(ExpatParseMgr* pm, int depth, string& tag);

 //! Return the top (root) of the parameter node tree
 //!
 //! This method returns the top node in the parameter node tree
 //!

ParamNode *GetRootNode() { return(_rootParamNode); }

//!	
//! Method to build an xml node from state.
//! This only needs to be implemented if the state of the ParamsBase
//! is not specified by the root ParamNode 
//! \retval node ParamNode representing the current ParamsBase instance
//!

virtual ParamNode* buildNode(); 

//!	
//! Method for manual setting of node flags
//!

void SetFlagDirty(const string& flag);
//!	
//! Method for obtaining the name and/or tag associated with the instance
//!

const string& GetName() {return _paramsBaseName;}
 //!
 //! Obtain a single value of type long associated with a tag.
 //! Optionally specify a vector of longs whose first element is the default value
 //! if the value has not been assigned.
 //! \param[in] tag XML tag associated with the desired long parameter
 //! \param[in] defaultVal (optional) const vector<long> to be assigned if specified element does not exist.
 //! \retval long value associated with the named parameter
 //!
virtual long GetValueLong(const string& tag, const vector<long>& defaultVal = _emptyLongVec)
	{return GetRootNode()->GetElementLong(tag, defaultVal)[0];}
 //!
 //! Obtain a single value of type double associated with a tag.
 //! Optionally specify a vector of doubles whose first element is the default value
 //! if the value has not been assigned.
 //! \param[in] tag XML tag associated with the desired long parameter
 //! \param[in] defaultVal (optional) const vector<double> to be assigned if specified element does not exist.
 //! \retval long value associated with the named parameter
 //!
virtual double GetValueDouble(const string& tag,const vector<double>& defaultVal = _emptyDoubleVec)
	{return GetRootNode()->GetElementDouble(tag,defaultVal)[0];}
 //!
 //! Obtain a single value of type std::string associated with a tag.
 //! Optionally specify a string which will be the default value
 //! if the value has not been assigned.
 //! \param[in] tag XML tag associated with the desired string parameter
 //! \param[in] defaultVal (optional) const string to be assigned if specified element does not exist.
 //! \retval const string& value associated with the named parameter
 //!
virtual const string& GetValueString(const string& tag, const string& defaultVal = _emptyString)
	{return GetRootNode()->GetElementString(tag, defaultVal);}
 //!
 //! Obtain a vector of values of type long associated with a tag.
 //! Optionally specify a vector of longs as the default value
 //! if the value has not been assigned.
 //! \param[in] tag XML tag associated with the desired long parameter
 //! \param[in] defaultVal (optional) const vector<long> to be assigned if specified element does not exist.
 //! \retval const vector<long>& value associated with the named parameter
 //!
virtual const vector<long>& GetValueLongVec(const string& tag,const vector<long>& defaultVal = _emptyLongVec)
	{return GetRootNode()->GetElementLong(tag, defaultVal);}
 //!
 //! Obtain a vector of values of type double associated with a tag.
 //! Optionally specify a vector of doubles which is the default value
 //! if the value has not been assigned.
 //! \param[in] tag XML tag associated with the desired double parameter
 //! \param[in] defaultVal (optional) const vector<double>& to be assigned if specified element does not exist.
 //! \retval const vector<double>& value associated with the named parameter
 //!
virtual const vector<double>& GetValueDoubleVec(const string& tag, const vector<double>& defaultVal = _emptyDoubleVec)
	{return GetRootNode()->GetElementDouble(tag, defaultVal);}
 //!
 //! Obtain a vector of values of type string associated with a tag.
 //! Optionally specify a vector of string which is the default value
 //! if the value has not been assigned.
 //! \param[in] tag XML tag associated with the desired double parameter
 //! \param[out] vector<string>& vec string vector to which the strings will be assigned.
 //! \param[in] defaultVal (optional) const vector<double>& to be assigned if specified element does not exist.
 //!
virtual void GetValueStringVec(const string& tag, vector<string>& vec, const vector<string>& defaultVal = _emptyStringVec)
	{GetRootNode()->GetElementStringVec(tag, vec, defaultVal);}


//! Method for setting a single long value associated with a tag.
//! This will capture the state of the Params argument before the value is set, 
//! then set the values, then Validate(), and finally capture the state of
//! the Params after the Validate()
//! sa Validate()
//! \param [in] string tag
//! \param [char*] description
//! \param [in] long value
//! \param [in] Params* instance that contains the change; usually p = (Params*)this.
//! \retval int zero if successful
	virtual int SetValueLong(string tag, const char* description, long value, Params* p);

//! Method for setting long values associated with a tag. 
//! This will capture the state of the Params argument before the value is set, 
//! then set the values, then Validate(), and finally capture the state of
//! the Params after the Validate()
//! sa Validate()
//! \param [in] string tag
//! \param [in] char* description
//! \param [in] vector<long> value
//! \param [in] Params* instance that contains the change; usually p = (Params*)this.
//! \retval int zero if successful
	virtual int SetValueLong(string tag, const char* description, const vector<long>& value, Params* p);

//! Method for setting a single double value associated with a tag.
//! This will capture the state of the Params argument before the value is set, 
//! then set the values, then Validate(), and finally capture the state of
//! the Params after the Validate()
//! sa Validate()
//! \param [in] string tag
//! \param [in] char* description
//! \param [in] double value
//! \param [in] Params* instance that contains the change; usually p is the invoker of this method
//! \retval int zero if successful
	virtual int SetValueDouble(string tag, const char* description, double value, Params* p);

//! Method for making a Set of double values associated with a tag.
//! This will capture the state of the Params argument before the value is set, 
//! then set the values, then Validate(), and finally capture the state of
//! the Params after the Validate()
//! sa Validate()
//! \param [in] string tag
//! \param [in] char* description
//! \param [in] vector<double> value
//! \param [in] Params* instance that contains the change; usually p is the invoker of this method
//! \retval int zero if successful
	virtual int SetValueDouble(string tag, const char* description, const vector<double>& value, Params* p);

//! Method for making a Set of a single string value associated with a tag.
//! This will capture the state of the Params argument before the value is set, 
//! then set the value, then Validate(), and finally capture the state of
//! the Params after the Validate()
//! sa Validate()
//! \param [in] string tag
//! \param [in] char* description
//! \param [in] string value
//! \param [in] Params* instance that contains the change; usually p is the invoker
//! \retval int zero if successful
	virtual int SetValueString(string tag, const char* description, const string& value, Params* p);

//! Method for making a Set of string values associated with a tag.
//! This will capture the state of the Params argument before the value is set, 
//! then set the values, then Validate(), and finally capture the state of
//! the Params after the Validate()
//! sa Params::Validate()
//! \param [in] string tag
//! \param [in] char* description
//! \param [in] vector<string> value
//! \param [in] Params* instance that contains the change; usually p is the invoker
//! \retval int zero if successful
	virtual int SetValueStringVec(string tag, const char* description, const vector<string>& value, Params* p);

//!	
//! Method for obtaining the type Id associated with a ParamsBase instance
//! \retval int ParamsBase TypeID for ParamsBase instance 
//!

ParamsBaseType GetParamsBaseTypeId() {return GetTypeFromTag(_paramsBaseName);}

//!
//! Static method for converting a Tag to a ParamsBase typeID
//! \retval int ParamsBase TypeID for Tag
//!
static ParamsBaseType GetTypeFromTag(const string&tag);

//!
//! Static method for converting a ParamsBase typeID to a Tag
//! \retval string Tag (Name) associated with ParamsBase TypeID
//!
static const string& GetTagFromType(ParamsBaseType t);

//!
//! Static method for constructing a default instance of a ParamsBase 
//! class based on the typeId.
//! \param[in] pType TypeId of the ParamsBase instance to be created.
//! \retval instance newly created ParamsBase instance
//!
static ParamsBase* CreateDefaultParamsBase(int pType){
	ParamsBase *p = (createDefaultFcnMap[pType])();
	return p;
}
//!
//! Static method for constructing a default instance of a ParamsBase 
//! class based on the Tag.
//! \param[in] tag XML tag of the ParamsBase instance to be created.
//! \retval instance newly created ParamsBase instance
//!
static ParamsBase* CreateDefaultParamsBase(const string&tag);

//Methods for registration and tabulation of existing Params instances


//!
//! Static method for registering a ParamsBase class.
//! This calls CreateDefaultInstance on the class. 
//! \param[in] tag  Tag of class to be registered
//! \param[in] fcn  Method that creates default instance of ParamsBase class 
//! \param[in] isParams set true if the ParamsBase class is derived from Params
//! \retval classID Returns the ParamsBaseClassId, or 0 on failure 
//!
	static int RegisterParamsBaseClass(const string& tag, BaseCreateFcn fcn, bool isParams);
//!
//! Static method for registering a tag for an already registered ParamsBaseClass.
//! This is needed for backwards compatibility when a tag is changed.
//! The class must first be registered with the new tag.
//! \param[in] tag  Tag of class to be registered
//! \param[in] newtag  Previously registered tag (new name of class)
//! \param[in] isParams set true if the ParamsBase class is derived from Params
//! \retval classID Returns the ParamsBaseClassId, or 0 on failure 
//!
	static int ReregisterParamsBaseClass(const string& tag, const string& newtag, bool isParams);
//!
//! Specify the Root ParamNode of a ParamsBase instance 
//! \param[in] pn  ParamNode of new root 
//!
	virtual void SetRootParamNode(ParamNode* pn){_rootParamNode = pn;}
//!
//! Static method to determine how many Params classes are registered
//! \retval count Number of registered Params classes
//!
	static int GetNumParamsClasses() {return numParamsClasses;}
//!
//! Static method to determine if a ParamsBase class is a Params class
//! \param[in] tag XML tag associated with ParamsBase class
//! \retval status True if the specified class is a Params class
//!
	static bool IsParamsTag(const string&tag) {return (GetTypeFromTag(tag) > 0);}
#ifndef DOXYGEN_SKIP_THIS
	static ParamsBase* CreateDummyParamsBase(std::string tag);
	
	static void addDummyParamsBaseInstance(ParamsBase*const & pb ) {dummyParamsBaseInstances.push_back(pb);}

	static void clearDummyParamsBaseInstances();
#endif
private:
	//These should be accessed by subclasses through get() and set() methods
	ParamNode *_currentParamNode;
	ParamNode *_rootParamNode;
	

protected:
	static vector<ParamsBase*> dummyParamsBaseInstances;
	static const string _emptyString;
	static const vector<string> _emptyStringVec;
	static const vector<double> _emptyDoubleVec;
	static const vector<long> _emptyLongVec;
	virtual ParamNode *getCurrentParamNode() {return _currentParamNode;}
	
	virtual void setCurrentParamNode(ParamNode* pn){ _currentParamNode=pn;}
	
	
	
	static map<string,int> classIdFromTagMap;
	static map<int,string> tagFromClassIdMap;
	static map<int,BaseCreateFcn> createDefaultFcnMap;

	string _paramsBaseName;
	int _parseDepth;
	static int numParamsClasses;
	static int numEmbedClasses;


protected:

 //! Return the current node in the parameter node tree
 //!
 //! This method returns the current node in the parameter node tree. 
 //! \sa Push(), Pop()
 //! \retval node Current ParamNode
 //!
 ParamNode *GetCurrentNode() { return(_currentParamNode); }

 //! Move down a level in the parameter tree
 //!
 //! The underlying storage model for parameter data is a hierarchical tree. 
 //! By default the hierarchy is flat. This method can be used to add
 //! and navigate branches of the tree. Invoking this method makes the branch
 //! named by \p name the current branch (node). If the branch \p name does not
 //! exist it will be created with the name provided. 
 //! Subsequent set and get methods will operate
 //! relative to the current branch.
 //! User-specific subtrees can be provided by extending this method
 //!
 //! \param[in] tag The name of the branch
 //! \param[in] pBase optional ParamsBase object to be associated with ParamNode
 //! \retval node Returns the new current node
 //!
 //! \sa Pop(), Delete(), GetCurrentNode()
 //
 ParamNode *Push(
	 string& tag,
	 ParamsBase* pBase = 0
	);


 //! Move up one level in the paramter tree
 //!
 //! This method move back up the tree hierarchy by one level.
 //! Moving up past the root of the tree is prohibited and will silenty fail
 //! with no ill effects.
 //!
 //! \retval node Returns the new current node
 //!
 //! \sa Pop(), Delete()
 //
 ParamNode *Pop();

 //! Delete the named branch.
 //!
 //! This method deletes the named child, and all decendents, of the current 
 //! destroying it's contents in the process. The 
 //! named node must be a child of the current node. If the named node
 //! does not exist the result is a no-op.
 //!
 //! \param[in] name The name of the branch
 //
 void Remove(const string &name);

 //! Return the attributes associated with the current branch
 //!
 //! \retval map attribute mapping
 //
 const map <string, string> &GetAttributes();


 //! Remove (undefine) all parameters
 //!
 //! This method deletes any and all paramters contained in the base 
 //! class as well as deleting any tree branches.
 //
 void Clear();
};
#ifndef DOXYGEN_SKIP_THIS
//The DummyParamsBase is simply holding the parse information for
//A paramsBase extension class that is not present. This can only occur
//as a ParamsBase node inside a DummyParams node
class DummyParamsBase : public ParamsBase {
	public:
		DummyParamsBase(XmlNode *parent, const string &name) :
		  ParamsBase(parent, name) {}
		ParamsBase* DummyParamsBase::deepCopy(ParamNode* nd = 0);
	virtual ~DummyParamsBase(){}
};
#endif //DOXYGEN_SKIP_THIS
}; //End namespace VAPoR
#endif //ParamsBase_H
