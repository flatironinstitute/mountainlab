#include "mllog.h"
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include "mlcommon.h"
#include <QCoreApplication>
#include <QThread>

namespace MLLog {
static bool s_debug_mode = false;
void setDebugMode(bool val)
{
    s_debug_mode = val;
}
QString get_log_file_name()
{
    static QString pid = QString("%1").arg(qApp->applicationPid());
    QString thread_id = QString("%1").arg((long long)QThread::currentThreadId());
    QString fname0 = QString("MLLog-pid%1-thread%2.log").arg(pid).arg(thread_id);
    return MLUtil::mlLogPath() + "/" + fname0;
}
void write_log_object(const QJsonObject& obj)
{
    QString fname = get_log_file_name();
    if (fname.isEmpty())
        return;
    QFile logf(fname);
    if (!logf.open(QIODevice::Append))
        return;
    QByteArray json = QJsonDocument(obj).toJson();
    logf.write(json.data(), json.count());
    logf.write("\n", 1);
    logf.close();
}
}

void MLLog::message(
    QString key1, QVariant value1,
    QString key2, QVariant value2,
    QString key3, QVariant value3,
    QString key4, QVariant value4,
    QString key5, QVariant value5,
    QString key6, QVariant value6)
{
    if (s_debug_mode) {
        QJsonObject obj;
        obj["timestamp_human"] = QDateTime::currentDateTime().toString("yyyy-MM-dd:hh.mm.ss.zzz");
        obj["timestamp"] = QDateTime::currentMSecsSinceEpoch();
        if (value1.isValid()) {
            obj[key1] = QJsonValue::fromVariant(value1);
        }
        else {
            obj["message"] = key1;
        }
        if (!key2.isEmpty())
            obj[key2] = QJsonValue::fromVariant(value2);
        if (!key3.isEmpty())
            obj[key3] = QJsonValue::fromVariant(value3);
        if (!key4.isEmpty())
            obj[key4] = QJsonValue::fromVariant(value4);
        if (!key5.isEmpty())
            obj[key5] = QJsonValue::fromVariant(value5);
        if (!key6.isEmpty())
            obj[key6] = QJsonValue::fromVariant(value6);
        write_log_object(obj);
    }
}
