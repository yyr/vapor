//-- TFLocationTip.cpp -------------------------------------------------------
//
// MappingFrame tool tip. 
//
//----------------------------------------------------------------------------

#include "TFLocationTip.h"
#include "mappingframe.h"

using namespace VAPoR;

//---------------------------------------------------------------------------- 
//
//----------------------------------------------------------------------------
TFLocationTip::TFLocationTip(MappingFrame *parent)
  : QToolTip(parent),
    _frame(parent)
{
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void TFLocationTip::maybeTip( const QPoint &pos )
{
  QRect r( pos - QPoint(1,1), pos + QPoint(1,1));

  if (_frame)
  {
    QString tipText = _frame->tipText(pos);

    if (!tipText.isNull())
    {
      tip(r, tipText);
    }
  }
}
