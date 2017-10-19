/******************************************************
** See the accompanying README and LICENSE files
** Author(s): Jeremy Magland
** Created: 9/29/2016
*******************************************************/
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
