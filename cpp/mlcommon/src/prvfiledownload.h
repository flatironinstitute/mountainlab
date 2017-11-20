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
#ifndef PRVFILEDOWNLOAD_H
#define PRVFILEDOWNLOAD_H

#include <QMutex>
#include <QString>
#include <QThread>
#include <QTime>

namespace PrvFileDownload {

class Downloader : public QThread {
public:
    //input
    QString source_url;
    QString destination_file_name;
    int size = 0; //optional, if known

    //output
    bool success = false;
    QString error;

    void run();
    double elapsed_msec();
    int num_bytes_downloaded();

private:
    QMutex m_mutex;
    int m_num_bytes_downloaded = 0;
    QTime m_timer;
};

class PrvParallelDownloader : public QThread {
public:
    //input
    QString source_url; //must be prv protocol
    QString destination_file_name;
    int size = 0; //mandatory
    int num_threads = 10;

    //output
    bool success = false;
    QString error;

    void run();
    double elapsed_msec();
    int num_bytes_downloaded();

private:
    QMutex m_mutex;
    QTime m_timer;
    int m_num_bytes_downloaded = 0;
};
}

#endif // PRVFILEDOWNLOAD_H
