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

#ifndef REMOTEREADMDA_H
#define REMOTEREADMDA_H

#include <QString>
#include "mda.h"
#include "mda32.h"

class RemoteReadMdaPrivate;
/**
 * @brief Do not use this class directly -- it is used by DiskReadMda
 */
class RemoteReadMda {
public:
    friend class RemoteReadMdaPrivate;
    RemoteReadMda(const QString& path = "");
    RemoteReadMda(const RemoteReadMda& other);
    void operator=(const RemoteReadMda& other);
    virtual ~RemoteReadMda();

    void setRemoteDataType(QString dtype);
    void setDownloadChunkSize(int size);
    int downloadChunkSize();

    void setPath(const QString& path);
    QString makePath() const; //not capturing the reshaping

    bool reshape(int N1b, int N2b, int N3b);

    int N1() const;
    int N2() const;
    int N3() const;
    QDateTime fileLastModified() const;

    ///Retrieve a chunk of the vectorized data of size 1xN starting at position i
    bool readChunk(Mda& X, int i, int size) const;
    bool readChunk32(Mda32& X, int i, int size) const;

private:
    RemoteReadMdaPrivate* d;
};

void unit_test_remote_read_mda_2(const QString& path);
void unit_test_remote_read_mda();

#endif // REMOTEREADMDA_H
