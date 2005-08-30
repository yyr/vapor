//
//      $Id$
//

#ifndef	_XmlNode_h_
#define	_XmlNode_h_

#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <vapor/MyBase.h>
#ifdef WIN32
#pragma warning(disable : 4251)
#endif

namespace VAPoR {


//
//! \class XmlNode
//! \brief An Xml tree
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//! This class manages an XML tree. Each node in the tree
//! coresponds to an XML "parent" element. The concept
//! of "parent" element is a creation of this class,
//! and should be confused with any notions of parent
//! in more commonly used XML jargon. A parent element
//! is simply one possessing child XML elements 
//! Non-parent elements - those elements that do not have 
//! children elements -
//! may be thought of as data elements. Typically
//! non-parent elements possess XML character data. Parent
//! elements may or may not have XML character data 
//! themselves.
//!
//! The XmlNode class is derived from the MyBase base
//! class. Hence all of the methods make use of MyBase's
//! error reporting capability - the success of any method
//! (including constructors) can (and should) be tested
//! with the GetErrCode() method. If non-zero, an error 
//! message can be retrieved with GetErrMsg().
//!
class VDF_API XmlNode : public VetsUtil::MyBase {
public:
	enum ErrCode_T {
		ERR_DEF = 1,	// default error
		ERR_TNP			// Tag not present
	};


 //! Constructor for the XmlNode class.
 //!
 //! Create's a new Xml node 
 //!
 //! \param[in] tag Name of Xml node
 //! \param[in] attrs A list of Xml attribute names and values for this node
 //! \param[in] numChildren Reserve space for the indicated number of 
 //! children. Children must be created with
 //! the NewChild() method
 //!
 XmlNode(
	const string &tag, const map<string, string> &attrs, 
	size_t numChildrenHint = 0
 );
 ~XmlNode();

 //! Set or get that node's tag (name)
 //!
 //! \retval tag A reference to the node's tag
 //
 string &Tag() { return (_tag); }

 // These methods set or get XML character data, possibly formatting
 // the data in the process. The paramter 'tag' identifies the XML
 // element tag associated with the character data. The
 // parameter, 'values', contains the character data itself. The Long and 
 // Double versions of these methods convert a between character streams
 // and vectors of longs or doubles as appropriate. 
 //
 // Get methods return a negative result if the named tag does not exist.
 // If the named tag does not exist, 'Get' methods will return a 
 // reference to a vector (or string) of zero length, *and* will
 // register an error with the SetErrMsg() method.
 //

 //! Set an Xml element of type long
 //!
 //! This method defines and sets an Xml element. The Xml character 
 //! data to be associated with this element is the array of longs
 //! specified by \p values
 //! 
 //! \param[in] tag Name of the element to define/set
 //! \param[in] values Vector of longs to be converted to character data
 //!
 //! \retval status Returns a non-negative value on success
 //
 int	SetElementLong(const string &tag, const vector<long> &values);

 //! Get an Xml element's data of type long
 //!
 //! Return the character data associated with the Xml elemented 
 //! named by \p tag for this node. The data is interpreted and 
 //! returned as a vector of longs. If the element does not exist
 //! an empty vector is returned
 //!
 //! \param[in] tag Name of element
 //! \retval vector Vector of longs associated with the named elemented
 //!
 const vector<long> &GetElementLong(const string &tag) const;

 //! Return true if the named element of type long exists
 //!
 //! \param[in] tag Name of element
 //! \retval bool 
 //!
 int HasElementLong(const string &tag) const;

 //! Set an Xml element of type double
 //!
 //! This method defines and sets an Xml element. The Xml character 
 //! data to be associated with this element is the array of doubles
 //! specified by \p values
 //! 
 //! \param[in] tag Name of the element to define/set
 //! \param[in] values Vector of doubles to be converted to character data
 //!
 //! \retval status Returns a non-negative value on success
 //
 int	SetElementDouble(const string &tag, const vector<double> &values);

 //! Get an Xml element's data of type double
 //!
 //! Return the character data associated with the Xml elemented 
 //! named by \p tag for this node. The data is interpreted and 
 //! returned as a vector of doubles. If the element does not exist
 //! an empty vector is returned
 //!
 //! \param[in] tag Name of element
 //! \retval vector Vector of doubles associated with the named elemented
 //!
 const vector<double> &GetElementDouble(const string &tag) const;

 //! Return true if the named element of type double exists
 //!
 //! \param[in] tag Name of element
 //! \retval bool 
 //!
 int HasElementDouble(const string &tag) const;

 //! Set an Xml element of type string
 //!
 //! This method defines and sets an Xml element. The Xml character 
 //! data to be associated with this element is the array of characters
 //! specified by \p values
 //! 
 //! \param[in] tag Name of the element to define/set
 //! \param[in] values Vector of characters to be converted to character data
 //!
 //! \retval status Returns a non-negative value on success
 //
 int	SetElementString(const string &tag, const string &str);

 //! Get an Xml element's data of type string
 //!
 //! Return the character data associated with the Xml elemented 
 //! named by \p tag for this node. The data is interpreted and 
 //! returned as a string. If the element does not exist
 //! an empty vector is returned
 //!
 //! \param[in] tag Name of element
 //! \retval vector Vector of doubles associated with the named elemented
 //!
 const string &GetElementString(const string &tag) const;

 //! Return true if the named element of type string exists
 //!
 //! \param[in] tag Name of element
 //! \retval bool 
 //!
 int HasElementString(const string &tag) const;

 //! Return the number of children nodes this node has
 //!
 //! \retval n The number of direct children this node has
 //!
 //! \sa NewChild()
 //
 int GetNumChildren() const { return (int)(_children.size());};

 //! Create a new child of this node
 //!
 //! Create a new child node, named \p tag. The new child node will be 
 //! appended to the array of child nodes. The \p numChildrenHint
 //! parameter is a hint specifying how many children the new child
 //! itself may have.
 //!
 //! \param[in] tag Name to give the new child node
 //! \param[in] attrs A list of Xml attribute names and values for this node
 //! \param[in] numChildrenHint Reserve space for future children of this node
 //! \retval child Returns the newly created child, or NULL if the child
 //! could not be created
 //
 XmlNode *NewChild(
	const string &tag, const map <string, string> &attrs, 
	size_t numChildrenHint = 0
 );
 //! Add an existing node as a child of the current node.
 //!
 //! The new child node will be 
 //! appended to the array of child nodes. 
 //!
 //! \param[in] child is the XmlNode object to be added as a child
 //
 void AddChild(
	XmlNode* child
 );
 //! Delete the indicated child node.
 //! 
 //! Delete the indicated child node, decrementing the total number
 //! of children by one. Return an error if the child does not
 //! exist (i.e. if index >= GetNumChildren())
 //!
 //! \param[in] index Index of the child. The first child is zero
 //! \retval status Returns a non-negative value on success
 //! \sa GetNumChildren()
 int	DeleteChild(size_t index);

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
 XmlNode *GetChild(size_t index);

 //! Return the indicated child node. 
 //!
 //! Return the indicated tagged child node. Return NULL if the child 
 //! does not exist.
 //! \param[in] tag Name of the child node to return
 //! \retval child Returns the indicated child, or NULL if the child
 //! could does not exist
 //
 XmlNode *GetChild(const string &tag);

 //! Write the XML tree, rooted at this node, to a file in XML format
 //
 friend ostream& operator<<(ostream &s, const XmlNode& node);

 //Following is a substitute for exporting the "<<" operator in windows.
 //I don't know how to export an operator<< !
 static ostream& streamOut(ostream& os, const XmlNode& node);

private:
 int	_objInitialized;	// has the obj successfully been initialized?

 map <string, vector<long>*> _longmap;	// node's long data
 map <string, vector<double>*> _doublemap;	// node's double data
 map <string, string> _stringmap;		// node's string data
 map <string, string> _attrmap;		// node's attributes
 vector <XmlNode *> _children;				// node's children
 string _tag;						// node's tag name
 vector <long> _emptyLongVec;				// empty elements 
 vector <double> _emptyDoubleVec;
 string _emptyString;


};
//ostream& VAPoR::operator<< (ostream& os, const XmlNode& node);
};

#endif	//	_XmlNode_h_
