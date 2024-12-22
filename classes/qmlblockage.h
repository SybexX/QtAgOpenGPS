#ifndef QMLBLOCKAGE_H
#define QMLBLOCKAGE_H

#include <QObject>
#include <QVariant>
#include <QVariantList>

class qmlblockage : public QObject
{
    Q_OBJECT
protected:
    QVariantList rows;
    bool needRead = true;
public:
    explicit qmlblockage(QObject *parent = nullptr);

    static inline QObject *aog_root = NULL;
    static inline void set_aog_root(QObject *aog_root_) { aog_root = aog_root_;}

    inline void set (int sectionno, int new_state) {
        if (needRead) {
            needRead = false;
            rows = aog_root->property("rowCount").toList();
        }

        rows[sectionno] = QVariant((int) new_state);
        aog_root->setProperty("rowCount", rows);
        qDebug() << rows;
    }

public slots:
    void onRowsUpdated();
};
#endif // QMLBLOCKAGE_H
