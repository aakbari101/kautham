/****************************************************************************
** Meta object code from reading C++ file 'constrainedcontrolwidget.h'
**
** Created: Thu Apr 10 11:50:34 2014
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../libgui/constrainedcontrolwidget.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'constrainedcontrolwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Kautham__ConstrainedControlWidget[] = {

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
      39,   35,   34,   34, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_Kautham__ConstrainedControlWidget[] = {
    "Kautham::ConstrainedControlWidget\0\0"
    "val\0sliderChanged(int)\0"
};

void Kautham::ConstrainedControlWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ConstrainedControlWidget *_t = static_cast<ConstrainedControlWidget *>(_o);
        switch (_id) {
        case 0: _t->sliderChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData Kautham::ConstrainedControlWidget::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Kautham::ConstrainedControlWidget::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_Kautham__ConstrainedControlWidget,
      qt_meta_data_Kautham__ConstrainedControlWidget, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Kautham::ConstrainedControlWidget::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Kautham::ConstrainedControlWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Kautham::ConstrainedControlWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Kautham__ConstrainedControlWidget))
        return static_cast<void*>(const_cast< ConstrainedControlWidget*>(this));
    return QWidget::qt_metacast(_clname);
}

int Kautham::ConstrainedControlWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
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
