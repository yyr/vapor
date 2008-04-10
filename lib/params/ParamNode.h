//
//      $Id$
//

#ifndef	_ParamNode_h_
#define	_ParamNode_h_

#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <vapor/MyBase.h>
#include <vapor/XmlNode.h>

namespace VAPoR {


//
//! \class ParamNode
//! \brief An Xml tree
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//! This class extends the XmlNode class, adding 
//! support for dirty flags.
//!
class PARAMS_API ParamNode : public XmlNode {
public:

 class DirtyFlag {
 public:
    DirtyFlag() {_status = true;}
    bool Test() const {return(_status);}
    void Set() {_status = true;}
    void Clear() {_status = false;}
 private:
    bool _status;
 };


 //! Constructor for the ParamNode class.
 //!
 //! Create's a new ParamNode node 
 //!
 //! \param[in] tag Name of ParamNode node
 //! \param[in] attrs A list of Xml attribute names and values for this node
 //! \param[in] numChildren Reserve space for the indicated number of 
 //! children. Children must be created with
 //! the NewChild() method
 //!
 ParamNode(
	const string &tag, const map<string, string> &attrs, 
	size_t numChildrenHint = 0
 );

 virtual ParamNode *Construct(
	const string &tag, const map<string, string> &attrs,
	size_t numChildrenHint = 0
 ) { return(new ParamNode(tag, attrs, numChildrenHint)); }

 //! Copy constructor for the ParamNode class.
 //!
 //! Create's a new ParamNode node from an existing one.
 //!
 //! \param[in] pn ParamNode instance from which to construct a copy
 //!
 ParamNode(ParamNode &pn);

 virtual ParamNode *Clone() {return new ParamNode(*this); };


 ~ParamNode();
 //! Set all the flags dirty (or clean)
 //!
 //! This method is useful if it is necessary to
 //! force all clients to refresh their state, e.g.
 //! if the ParamNode has been cloned from 
 //! another node.
 //
 void SetAllFlags(bool dirty);
	
 //! Set an ParamNode parameter of type long
 //!
 //! This method defines and sets a parameter of type long. The
 //! paramter data
 //! data to be associated with \p tag is the array of longs
 //! specified by \p values
 //!
 //! \param[in] tag Name of the element to define/set
 //! \param[in] values Vector of longs
 //!
 //! \retval status Returns a reference to vector containing the data
 //
 vector<long> &SetElementLong(
	const string &tag, const vector<long> &values
 );

 vector<double> &SetElementDouble(
	const string &tag, const vector<double> &values
 );

 string &SetElementString(const string &tag, const string &str);

 //! Set the dirty flag not necessarily associated with xml node.
 //! Has no effect if there the flag is not registered.
 //!
 //! \param[in] tag Name of the element to mark dirty
 //!
 
 void SetFlagDirty(const string &tag);

 //! Add an existing node as a child of the current node.
 //!
 //! The new child node will be
 //! appended to the array of child nodes.
 //!
 //! \note This method differs from the base class method that it
 //! overloads in that it prohibits siblings from having duplicate names.
 //!
 //! \param[in] child is the ParamNode object to be added as a child
 //! \retval status Return 0 upon success. A negative number is returned
 //! if a sibling already exists with the same name.
 //
 int AddChild(ParamNode* child);


 //! Return the indicated child node. 
 //!
 //! Return the ith child of this node. The first child node is index=0,
 //! the last is index=GetNumChildren()-1. Return NULL if the child 
 //! does not exist.
 //!
 //! \param[in] index Index of the child. The first child is zero
 //! \retval child Returns the indicated child, or NULL if the child
 //! could does not exist
 //! \sa GetNumChildren()
 //
 ParamNode *GetChild(size_t index) {return((ParamNode *) XmlNode::GetChild(index));}

 //! Return the indicated child node. 
 //!
 //! Return the indicated tagged child node. Return NULL if the child 
 //! does not exist.
 //! \param[in] tag Name of the child node to return
 //! \retval child Returns the indicated child, or NULL if the child
 //! could does not exist
 //
 ParamNode *GetChild(const string &tag) {return((ParamNode *) XmlNode::GetChild(tag));}

 //! Register a dirty flag with a named parameter of type long
 //!
 //! This method stores a pointer to the DirtyFlag \p df with
 //! the long parameter named by \p tag. The ParamNode::DirtyFlag::Set()
 //! method will be invoked whenever the associated parameter is 
 //! modified. 
 //!
 //! \note It is possible to register dirty flags for parameters 
 //! that do not exist. In which case they will not be set until
 //! the parameter is defined.
 //!
 //! \param[in] tag Name of ParamNode node
 //! \param[in] df A pointer to a dirty flag
 //!
 //! \sa RegisterDirtyFlagLong()
 //
 void RegisterDirtyFlagLong(const string &tag, ParamNode::DirtyFlag *df);

 //! Unregister a dirty flag associated with the named parameter of type long
 //!
 //! This method unregisters the DirtyFlag \p df associated with
 //! the long parameter named by \p tag, previously registered
 //! with RegisterDirtyFlagLong(). It is imperative that DirtyFlags pointers 
 //! are unregistered if the objects they refer to are invalidated (deleted).
 //!
 //! \param[in] tag Name of ParamNode node
 //! \param[in] df A pointer to a dirty flag
 //!
 //! \sa UnRegisterDirtyFlagLong()
 //
 void UnRegisterDirtyFlagLong(
	const string &tag, const ParamNode::DirtyFlag *df
 );

 void RegisterDirtyFlagDouble(const string &tag, ParamNode::DirtyFlag *df);
 void UnRegisterDirtyFlagDouble(
	const string &tag, const ParamNode::DirtyFlag *df
 );

 void RegisterDirtyFlagString(const string &tag, ParamNode::DirtyFlag *df);
 void UnRegisterDirtyFlagString(
	const string &tag, const ParamNode::DirtyFlag *df
 );

 //! Register a dirty flag with a subnode of the tree
 //!
 //! This method stores a pointer to the DirtyFlag \p df with
 //! the long parameter named by \p tag. The ParamNode::DirtyFlag::Set()
 //! method will be invoked whenever the associated node is 
 //! modified, 
 //!
 //! \param[in] tag Name of ParamNode node
 //! \param[in] df A pointer to a dirty flag
 //!
 //! \sa RegisterDirtyFlagNode()
 //
 void RegisterDirtyFlagNode(const string &tag, ParamNode::DirtyFlag *df);

 //! Unregister a dirty flag associated with the named node
 //!
 //! This method unregisters the DirtyFlag \p df associated with
 //! the parameter subtree named by \p tag, previously registered
 //! with RegisterDirtyFlagNode(). It is imperative that DirtyFlags pointers 
 //! are unregistered if the objects they refer to are invalidated (deleted).
 //!
 //! \param[in] tag Name of ParamNode node of subtree
 //! \param[in] df A pointer to a dirty flag
 //!
 //! \sa UnRegisterDirtyFlagNode()
 //
 void UnRegisterDirtyFlagNode(
	const string &tag, const ParamNode::DirtyFlag *df
 );

protected:
 map <string, vector <DirtyFlag *> > _dirtyLongFlags;
 map <string, vector <DirtyFlag *> > _dirtyDoubleFlags;
 map <string, vector <DirtyFlag *> > _dirtyStringFlags;
 map <string, vector <DirtyFlag *> > _dirtyNodeFlags;

};

};

#endif	//	_ParamNode_h_
