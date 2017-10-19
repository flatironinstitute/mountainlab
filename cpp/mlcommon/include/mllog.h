#ifndef MLLOG_H
#define MLLOG_H

#include <QString>
#include <QVariant>

namespace MLLog {

void setLogMode(bool val);
void message(
    QString key1, QVariant value1 = QVariant(),
    QString key2 = "", QVariant value2 = QVariant(),
    QString key3 = "", QVariant value3 = QVariant(),
    QString key4 = "", QVariant value4 = QVariant(),
    QString key5 = "", QVariant value5 = QVariant(),
    QString key6 = "", QVariant value6 = QVariant());
}

#endif // MLLOG_H
