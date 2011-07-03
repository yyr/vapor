//
//      $Id$
//
//	File:		TransformDelegate.h
//
//	Author:		Kenny Gruchalla
//			National Renewable Energy Laboratory
//
//	Description:	Delegate for inline editing of transform model
//
//

#ifndef TRANSFORMDELEGATE_H
#define TRANSFORMDELEGATE_H

#include <QItemDelegate>


QT_USE_NAMESPACE

namespace VAPoR 
{

class TransformDelegate : public QItemDelegate
{
    Q_OBJECT

public:
   TransformDelegate(QObject *parent = 0);
   virtual ~TransformDelegate() {}

   QWidget *createEditor(QWidget *parent, 
                         const QStyleOptionViewItem &option,
                         const QModelIndex &index) const;
   void setEditorData(QWidget *editor, const QModelIndex &index) const;
   void setModelData(QWidget *editor, 
                     QAbstractItemModel *model,
                     const QModelIndex &index) const;
   void updateEditorGeometry(QWidget *editor,
                             const QStyleOptionViewItem &option, 
                             const QModelIndex &index) const;

 signals:

   void transformChanged() const;

};

};

#endif
