/****************************************************************************
** Meta object code from reading C++ file 'populatedatapage.h'
**
** Created: Mon Oct 21 15:47:29 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../populatedatapage.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'populatedatapage.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_PopulateDataPage[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      18,   17,   17,   17, 0x08,
      46,   17,   17,   17, 0x08,
      75,   17,   17,   17, 0x08,
     116,  110,   17,   17, 0x08,
     158,  110,   17,   17, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_PopulateDataPage[] = {
    "PopulateDataPage\0\0on_clearAllButton_clicked()\0"
    "on_selectAllButton_clicked()\0"
    "on_advancedOptionsButton_clicked()\0"
    "value\0on_startTimeSpinner_valueChanged(QString)\0"
    "on_numtsSpinner_valueChanged(QString)\0"
};

void PopulateDataPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        PopulateDataPage *_t = static_cast<PopulateDataPage *>(_o);
        switch (_id) {
        case 0: _t->on_clearAllButton_clicked(); break;
        case 1: _t->on_selectAllButton_clicked(); break;
        case 2: _t->on_advancedOptionsButton_clicked(); break;
        case 3: _t->on_startTimeSpinner_valueChanged((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 4: _t->on_numtsSpinner_valueChanged((*reinterpret_cast< QString(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData PopulateDataPage::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject PopulateDataPage::staticMetaObject = {
    { &QWizardPage::staticMetaObject, qt_meta_stringdata_PopulateDataPage,
      qt_meta_data_PopulateDataPage, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &PopulateDataPage::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *PopulateDataPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *PopulateDataPage::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_PopulateDataPage))
        return static_cast<void*>(const_cast< PopulateDataPage*>(this));
    if (!strcmp(_clname, "Ui_Page4"))
        return static_cast< Ui_Page4*>(const_cast< PopulateDataPage*>(this));
    return QWizardPage::qt_metacast(_clname);
}

int PopulateDataPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWizardPage::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
