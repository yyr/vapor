//
//      $Id$
//
//	File:		TransformListModel.h
//
//	Author:		Kenny Gruchalla
//			National Renewable Energy Laboratory
//
//	Description:	List Model for manipulating tranformation elements
//
//

#ifndef TRANSFORMLISTMODEL_H
#define TRANSFORMLISTMODEL_H

#include <QStringListModel>
#include "Transform3d.h"

QT_USE_NAMESPACE

namespace VAPoR 
{

class TransformListModel : public QStringListModel
{
   Q_OBJECT

public:

   TransformListModel(const QStringList &strings, QObject *parent=0);
   Qt::ItemFlags flags(const QModelIndex &index) const;
   void add(Transform3d::TransformBase*);
};

};

#endif


