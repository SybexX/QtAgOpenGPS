#ifndef SIMPLEPROPERTY_H
#define SIMPLEPROPERTY_H

#include <QObject>
#include <QBindable>

//macro to make it easier to set up simple bindable properties
//in a class.  WARNING! This macro should only be used in
//the public section of the class, and it always leaves the
//scope in public, even if it was placed initially in a
//private section. Also setter is set_name, instead of setName
//due to limitations of C++ macros. Also bindable_name

// For value types
#define SIMPLE_BINDABLE_PROPERTY(type, name) \
Q_PROPERTY(type name READ name WRITE set_##name NOTIFY name##Changed BINDABLE bindable_##name) \
    public: \
    type name() const { return m_##name.value(); } \
    void set_##name(const type &value) { m_##name.setValue(value); } \
    QBindable<type> bindable_##name() { return &m_##name; } \
    Q_SIGNAL void name##Changed();

// For pointer types (QObject*)
#define SIMPLE_BINDABLE_PROPERTY_PTR(type, name) \
Q_PROPERTY(type name READ name WRITE set_##name NOTIFY name##Changed BINDABLE bindable_##name) \
    public: \
    type name() const { return m_##name.value(); } \
    void set_##name(type value) { m_##name.setValue(value); } \
    QBindable<type> bindable_##name() { return &m_##name; } \
    Q_SIGNAL void name##Changed();

#endif // SIMPLEPROPERTY_H
