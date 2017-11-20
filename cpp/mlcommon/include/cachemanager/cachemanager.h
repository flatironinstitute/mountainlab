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

#ifndef CACHEMANAGER_H
#define CACHEMANAGER_H

#include <QString>
#include <QVariant>

class CacheManagerPrivate;
class CacheManager {
public:
    enum Duration {
        ShortTerm,
        LongTerm
    };

    friend class CacheManagerPrivate;
    CacheManager();
    virtual ~CacheManager();

    void setLocalBasePath(const QString& path);
    void setIntermediateFileFolder(const QString& folder);
    //QString makeRemoteFile(const QString& mlproxy_url, const QString& file_name = "", Duration duration = ShortTerm);
    QString makeLocalFile(const QString& file_name = "", Duration duration = ShortTerm);
    QString makeIntermediateFile(const QString& file_name = "");
    QString localTempPath();

    void setTemporaryFileDuration(QString path, qint64 duration_sec);
    void setTemporaryFileExpirePid(QString path, qint64 pid);
    QString makeExpiringFile(QString file_name, qint64 duration_sec);
    void removeExpiredFiles();

    void cleanUp();

    static CacheManager* globalInstance();

    //private slots:
    //    void slot_remove_on_delete();

private:
    CacheManagerPrivate* d;
};

#endif // CACHEMANAGER_H
