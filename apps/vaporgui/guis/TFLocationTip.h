//-- TFLocationTip.h -------------------------------------------------------
//
// MappingFrame tool tip. 
//
//----------------------------------------------------------------------------
#ifndef TFLocationTip_H
#define TFLocationTip_H

#include <qtooltip.h>
#include <qwidget.h>

namespace VAPoR {

class MappingFrame;

class TFLocationTip : public QToolTip 
{

public:

    TFLocationTip( MappingFrame *parent );
    virtual ~TFLocationTip() {}

protected:

    void maybeTip( const QPoint& );
    MappingFrame *_frame;  
};
};

#endif
