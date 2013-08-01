/****************************************************************************
** Meta object code from reading C++ file 'createvdfadvanced.h'
**
** Created: Wed Jul 31 16:08:23 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../createvdfadvanced.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'createvdfadvanced.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_CreateVdfAdvanced[] = {

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
      19,   18,   18,   18, 0x08,
      45,   18,   18,   18, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_CreateVdfAdvanced[] = {
    "CreateVdfAdvanced\0\0on_acceptButton_clicked()\0"
    "on_cancelButton_clicked()\0"
};

void CreateVdfAdvanced::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        CreateVdfAdvanced *_t = static_cast<CreateVdfAdvanced *>(_o);
        switch (_id) {
        case 0: _t->on_acceptButton_clicked(); break;
        case 1: _t->on_cancelButton_clicked(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData CreateVdfAdvanced::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CreateVdfAdvanced::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_CreateVdfAdvanced,
      qt_meta_data_CreateVdfAdvanced, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CreateVdfAdvanced::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CreateVdfAdvanced::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CreateVdfAdvanced::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CreateVdfAdvanced))
        return static_cast<void*>(const_cast< CreateVdfAdvanced*>(this));
    if (!strcmp(_clname, "Ui_Page3adv"))
        return static_cast< Ui_Page3adv*>(const_cast< CreateVdfAdvanced*>(this));
    return QDialog::qt_metacast(_clname);
}

int CreateVdfAdvanced::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
