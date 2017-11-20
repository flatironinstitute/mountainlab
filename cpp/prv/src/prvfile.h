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
#ifndef PRVFILE_H
#define PRVFILE_H

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QObject>
#include <functional>
#include "mlcommon.h"

struct PrvFileCreateOptions {
    bool create_temporary_files = false;
};

struct PrvFileLocateOptions {
    QStringList local_search_paths;
    bool search_locally = true;
    bool search_remotely = false;
    QJsonArray remote_servers;
    bool verbose = false;
};

struct PrvFileRecoverOptions {
    bool recover_all_prv_files = false;
    PrvFileLocateOptions locate_opts;
};

class PrvFilePrivate;
class PrvFile {
public:
    friend class PrvFilePrivate;
    PrvFile(const QString& file_path = "");
    PrvFile(const QJsonObject& obj);
    PrvFile(const PrvFile& other);
    virtual ~PrvFile();
    void operator=(const PrvFile& other);
    QJsonObject object() const;
    bool read(const QString& file_path);
    bool write(const QString& file_path) const;
    bool representsFile() const;
    bool representsFolder() const;
    bool createFromFile(const QString& file_path, const PrvFileCreateOptions& opts);
    bool createFromDirectory(const QString& folder_path, const PrvFileCreateOptions& opts);
    bool recoverFile(const QString& dst_file_path, const PrvFileRecoverOptions& opts);
    bool recoverFolder(const QString& dst_folder_path, const PrvFileRecoverOptions& opts);
    QString locate(const PrvFileLocateOptions& opts);
    QString locateDirectory(const PrvFileLocateOptions& opts);

    QString prvFilePath() const;
    QString checksum() const;
    QString fcs() const;
    bigint size() const;
    QString originalPath() const;

private:
    PrvFilePrivate* d;
};

QString http_get_text_curl_0(const QString& url);
bool is_url(QString txt);

namespace NetUtils {
typedef std::function<void(qint64, qint64)> ProgressFunction;
QString httpPostFile(const QUrl& url, const QString& fileName, const ProgressFunction& f = ProgressFunction());
}

#endif // PRVFILE_H
