/******************************************************
** See the accompanying README and LICENSE files
** Author(s): Jeremy Magland
** Created: 7/6/2016
*******************************************************/

#ifndef MLNETWORK_H
#define MLNETWORK_H

#include <QFile>
#include <QMutex>
#include <QNetworkReply>
#include <QString>
#include <QThread>
#include <QTime>
#include <taskprogress.h>

namespace MLNetwork {

QString httpGetTextSync(QString url);
QString httpGetBinaryFileSync(QString url);
QString httpPostFileSync(QString file_name, QString url);
QString httpPostFileParallelSync(QString file_name, QString url, int num_threads = 10);

class Runner : public QObject {
    Q_OBJECT
public:
    virtual void start() = 0;
    bool isFinished();
    bool waitForFinished(int msec);
    void requestStop();
    bool stopRequested() const;
signals:
    void finished();

protected:
    void setFinished();

private:
    bool m_is_finished = false;
    bool m_stop_requested = false;
};

class Downloader : public Runner {
    Q_OBJECT
public:
    virtual ~Downloader();

    //input
    QString source_url;
    QString destination_file_name;
    int size = 0; //optional, if known

    //output
    bool success = false;
    QString error;

    void start();
    double elapsed_msec();
    int num_bytes_downloaded();
signals:
    void progress();
private slots:
    void slot_reply_error();
    void slot_reply_ready_read();
    void slot_reply_finished();

private:
    int m_num_bytes_downloaded = 0;
    QTime m_timer;
    TaskProgress m_task; // WW: This dependency shouldn't be here. We can emit signals instead.
    QNetworkReply* m_reply = 0;
    QString m_tmp_fname;
    QFile* m_file = 0;
};

class PrvParallelDownloader : public Runner {
    Q_OBJECT
public:
    virtual ~PrvParallelDownloader();

    //input
    QString source_url; //must be prv protocol
    QString destination_file_name;
    int size = 0; //mandatory
    int num_threads = 10;

    //output
    bool success = true;
    QString error;

    void start();
    double elapsed_msec();
    int num_bytes_downloaded();
private slots:
    void slot_downloader_progress();
    void slot_downloader_finished();
    void slot_concatenate_finished();

private:
    QMutex m_mutex;
    QTime m_timer;
    int m_num_bytes_downloaded = 0;

    TaskProgress m_task;
    QList<Downloader*> m_downloaders;
};

class Uploader : public Runner {
    Q_OBJECT
public:
    virtual ~Uploader();

    //input
    QString source_file_name;
    QString destination_url;
    int start_byte = -1;
    int end_byte = -1;

    //output
    bool success = false;
    QString response_text;
    QString error;

    void start();
    double elapsed_msec();
    int num_bytes_uploaded();
signals:
    void progress();

private:
    QMutex m_mutex;
    int m_num_bytes_uploaded = 0;
    QTime m_timer;

    QNetworkReply* m_reply = 0;
    TaskProgress m_task;
    QFile* m_file = 0;
};

class PrvParallelUploader : public Runner {
    Q_OBJECT
public:
    virtual ~PrvParallelUploader();

    //input
    QString source_file_name;
    QString destination_url;
    int num_threads = 10;

    //output
    bool success = true;
    QString response_text;
    QString error;

    void start();
    double elapsed_msec();
    int num_bytes_uploaded();
private slots:
    void slot_uploader_progress();
    void slot_uploader_finished();
    void slot_concat_upload_finished();

private:
    int m_size = 0;
    int m_num_bytes_uploaded = 0;
    QTime m_timer;
    TaskProgress m_task;

    QList<Uploader*> m_uploaders;
    Downloader m_concat_upload;
};
}

#endif // MLNETWORK_H
