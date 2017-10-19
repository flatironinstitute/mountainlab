/******************************************************
** See the accompanying README and LICENSE files
** Author(s): Jeremy Magland
** Created: 4/27/2016
*******************************************************/

#ifndef ProcessorManager_H
#define ProcessorManager_H

#include <QString>
#include <QVariant>
#include <QProcess>
#include <QDateTime>
#include <QJsonObject>

struct MLParameter {
    QString name;
    QString ptype;
    QString description;
    bool optional;
    QVariant default_value;
};

struct MLProcessor {
    QString name;
    QString version;
    QString description;
    QMap<QString, MLParameter> inputs;
    QMap<QString, MLParameter> outputs;
    QMap<QString, MLParameter> parameters;
    QString exe_command;
    QJsonObject spec;
    QString mp_file_name;

    QString basepath;
};

class ProcessorManagerPrivate;
class ProcessorManager : public QObject {
public:
    friend class ProcessorManagerPrivate;
    ProcessorManager();
    virtual ~ProcessorManager();

    void setProcessorPaths(const QStringList& paths);
    void reloadProcessors();

    QStringList processorNames() const;
    MLProcessor processor(const QString& name);

    bool checkParameters(const QString& processor_name, const QVariantMap& parameters, QString* errstr);
    void setDefaultParameters(const QString& processor_name, QVariantMap& parameters);

private:
    bool loadProcessors(const QString& path, bool recursive = true);
    bool loadProcessorFile(const QString& path);

private:
    ProcessorManagerPrivate* d;
};

QString execute_and_read_stdout(QString cmd);

#endif // ProcessorManager_H
