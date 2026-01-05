/****************************************************************************
** Meta object code from reading C++ file 'studentwidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../include/studentwidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'studentwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_StudentWidget_t {
    QByteArrayData data[19];
    char stringdata0[261];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_StudentWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_StudentWidget_t qt_meta_stringdata_StudentWidget = {
    {
QT_MOC_LITERAL(0, 0, 13), // "StudentWidget"
QT_MOC_LITERAL(1, 14, 15), // "logoutRequested"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 14), // "onViewTeachers"
QT_MOC_LITERAL(4, 46, 11), // "onViewSlots"
QT_MOC_LITERAL(5, 58, 13), // "onBookMeeting"
QT_MOC_LITERAL(6, 72, 14), // "isGroupBooking"
QT_MOC_LITERAL(7, 87, 7), // "groupId"
QT_MOC_LITERAL(8, 95, 14), // "onViewMeetings"
QT_MOC_LITERAL(9, 110, 15), // "onCancelMeeting"
QT_MOC_LITERAL(10, 126, 13), // "onViewHistory"
QT_MOC_LITERAL(11, 140, 13), // "onViewMinutes"
QT_MOC_LITERAL(12, 154, 12), // "onViewGroups"
QT_MOC_LITERAL(13, 167, 13), // "onCreateGroup"
QT_MOC_LITERAL(14, 181, 18), // "onRequestJoinGroup"
QT_MOC_LITERAL(15, 200, 18), // "onViewJoinRequests"
QT_MOC_LITERAL(16, 219, 16), // "onApproveRequest"
QT_MOC_LITERAL(17, 236, 15), // "onRejectRequest"
QT_MOC_LITERAL(18, 252, 8) // "onLogout"

    },
    "StudentWidget\0logoutRequested\0\0"
    "onViewTeachers\0onViewSlots\0onBookMeeting\0"
    "isGroupBooking\0groupId\0onViewMeetings\0"
    "onCancelMeeting\0onViewHistory\0"
    "onViewMinutes\0onViewGroups\0onCreateGroup\0"
    "onRequestJoinGroup\0onViewJoinRequests\0"
    "onApproveRequest\0onRejectRequest\0"
    "onLogout"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_StudentWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      17,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   99,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       3,    0,  100,    2, 0x08 /* Private */,
       4,    0,  101,    2, 0x08 /* Private */,
       5,    2,  102,    2, 0x08 /* Private */,
       5,    1,  107,    2, 0x28 /* Private | MethodCloned */,
       5,    0,  110,    2, 0x28 /* Private | MethodCloned */,
       8,    0,  111,    2, 0x08 /* Private */,
       9,    0,  112,    2, 0x08 /* Private */,
      10,    0,  113,    2, 0x08 /* Private */,
      11,    0,  114,    2, 0x08 /* Private */,
      12,    0,  115,    2, 0x08 /* Private */,
      13,    0,  116,    2, 0x08 /* Private */,
      14,    0,  117,    2, 0x08 /* Private */,
      15,    0,  118,    2, 0x08 /* Private */,
      16,    0,  119,    2, 0x08 /* Private */,
      17,    0,  120,    2, 0x08 /* Private */,
      18,    0,  121,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool, QMetaType::Int,    6,    7,
    QMetaType::Void, QMetaType::Bool,    6,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void StudentWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<StudentWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->logoutRequested(); break;
        case 1: _t->onViewTeachers(); break;
        case 2: _t->onViewSlots(); break;
        case 3: _t->onBookMeeting((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 4: _t->onBookMeeting((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 5: _t->onBookMeeting(); break;
        case 6: _t->onViewMeetings(); break;
        case 7: _t->onCancelMeeting(); break;
        case 8: _t->onViewHistory(); break;
        case 9: _t->onViewMinutes(); break;
        case 10: _t->onViewGroups(); break;
        case 11: _t->onCreateGroup(); break;
        case 12: _t->onRequestJoinGroup(); break;
        case 13: _t->onViewJoinRequests(); break;
        case 14: _t->onApproveRequest(); break;
        case 15: _t->onRejectRequest(); break;
        case 16: _t->onLogout(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (StudentWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StudentWidget::logoutRequested)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject StudentWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_StudentWidget.data,
    qt_meta_data_StudentWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *StudentWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *StudentWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_StudentWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int StudentWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 17)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 17)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 17;
    }
    return _id;
}

// SIGNAL 0
void StudentWidget::logoutRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
