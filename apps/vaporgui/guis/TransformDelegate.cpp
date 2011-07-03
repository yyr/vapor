//
//      $Id$
//
//	File:		TransformDelegate.cpp
//
//	Author:		Kenny Gruchalla
//			National Renewable Energy Laboratory
//
//	Description:	Delegate for inline editing of transform model
//
//

#include "TransformDelegate.h"
#include "TransformWidget.h"
#include "ModelParams.h"
#include "vizwinmgr.h"
#include "session.h"
#include "panelcommand.h"

#include <QtGui>

using namespace std;
using namespace VAPoR;

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
TransformDelegate::TransformDelegate(QObject *parent)
    : QItemDelegate(parent)
{
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
QWidget *TransformDelegate::createEditor(QWidget *parent,
                                         const QStyleOptionViewItem&,
                                         const QModelIndex &index) const
{
   return new TransformWidget(parent);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void TransformDelegate::setEditorData(QWidget *editor,
                                      const QModelIndex &index) const
{
   QStringList data = index.model()->data(index, Qt::EditRole).toString().split(" ");

   TransformWidget *widget = static_cast<TransformWidget*>(editor);
   widget->setLabel(data[0]);

   widget->setX(data[1].toDouble());
   widget->setY(data[2].toDouble());
   widget->setZ(data[3].toDouble());

   if (data.size() == 5)
   {
      widget->setTheta(data[4].toDouble());
   }

   widget->show();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void TransformDelegate::setModelData(QWidget *editor,
                                     QAbstractItemModel *model,
                                     const QModelIndex &index) const
{
   QString text;
   TransformWidget *widget = static_cast<TransformWidget*>(editor);
   widget->interpretText();

   text = widget->label();
   text += QString(" %1 %2 %3").arg(widget->x()).arg(widget->y()).arg(widget->z());

   if (widget->label() == "Rotate")
   {
      text += QString(" %1").arg(widget->theta());
   }

   QString data = index.model()->data(index, Qt::EditRole).toString();

   if (data != text)
   {
      QString cmdtext = "edit " + widget->label() + " item";

      ModelParams* modelParams = (ModelParams*) VizWinMgr::getActiveParams(ModelParams::_modelParamsTag);
      PanelCommand* cmd = PanelCommand::captureStart(modelParams, cmdtext.toAscii().data());

      model->setData(index, QVariant(text));

      emit(transformChanged());
      
      PanelCommand::captureEnd(cmd, modelParams);
   }
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void TransformDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

