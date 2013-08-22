/****************************************************************************
** Meta object code from reading C++ file 'selectfilepage.h'
**
** Created: Tue Aug 20 12:09:43 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../selectfilepage.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'selectfilepage.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_SelectFilePage[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      16,   15,   15,   15, 0x08,
      49,   15,   15,   15, 0x08,
      76,   15,   15,   15, 0x08,
     106,   15,   15,   15, 0x08,
     134,   15,   15,   15, 0x08,
     162,   15,   15,   15, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_SelectFilePage[] = {
    "SelectFilePage\0\0on_browseOutputVdfFile_clicked()\0"
    "on_addFileButton_clicked()\0"
    "on_removeFileButton_clicked()\0"
    "on_momRadioButton_clicked()\0"
    "on_popRadioButton_clicked()\0"
    "on_romsRadioButton_clicked()\0"
};

void SelectFilePage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        SelectFilePage *_t = static_cast<SelectFilePage *>(_o);
        switch (_id) {
        case 0: _t->on_browseOutputVdfFile_clicked(); break;
        case 1: _t->on_addFileButton_clicked(); break;
        case 2: _t->on_removeFileButton_clicked(); break;
        case 3: _t->on_momRadioButton_clicked(); break;
        case 4: _t->on_popRadioButton_clicked(); break;
        case 5: _t->on_romsRadioButton_clicked(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData SelectFilePage::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject SelectFilePage::staticMetaObject = {
    { &QWizardPage::staticMetaObject, qt_meta_stringdata_SelectFilePage,
      qt_meta_data_SelectFilePage, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &SelectFilePage::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *SelectFilePage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *SelectFilePage::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_SelectFilePage))
        return static_cast<void*>(const_cast< SelectFilePage*>(this));
    if (!strcmp(_clname, "Ui_Page2"))
        return static_cast< Ui_Page2*>(const_cast< SelectFilePage*>(this));
    return QWizardPage::qt_metacast(_clname);
}

int SelectFilePage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWizardPage::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
