#ifndef FIELDINTERFACE_H
#define FIELDINTERFACE_H

#include <QObject>
#include <QQmlEngine>
#include <QMutex>
#include <QPropertyBinding>
#include "simpleproperty.h"

class FieldInterface : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT
private:
    explicit FieldInterface(QObject *parent = nullptr);
    ~FieldInterface() override=default;

    //prevent copying
    FieldInterface(const FieldInterface &) = delete;
    FieldInterface &operator=(const FieldInterface &) = delete;

    static FieldInterface *s_instance;
    static QMutex s_mutex;
    static bool s_cpp_created;

public:
    static FieldInterface *instance();
    static FieldInterface *create (QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    enum class Flags {
        Mapping = 1,
        Headland = 2,
        Lines = 4,
        Flags = 8,
    };

    Q_ENUM(Flags)

    SIMPLE_BINDABLE_PROPERTY(QList<QVariant>, field_list)

signals:
    // QML can call these signals directly - no need for Q_INVOKABLE wrappers
    void updateList();
    void newField(QString field_name);
    void openField(QString field_name);
    void newFieldFrom(QString existing_field, QString new_field, int flags);
    void newFieldFromKML(QString field_name, QString file_name);
    void closeField();
    void deleteField(QString field_name);

    //on android permissions restrict what we can do
    //so import and export to zip files
    void exportFieldZip(QString field_name, QString zipfilename);
    void importFieldZip(QString zipfilename);

private:
    Q_OBJECT_BINDABLE_PROPERTY(FieldInterface, QList<QVariant>, m_field_list, &FieldInterface::field_listChanged)
    /* looks like this to QML:
    property var field_list: [
        { index: 0, name: "test3", latitude: 53, longitude: -111, hasBoundary: true, boundaryArea: 1232312 },
        { index: 1, name: "test1", latitude: 53.004, longitude: -111, hasBoundary: true, boundaryArea: -10 },
        { index: 2, name: "test2", latitude: 53.1, longitude: -111.2, hasBoundary: true, boundaryArea: -10 },
        { index: 3, name: "test3", latitude: 53.02, longitude: -111, hasBoundary: true, boundaryArea: 1232312 },
        { index: 4, name: "test1", latitude: 53.004, longitude: -111, hasBoundary: true, boundaryArea: -10 },
        { index: 5, name: "test2", latitude: 53.1, longitude: -111.2, hasBoundary: true, boundaryArea: -10 },
        { index: 6, name: "test3", latitude: 53.009, longitude: -111, hasBoundary: true, boundaryArea: 1232312 },
        { index: 7, name: "test1", latitude: 53.004, longitude: -111, hasBoundary: true, boundaryArea: -10 },
        { index: 8, name: "test2", latitude: 53.1, longitude: -111.2, hasBoundary: true, boundaryArea: -10 },
        { index: 9, name: "test3", latitude: 53.2, longitude: -111, hasBoundary: true, boundaryArea: 1232312 },
        { index: 10, name: "test1", latitude: 53.004, longitude: -111, hasBoundary: true, boundaryArea: -10 },
        { index: 11, name: "test2", latitude: 53.1, longitude: -111.2, hasBoundary: true, boundaryArea: -10 }
    ]
     */
};

#endif // FIELDINTERFACE_H
