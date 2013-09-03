/****************************************************************************
** Meta object code from reading C++ file 'createvdfaddnewvar.h'
**
** Created: Thu Aug 29 12:22:07 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../createvdfaddnewvar.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'createvdfaddnewvar.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_CreateVdfAddNewVar[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      20,   19,   19,   19, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_CreateVdfAddNewVar[] = {
    "CreateVdfAddNewVar\0\0on_buttonBox_accepted()\0"
};

void CreateVdfAddNewVar::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        CreateVdfAddNewVar *_t = static_cast<CreateVdfAddNewVar *>(_o);
        switch (_id) {
        case 0: _t->on_buttonBox_accepted(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData CreateVdfAddNewVar::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CreateVdfAddNewVar::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_CreateVdfAddNewVar,
      qt_meta_data_CreateVdfAddNewVar, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CreateVdfAddNewVar::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CreateVdfAddNewVar::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CreateVdfAddNewVar::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CreateVdfAddNewVar))
        return static_cast<void*>(const_cast< CreateVdfAddNewVar*>(this));
    if (!strcmp(_clname, "Ui_Page3addnewvar"))
        return static_cast< Ui_Page3addnewvar*>(const_cast< CreateVdfAddNewVar*>(this));
    return QDialog::qt_metacast(_clname);
}

int CreateVdfAddNewVar::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
