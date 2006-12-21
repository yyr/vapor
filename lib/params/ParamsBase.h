
//
//      $Id$
//
//***********************************************************************
//                                                                      *
//                      Copyright (C)  2006	                        *
//          University Corporation for Atmospheric Research             *
//                      All Rights Reserved                             *
//                                                                      *
//***********************************************************************
//
//	File:		ParamsBase.h
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Tue Dec 5 14:23:51 MST 2006
//
//	Description:	
//
//

#ifndef _ParamsBase_h_
#define _ParamsBase_h_


#include "vapor/XmlNode.h"
#include "vapor/ExpatParseMgr.h"
#include "ParamNode.h"

using namespace VetsUtil;

namespace VAPoR{

//
//! \class ParamBase
//! \brief A base class for managing (storing and retrieving) 
//! simple parameters.
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//! This base class can be used to derive feature-specific parameter 
//! management classes. Parameters (named arrays of longs, doubles
//! or string) managed by this class are stored in a XmlNode tree. The base
//! class handles storage and restoration of parameters to/from disk
//! on behalf of derived classes (provided of course derived classes use 
//! base class methods for setting and getting parameters).
//! 
class PARAMS_API ParamsBase : public VetsUtil::MyBase, public ParsedXml {

	
public: 


 //! Create new ParmsBase abstract base class.
 //!
 //! Create a ParamsBase abstract base class from scratch. All
 //! state information will be maintained as an XmlNode tree. The
 //! XmlTree will be made a child of the parent node, \p parent.
 //! The parent node, \p parent, must be a leaf node (have no
 //! children).
 //!
 //! \param[in] parent Parent XmlNode.
 //! \param[in] name The name of the instance of the derived class
 //
 ParamsBase(
	XmlNode *parent, const string &name
 );

 
 //! Destroy object
 //!
 //! \note This destructor does not delete children XmlNodes created
 //! as children of the \p parent constructor parameter.
 //!
 virtual ~ParamsBase();

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
 //
 virtual bool elementStartHandler(
	ExpatParseMgr* pm, int depth, string& tag, const char ** attribs
 );

 //! Xml end tag parsing method
 //!
 //! This method is called to handle parsing of an XML file. The contents
 //! of the file will replace any current parameter settings. The method
 //! is virtual so that derived classes may receive notification when
 //! an object instance has finished reseting state from an XML file
 //
 virtual bool elementEndHandler(ExpatParseMgr* pm, int depth, string& tag);

 //! Return the top (root) of the parameter node tree
 //!
 //! This method returns the top node in the parameter node tree
 //!
 ParamNode *GetRootNode() { return(_rootParamNode); }

protected:



 //! Return the current node in the parameter node tree
 //!
 //! This method returns the current node in the parameter node tree. 
 //! \sa Push(), Pop()
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
 //!
 //! \param[in] name The name of the branch
 //! \retval node Returns the new current node
 //!
 //! \sa Pop(), Delete(), GetCurrentNode()
 //
 ParamNode *Push(const string &name);


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
 //! does not exit the result is a no-op.
 //!
 //! \param[in] name The name of the branch
 //
 void Remove(const string &name);

 //! Return the attributes associated with the current branch
 //!
 //! \retval a list of attributes
 //
 const map <string, string> &GetAttributes();

 //! Reset parameter state to the default
 //!
 //! This pure virtual method must be implented by the derived class to
 //! restore state to the default
 //
 virtual void ResetDefault() = 0;

 //! Remove (undefine) all parameters
 //!
 //! This method deletes any and all paramters contained in the base 
 //! class as well as deleting any tree branches.
 //
 void Clear();

private:

 ParamNode *_currentParamNode;
 ParamNode *_rootParamNode;

 int _parseDepth;

 
};

};

#endif	// _ParamsBase_h_
