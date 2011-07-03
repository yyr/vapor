//
//      $Id$
//
//	File:		TransformListModel.cpp
//
//	Author:		Kenny Gruchalla
//			National Renewable Energy Laboratory
//
//	Description:	List Model for manipulating tranformation elements
//
//

#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif

#include <QtGui>

#include "TransformListModel.h"

using namespace VAPoR;

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
TransformListModel::TransformListModel(const QStringList &strings, 
                                       QObject *parent):
        QStringListModel(strings,parent)
{
}

//----------------------------------------------------------------------------
// Override flags to handle drag-drop reordering
//----------------------------------------------------------------------------
Qt::ItemFlags TransformListModel::flags(const QModelIndex &index) const
{
   Qt::ItemFlags f;

   if (index.isValid())
      f = Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | 
          Qt::ItemIsEnabled | Qt::ItemIsEditable;
   else
      f=  Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | 
          Qt::ItemIsEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable;

   return f;
}

//----------------------------------------------------------------------------
// Add a transform to the model
//----------------------------------------------------------------------------
void TransformListModel::add(Transform3d::TransformBase* transform)
{
   QModelIndex i = index(rowCount()-1,0);
   insertRow(i.row()+1); QModelIndex child = index(i.row()+1, 0);

   setData(child, QVariant(transform->label()));
}
