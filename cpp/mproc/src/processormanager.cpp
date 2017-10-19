/******************************************************
** See the accompanying README and LICENSE files
** Author(s): Jeremy Magland
** Created: 4/27/2016
*******************************************************/

#include "processormanager.h"
#include <QDir>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "mlcommon.h"

#include <QCoreApplication>
#include <QThread>
#include <QTimer>
#include <cachemanager.h>

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(MPM, "mproc.processormanager")

class ProcessorManagerPrivate {
public:
    ProcessorManager* q;

    QStringList m_processor_paths;
    QMap<QString, MLProcessor> m_processors;

    void reload_processors();

    static MLProcessor create_processor_from_json_object(QJsonObject obj);
    static MLParameter create_parameter_from_json_object(QJsonObject obj);
};

ProcessorManager::ProcessorManager()
{
    d = new ProcessorManagerPrivate;
    d->q = this;
}

ProcessorManager::~ProcessorManager()
{
    delete d;
}

void ProcessorManager::setProcessorPaths(const QStringList& paths)
{
    d->m_processor_paths = paths;
    d->reload_processors();
}

void ProcessorManager::reloadProcessors()
{
    d->reload_processors();
}

bool ProcessorManager::loadProcessors(const QString& path, bool recursive)
{
    QStringList fnames = QDir(path).entryList(QStringList("*.mp"), QDir::Files, QDir::Name);
    foreach (QString fname, fnames) {
        if (!this->loadProcessorFile(path + "/" + fname)) {
            //return false;
        }
    }
    if (recursive) {
        QStringList subdirs = QDir(path).entryList(QStringList("*"), QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        foreach (QString subdir, subdirs) {
            if (!this->loadProcessors(path + "/" + subdir))
                return false;
        }
    }
    return true;
}

bool ProcessorManager::loadProcessorFile(const QString& path)
{
    QString spec_tmp_fname = CacheManager::globalInstance()->makeLocalFile(MLUtil::computeSha1SumOfString(path) + ".spec"); //changed by jfm on 9/7/17, see below
    QString json;
    if (QFileInfo(path).isExecutable()) {
        if ((QFile::exists(spec_tmp_fname)) && (QFileInfo(spec_tmp_fname).lastModified().secsTo(QFileInfo(spec_tmp_fname).lastModified()) >= 0) && (QFileInfo(spec_tmp_fname).lastModified().secsTo(QDateTime::currentDateTime()) <= 10)) {
            json = TextFile::read(spec_tmp_fname); // read the saved spec so we don't need to make the system call next time, but let's be careful about it... regenerate every few seconds
        }
        else {
            QProcess pp;
            pp.start(path, QStringList("spec"));
            if (!pp.waitForFinished(-1)) {
                qCWarning(MPM) << "Problem with executable processor file, waiting for finish: " + path;
                return false;
            }
            pp.waitForReadyRead();
            QString output = pp.readAll();
            json = output;
            if (json.isEmpty()) {
                qCWarning(MPM) << "Potential problem with executable processor file: " + path + ". Expected json output but got empty string.";
                if (QFileInfo(path).size() < 1e6) {
                    json = TextFile::read(path);
                    //now test it, since it is executable we are suspicious...
                    QJsonParseError error;
                    QJsonObject obj = QJsonDocument::fromJson(json.toLatin1(), &error).object();
                    if (error.error != QJsonParseError::NoError) {
                        qCWarning(MPM) << "Executable processor file did not return output for spec: " + path;
                        return false;
                    }
                    else {
                        //we are okay -- apparently the text file .mp got marked as executable by the user, so let's proceed
                    }
                }
                else {
                    qCWarning(MPM) << "File is too large to be a text file. Executable processor file did not return output for spec: " + path;
                    return false;
                }
            }
            else {
                // jfm changed the following on 9/7/17 so as not to write to a place we should not
                //TextFile::write(path + ".spec", json); // so we don't need to make the system call this time
                TextFile::write(spec_tmp_fname, json);
                CacheManager::globalInstance()->setTemporaryFileDuration(spec_tmp_fname, 600);
            }
        }
    }
    else {
        json = TextFile::read(path);
        if (json.isEmpty()) {
            qCWarning(MPM) << "Processor file is empty: " + path;
            return false;
        }
    }
    QJsonParseError error;
    QJsonObject obj = QJsonDocument::fromJson(json.toLatin1(), &error).object();
    if (error.error != QJsonParseError::NoError) {
        qCWarning(MPM) << "Json parse error when loading processor plugin: " << error.errorString();
        return false;
    }
    if (!obj["processors"].isArray()) {
        qCWarning(MPM) << "Problem with processor file: processors field is missing or not any array: " + path;
        return false;
    }
    QJsonArray processors = obj["processors"].toArray();
    for (int i = 0; i < processors.count(); i++) {
        if (!processors[i].isObject()) {
            qCWarning(MPM) << "Problem with processor file: processor is not an object: " + path;
            return false;
        }
        MLProcessor P = d->create_processor_from_json_object(processors[i].toObject());
        P.mp_file_name = path;
        P.basepath = QFileInfo(path).path();
        if (P.name.isEmpty()) {
            qCDebug(MPM) << QJsonDocument(processors[i].toObject()).toJson();
            qCWarning(MPM) << "Problem with processor file: processor error: " + path;
            return false;
        }
        d->m_processors[P.name] = P;
    }
    return true;
}

QStringList ProcessorManager::processorNames() const
{
    return d->m_processors.keys();
}

MLProcessor ProcessorManager::processor(const QString& name)
{
    return d->m_processors.value(name);
}

bool ProcessorManager::checkParameters(const QString& processor_name, const QVariantMap& parameters, QString* errstr)
{
    if (processor_name.isEmpty()) {
        qCWarning(MPM) << "checkProcess: processor name is empty.";
        *errstr = "checkProcess: processor name is empty.";
        return true;
    }
    if (!d->m_processors.contains(processor_name)) {
        qCWarning(MPM) << "checkProcess: Unable to find processor (175): " + processor_name;
        *errstr = "checkProcess: Unable to find processor (175): " + processor_name;
        return false;
    }

    MLProcessor P = d->m_processors[processor_name];
    {
        QStringList keys = P.inputs.keys();
        foreach (QString key, keys) {
            if (!P.inputs[key].optional) {
                if (MLUtil::toStringList(parameters[key]).isEmpty()) {
                    qCWarning(MPM) << QString("checkProcess for %1: Missing input: %2").arg(processor_name).arg(key);
                    *errstr = QString("checkProcess for %1: Missing input: %2").arg(processor_name).arg(key);
                    return false;
                }
            }
        }
    }
    {
        QStringList keys = P.outputs.keys();
        foreach (QString key, keys) {
            if (!P.outputs[key].optional) {
                if (MLUtil::toStringList(parameters[key]).isEmpty()) {
                    qCWarning(MPM) << QString("checkProcess for %1: Missing required output: %2").arg(processor_name).arg(key);
                    *errstr = QString("checkProcess for %1: Missing required output: %2").arg(processor_name).arg(key);
                    return false;
                }
            }
        }
    }
    {
        QStringList keys = P.parameters.keys();
        foreach (QString key, keys) {
            if (!P.parameters[key].optional) {
                if (!parameters.contains(key)) {
                    qCWarning(MPM) << QString("checkProcess for %1: Missing required parameter: %2").arg(processor_name).arg(key);
                    *errstr = QString("checkProcess for %1: Missing required parameter: %2").arg(processor_name).arg(key);
                    return false;
                }
            }
        }
    }
    {
        QStringList parameters_keys = parameters.keys();
        foreach (QString key, parameters_keys) {
            if (!key.startsWith("_")) {
                if ((!P.parameters.contains(key)) && (!P.inputs.contains(key)) && (!P.outputs.contains(key))) {
                    qCWarning(MPM) << QString("checkProcess for %1: invalid parameter: %2").arg(processor_name).arg(key);
                    *errstr = QString("checkProcess for %1: invalid parameter: %2").arg(processor_name).arg(key);
                    return false;
                }
            }
        }
    }
    return true;
}

void ProcessorManager::setDefaultParameters(const QString& processor_name, QVariantMap& parameters)
{
    if (processor_name.isEmpty())
        return;
    if (!d->m_processors.contains(processor_name)) {
        return;
    }
    MLProcessor P = d->m_processors[processor_name];
    {
        QStringList keys = P.parameters.keys();
        foreach (QString key, keys) {
            if (P.parameters[key].optional) {
                if (!parameters.contains(key)) {
                    parameters[key] = P.parameters[key].default_value;
                }
            }
        }
    }
}

QString execute_and_read_stdout(QString cmd)
{
    QProcess P;
    P.start(cmd);
    P.waitForStarted();
    P.waitForFinished(-1);
    return P.readAllStandardOutput();
}

MLProcessor ProcessorManagerPrivate::create_processor_from_json_object(QJsonObject obj)
{
    MLProcessor P;
    P.spec = obj;
    P.name = obj["name"].toString();
    P.version = obj["version"].toString();
    P.description = obj["description"].toString();

    QJsonArray inputs = obj["inputs"].toArray();
    for (int i = 0; i < inputs.count(); i++) {
        MLParameter param = create_parameter_from_json_object(inputs[i].toObject());
        P.inputs[param.name] = param;
    }

    QJsonArray outputs = obj["outputs"].toArray();
    for (int i = 0; i < outputs.count(); i++) {
        MLParameter param = create_parameter_from_json_object(outputs[i].toObject());
        P.outputs[param.name] = param;
    }
    if (!P.outputs.contains("console_out")) {
        MLParameter param;
        param.name = "console_out";
        param.optional = true;
        P.outputs["console_out"] = param;
    }

    QJsonArray parameters = obj["parameters"].toArray();
    for (int i = 0; i < parameters.count(); i++) {
        MLParameter param = create_parameter_from_json_object(parameters[i].toObject());
        P.parameters[param.name] = param;
    }

    P.exe_command = obj["exe_command"].toString();

    return P;
}

MLParameter ProcessorManagerPrivate::create_parameter_from_json_object(QJsonObject obj)
{
    MLParameter param;
    param.name = obj["name"].toString();
    param.ptype = obj["ptype"].toString();
    param.description = obj["description"].toString();
    param.optional = obj["optional"].toBool();
    param.default_value = obj["default_value"].toVariant();
    return param;
}

void ProcessorManagerPrivate::reload_processors()
{
    QMap<QString, MLProcessor> saved = m_processors;
    m_processors.clear();
    foreach (QString processor_path, m_processor_paths) {
        if (!q->loadProcessors(processor_path)) {
            //something happened, let's revert to saved version
            m_processors = saved;
            return;
        }
    }
}
