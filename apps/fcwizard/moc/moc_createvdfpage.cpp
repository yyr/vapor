/****************************************************************************
** Meta object code from reading C++ file 'createvdfpage.h'
**
** Created: Tue Aug 20 13:59:53 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../createvdfpage.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'createvdfpage.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_CreateVdfPage[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      15,   14,   14,   14, 0x0a,
      29,   14,   14,   14, 0x08,
      62,   14,   14,   14, 0x08,
      92,   14,   14,   14, 0x08,
     118,   14,   14,   14, 0x08,
     146,   14,   14,   14, 0x08,
     181,  175,   14,   14, 0x08,
     223,  175,   14,   14, 0x08,
     261,   14,   14,   14, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_CreateVdfPage[] = {
    "CreateVdfPage\0\0saveAndExit()\0"
    "on_advanceOptionButton_clicked()\0"
    "on_vdfCommentButton_clicked()\0"
    "on_newVarButton_clicked()\0"
    "on_clearAllButton_clicked()\0"
    "on_selectAllButton_clicked()\0value\0"
    "on_startTimeSpinner_valueChanged(QString)\0"
    "on_numtsSpinner_valueChanged(QString)\0"
    "addVar()\0"
};

void CreateVdfPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        CreateVdfPage *_t = static_cast<CreateVdfPage *>(_o);
        switch (_id) {
        case 0: _t->saveAndExit(); break;
        case 1: _t->on_advanceOptionButton_clicked(); break;
        case 2: _t->on_vdfCommentButton_clicked(); break;
        case 3: _t->on_newVarButton_clicked(); break;
        case 4: _t->on_clearAllButton_clicked(); break;
        case 5: _t->on_selectAllButton_clicked(); break;
        case 6: _t->on_startTimeSpinner_valueChanged((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 7: _t->on_numtsSpinner_valueChanged((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 8: _t->addVar(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData CreateVdfPage::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CreateVdfPage::staticMetaObject = {
    { &QWizardPage::staticMetaObject, qt_meta_stringdata_CreateVdfPage,
      qt_meta_data_CreateVdfPage, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CreateVdfPage::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CreateVdfPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CreateVdfPage::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CreateVdfPage))
        return static_cast<void*>(const_cast< CreateVdfPage*>(this));
    if (!strcmp(_clname, "Ui_Page3"))
        return static_cast< Ui_Page3*>(const_cast< CreateVdfPage*>(this));
    return QWizardPage::qt_metacast(_clname);
}

int CreateVdfPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWizardPage::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
