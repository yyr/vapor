//************************************************************************
//																		*
//		     Copyright (C)  2005										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		mapeditor.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		August 2005
//
//	Description:	Implements the MapEditor class.  This is the abstract class
//		that manages a mapper function editing session
//
#include "mapeditor.h"
#include "mapperfunction.h"
#include <qframe.h>
#include <qimage.h>
#include "session.h"
#include "params.h"
#include "vizwinmgr.h"
using namespace VAPoR;

MapEditor::MapEditor(MapperFunction* tf,  QFrame* frm){
	myMapperFunction = tf;
	myFrame = frm;
	tf->setEditor(this);
	varNum = -1;
}
MapEditor::~MapEditor(){
	//Don't delete the image:  QT refcounts them
	//However, must notify the frame that I'm no longer here!
	//Also, be sure to delete the Mapper function!
	delete myMapperFunction;
}
