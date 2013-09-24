/****************************************************************************
** Meta object code from reading C++ file 'createvdfcomment.h'
**
** Created: Mon Sep 23 12:30:49 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../createvdfcomment.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'createvdfcomment.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_CreateVdfComment[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      18,   17,   17,   17, 0x08,
      42,   17,   17,   17, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_CreateVdfComment[] = {
    "CreateVdfComment\0\0on_buttonBox_accepted()\0"
    "on_buttonBox_rejected()\0"
};

void CreateVdfComment::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        CreateVdfComment *_t = static_cast<CreateVdfComment *>(_o);
        switch (_id) {
        case 0: _t->on_buttonBox_accepted(); break;
        case 1: _t->on_buttonBox_rejected(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData CreateVdfComment::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CreateVdfComment::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_CreateVdfComment,
      qt_meta_data_CreateVdfComment, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CreateVdfComment::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CreateVdfComment::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CreateVdfComment::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CreateVdfComment))
        return static_cast<void*>(const_cast< CreateVdfComment*>(this));
    if (!strcmp(_clname, "Ui_Page3cmt"))
        return static_cast< Ui_Page3cmt*>(const_cast< CreateVdfComment*>(this));
    return QDialog::qt_metacast(_clname);
}

int CreateVdfComment::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
