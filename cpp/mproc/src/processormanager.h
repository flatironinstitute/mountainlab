/*
 * Copyright 2016-2017 Flatiron Institute, Simons Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
    QString package_uri;
    QMap<QString, MLParameter> inputs;
    QMap<QString, MLParameter> outputs;
    QMap<QString, MLParameter> parameters;
    QString exe_command;
    QString requirements_command;
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

    void setPackageURI(const QString &uri);
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
