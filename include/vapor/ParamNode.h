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

class ParamsBase;
//
//! \class ParamNode 
//! \ingroup Public_Params
//! \brief An Xml tree
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//! This class extends the XmlNode class. The XML hierarchy includes
//! pointers to ParamsBase instances, enabling use of
//! classes embedded in Params instances.
//!
class PARAMS_API ParamNode : public XmlNode {
public:

//! @name Internal
//! Internal methods not intended for general use
///@{

 //! Constructor for the ParamNode class.
 //!
 //! Create a new ParamNode node 
 //!
 //! \param[in] tag Name of ParamNode node
 //! \param[in] attrs A list of Xml attribute names and values for this node
 //! \param[in] numChildrenHint Reserve space for the indicated number of 
 //! children. Children must be created with
 //! the NewChild() method
 //!
 ParamNode(
	const string &tag, const map<string, string> &attrs, 
	size_t numChildrenHint = 0
 );

  ParamNode(
	const string &tag, 
	size_t numChildrenHint = 0
	);

 //! Construct a new ParamNode node 
 //!
 //! \param[in] tag Name of ParamNode node
 //! \param[in] attrs A list of Xml attribute names and values for this node
 //! \param[in] numChildrenHint Reserve space for the indicated number of 
 //! \retval node Newly constructed ParamNode
 //!
 virtual ParamNode *Construct(
	const string &tag, const map<string, string> &attrs,
	size_t numChildrenHint = 0
 ) { return(new ParamNode(tag, attrs, numChildrenHint)); }

 //! Copy constructor for the ParamNode class.
 //!
 //! Creates a new ParamNode node from an existing one.
 //!
 //! \param[in] pn ParamNode instance from which to construct a copy
 //!
 ParamNode(const ParamNode &pn);

 //! Method that clones the ParamNode structure, using buildNode to
 //! construct the ParamNodes associated with registered ParamsBase
 //! instances in the ParamNode hierarchy
 ParamNode* NodeCopy();

//! Copy only the ParamNode itself, not any of its children.
 ParamNode* ShallowCopy();
 
 //! Like copy constructor for the ParamNode class, but
 //! in addition to cloning the child nodes in xml hierarchy,
 //! also clones the ParamsBase instances that are referenced
 //! by child nodes
 //!
 //! Creates a new ParamNode node from an existing one.
 //!
 //! \retval node ParamNode copied from this
 //!
 virtual ParamNode* deepCopy() ;

 virtual ~ParamNode();

	
 ///@}

 //! Set a single ParamNode parameter of type double
 //!
 //! This method defines and sets a parameter of type double. The
 //! parameter
 //! data to be associated with \p tag is the single double
 //! specified by \p value
 //!
 //! \param[in] tag Name of the element to define/set
 //! \param[in] value double
 //!
 //! \retval status Returns 0 if successful
 //
 int SetElementDouble(
	const string &tag, double value
 );
 //! Set an ParamNode parameter of type double
 //!
 //! This method defines and sets a parameter of type double. The
 //! parameter data
 //! data to be associated with \p tag is the array of double
 //! specified by \p values
 //!
 //! \param[in] tag Name of the element to define/set
 //! \param[in] values Vector of doubles
 //!
 //! \retval status Returns 0 if successful
 //
 int SetElementDouble(
	const string &tag, const vector<double> &values
 );
//! Set an ParamNode parameter of type double
 //!
 //! This method defines and sets a parameter of type double. The
 //! parameter data
 //! data to be associated with \p tagpath is the array of double
 //! specified by \p values
 //!
 //! \param[in] tagpath Names of the nodes leading to the element to be set
 //! \param[in] values Vector of doubles
 //!
 //! \retval status Returns 0 if successful
 //
 int SetElementDouble(
	const vector<string> &tagpath, const vector<double> &values
 );

 //! Get an element's data of type double
 //!
 //! Return the character data associated with the Xml element 
 //! identified by a sequence \p tagpath from this node. The data is interpreted and 
 //! returned as a vector of doubles. If the element does not exist
 //! an empty vector is returned. If ErrOnMissing() is true an 
 //! error is generated if the element is missing;
 //!
 //! \param[in] tagpath sequence of tags leading to element 
 //! \param[in] defaultVal (optional) default vector<double> to be assigned if specified element does not exist
 //! \retval vector Vector of doubles associated with the named element
 //!
 virtual const vector<double> GetElementDouble(const vector<string> &tagpath, const vector<double>& defaultVal = XmlNode::_emptyDoubleVec );

 //! Get an Xml element's data of type double
 //!
 //! Return the character data associated with the Xml elemented 
 //! named by \p tag for this node. The data is interpreted and 
 //! returned as a vector of doubles. If the element does not exist
 //! an empty vector is returned. If ErrOnMissing() is true an 
 //! error is generated if the element is missing;
 //!
 //! \param[in] tag Name of element
 //! \param[in] defaultVal (optional) default vector<double> to be assigned if specified element does not exist
 //! \retval vector Vector of doubles associated with the named element
 //!
 virtual const vector<double> GetElementDouble(const string &tag, const vector<double>& defaultVal = XmlNode::_emptyDoubleVec); 
 
 int SetElementLong(
	const string &tag, const vector<long> &values
 );
 //! Set a single ParamNode parameter of type long
 //!
 //! This method defines and sets a parameter of type long. The
 //! paramter data
 //! data to be associated with \p tag is the single long
 //! specified by \p value
 //!
 //! \param[in] tag Name of the element to define/set
 //! \param[in] value long
 //!
 //! \retval status Returns 0 if successful
 //
 int SetElementLong(
	const string &tag, long value
 );
//! Set an ParamNode parameter of type long
 //!
 //! This method defines and sets a parameter of type long. The
 //! parameter data
 //! data to be associated with \p tagpath is the array of longs
 //! specified by \p values
 //!
 //! \param[in] tagpath Names of nodes leading to the value to be set
 //! \param[in] values Vector of longs
 //!
 //! \retval status Returns 0 if successful
 //
 int SetElementLong(
	const vector<string> &tagpath, const vector<long> &values
 );
 //! Get an Xml element's data of type long
 //!
 //! Return the character data associated with the XML element 
 //! reached via a sequence \p tagpath of nodes from this node. 
 //! The data is interpreted and 
 //! returned as a vector of longs. If the element does not exist
 //! an empty vector is returned. If ErrOnMissing() is true an 
 //! error is generated if the element is missing;
 //!
 //! \param[in] tagpath Sequence of tags to element
 //! \param[in] defaultVal (optional) vector<long> to be assigned if specified element does not exist.
 //! \retval vector<long> vector of longs associated with the named elemented
 //!
 virtual const vector<long> GetElementLong(const vector<string> &tagpath, const vector<long>& defaultVal = XmlNode::_emptyLongVec);
 //! Get an Xml element's data of type long
 //!
 //! Return the character data associated with the Xml element
 //! named by \p tag for this node. The data is interpreted and 
 //! returned as a vector of longs. If the element does not exist
 //! an empty vector is returned. If ErrOnMissing() is true an 
 //! error is generated if the element is missing;
 //!
 //! \param[in] tag Name of element
 //! \param[in] defaultVal (optional) Vector of longs that will be set if specified element does not exist
 //! \retval vector Vector of longs associated with the named elemented
 //!
 virtual const vector<long> GetElementLong(const string &tag, const vector<long>& defaultVal= XmlNode::_emptyLongVec);


 //! Set a single ParamNode parameter of type string
 //!
 //! This method defines and sets a parameter of type string. The
 //! parameter data
 //! data to be associated with \p tag is the single string
 //! specified by \p value
 //!
 //! \param[in] tag Name of the element to define/set
 //! \param[in] value string
 //!
 //! \retval status Returns 0 if successful
 //
 int SetElementString(
	const string &tag, const string &value
 );
//! Set a single ParamNode parameter of type string
 //!
 //! This method defines and sets a parameter of type string. The
 //! parameter data
 //! data to be associated with \p tagpath is the single string
 //! specified by \p value
 //!
 //! \param[in] tagpath sequence of tags leading to specified element.
 //! \param[in] value string
 //!
 //! \retval status Returns 0 if successful
 //
 int SetElementString(
	const vector<string> &tagpath, const string &value
 );
 //! Set an ParamNode parameter of type string vector
 //!
 //! This method defines and sets a parameter of type string vector. The
 //! parameter data
 //! to be associated with \p tag is the array of strings
 //! specified by \p values
 //! The strings in the vector \p values must not contain white characters.
 //!
 //! \param[in] tag Name(Tag) of the element to define/set
 //! \param[in] values Vector of strings
 //!
 //! \retval status Returns 0 if successful
 //
 int SetElementStringVec(const string &tag, const vector<string> &values);
  //! Set a ParamNode parameter of type string
 //!
 //! This method defines and sets a parameter of type string vector. The
 //! parameter data
 //! data to be associated with \p tagpath is the array of strings
 //! specified by \p values
 //! The strings in the vector \p values must not contain white characters.
 //!
 //! \param[in] tagpath Names of nodes leading to value to be set
 //! \param[in] values Vector of strings
 //!
 //! \retval status Returns 0 if successful
 //
 int SetElementStringVec(const vector<string> &tagpath, const vector<string> &values);

 //
 //! Get an Xml element's data of type string
 //!
 //! Return the character data associated with the Xml elemented 
 //! named by \p tag for this node. The data is interpreted and 
 //! returned as a string. If the element does not exist
 //! an empty vector is returned. If ErrOnMissing() is true an 
 //! error is generated if the element is missing;
 //!
 //! \param[in] tag Name of element
 //! \param[in] defaultVal (optional) string to be assigned if specified element does not exist.
 //! \retval string The string associated with the named element
 //!
 virtual const string GetElementString(const string &tag, const string& defaultVal = XmlNode::_emptyString); 
 
 //! Get an element's data of type string
 //!
 //! Return the character data associated with the Xml element 
 //! identified by \p tagpath for this node. The data is interpreted and 
 //! returned as a string. If the element does not exist
 //! an empty vector is returned. If ErrOnMissing() is true an 
 //! error is generated if the element is missing;
 //!
 //! \param[in] tagpath sequence of tags leading to element
 //! \param[in] defaultVal (optional) string to be assigned if specified element does not exist.
 //! \retval string The string associated with the named element
 //!
 virtual const string GetElementString(const vector<string> &tagpath, const string& defaultVal = XmlNode::_emptyString );
 
 //! Get an element's data of type string vector using a path to node
 //!
 //! Builds the string vector data associated with the Xml element 
 //! identified by a sequence of tags \p tagpath from this node. 
 //! The strings in the vector \p vec must not contain white characters.
 //! If the element does not exist
 //! an empty vector is returned
 //!
 //! \param[in] tagpath Sequence of tags
 //! \param[in] defaultVal (optional) vector<string> to be assigned if specified element does not exist.
 //! \param[out] vec Vector of strings associated with the named element
 //!
 virtual void GetElementStringVec(const vector<string> &tagpath, vector <string> &vec, const vector<string>& defaultVal = _emptyStringVec) ;
 //! Get an element's data of type string vector at the current node
 //!
 //! Build the string vector data associated with the Xml element 
 //! identified by a tag \p at this node. 
 //! The strings in the vector \p vec must not contain white characters.
 //! If the element does not exist
 //! an empty vector is returned
 //!
 //! \param[in] tag Node tag
 //! \param[in] defaultVal (optional) vector<string> to be assigned if specified element does not exist.
 //! \param[out] vec Vector of strings associated with the named element
 //!
 virtual void GetElementStringVec(const string &tag, vector <string> &vec, const vector<string>& defaultVal = _emptyStringVec);
 
//! Add an existing node as a child of the current node.
 //!
 //! The new child node will be
 //! appended to the array of child nodes.
 //!
 //! \note This method differs from the base class method that it
 //! overloads in that it prohibits siblings from having duplicate tags.
 //!
 //! \param[in] tag is the tag that will identify the new node
 //! \param[in] child is the ParamNode object to be added as a child
 //! \retval status Return 0 upon success. A negative number is returned
 //! if a sibling already exists with the same name.
 //
 int AddNode(const string& tag, ParamNode* child);

 //! Add an existing node as a last child of a path of
 //! nodes starting at the current 
 //!
 //! The new child node will be
 //! appended to the array of child nodes of the final node.
 //!
 //! \note This method differs from the base class method that it
 //! overloads in that it prohibits siblings from having duplicate tags.
 //!
 //! \param[in] tagpath is vector of tags specifying path to the new node
 //! \param[in] child is the ParamNode object to be added as a child
 //! \retval status Return 0 upon success. A negative number is returned
 //! if a sibling already exists with the same name, or if one of the
 //! specified nodes in the path sequence does not already exist
 //
 int AddNode(const vector<string>& tagpath, ParamNode* child);


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
 ParamNode *GetChild(size_t index) const {return((ParamNode *) XmlNode::GetChild(index));}

 //! Return the indicated child node based on a sequence of tags
 //!
 //! Return the indicated tagged child node. Return NULL if the child 
 //! does not exist.
 //! \param[in] tagpath Sequence of nodes to specified child
 //! \retval child Returns the indicated child, or NULL if the child
 //! could does not exist
 //
 ParamNode *GetNode(const vector<string> &tagpath);

 //! Return the indicated child node. 
 //! \par
 //! This method is useful whenever it is necessary to obtain data
 //! from a node in the XML hierarchy that is not the root node.
 //! For example, ParamsIso class instances are represented as XML
 //! hierarchies with a subnode for each of the variables in the VDC.
 //! In order to obtain the TransferFunction for one of these variables,
 //! one can use GetNode("var") to obtain the node associated with the
 //! variable "var", and then use GetNode(_transferFunctionTag) to obtain the
 //! XML node associated with the TransferFunction.
 //! Return the indicated tagged child node. Return NULL if the child 
 //! does not exist.
 //! \param[in] tag Name of the child node to return
 //! \retval child Returns the indicated child, or NULL if the child
 //! could does not exist
 //
 ParamNode *GetNode(const string &tag) const {return((ParamNode *) XmlNode::GetChild(tag));}
 //! Replace the indicated child node based on a sequence of tags
 //!
 //! \param[in] tagpath Sequence of nodes to specified child
 //! \param[in] newNode ParamNode to install
 //! \retval status Returns -1, if the child
 //! does not exist
 //
 int ReplaceNode(const vector<string> &tagpath, ParamNode* newNode);

 //! Replace the indicated child node. 
 //!
 //! If child has a paramsBase instance, it is deleted.
 //! \param[in] tag Name of the child node to replace
 //! \param[in] newNode ParamNode to install
 //! \retval status Returns 0 if successful
 //
 int ReplaceNode(const string &tag, ParamNode* newNode);

 //! Delete the indicated child node and all its descendants. 
 //!
 //! \param[in] tag Name of the child node to delete
 //! \retval status Return 0 if successful, -1 if child does not exist.
 //
 int DeleteNode(const string &tag);
		
 //! Delete the indicated child node and all its descendants, based
 //! on a path to the child
 //!
 //! \param[in] tagpath Path to the child node to delete
 //! \retval status Return 0 if successful, -1 if child does not exist.
 //
 int DeleteNode(const vector<string> &tagpath); 

 //! Set a ParamsBase node for which this is the root
 //!
 //! The ParamsBase node is NULL unless this node is associated with a
 //! registered ParamsBase object.
 //! 
 //!
 //! \param[in] pBase Pointer to the ParamsBase node for which this is the root node
 //!
 //! \sa GetParamsBase()
 //
 void SetParamsBase(ParamsBase* pBase) {_paramsBase = pBase;}

 //! Get the ParamsBase node for which this is the root
 //!
 //! The ParamsBase node is NULL unless this node is associated with a
 //! registered ParamsBase object.
 //! 
 //! \retval Pointer to the ParamsBase node for which this is the root node
 //! \sa SetParamsBase()
 //
ParamsBase* GetParamsBase() {return _paramsBase;}
#ifndef DOXYGEN_SKIP_THIS
static const string _paramsBaseAttr;
static const string _paramNodeAttr;

protected:
 static const string _typeAttr;
 
 
 vector<long> longvec;
 vector<double>doublevec;

 ParamsBase* _paramsBase;
#endif //DOXYGEN_SKIP_THIS
};

};

#endif	//	_ParamNode_h_
