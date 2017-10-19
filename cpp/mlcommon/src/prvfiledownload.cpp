/******************************************************
** See the accompanying README and LICENSE files
** Author(s): Jeremy Magland
** Created: 9/29/2016
*******************************************************/

#include "prvfiledownload.h"

#include <QEventLoop>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <QString>
#include <cachemanager.h>
#include <taskprogress.h>
#include "mlcommon.h"

namespace PrvFileDownload {

void Downloader::run()
{
    {
        QMutexLocker locker(&m_mutex);
        m_timer.start();
        m_num_bytes_downloaded = 0;
    }

    TaskProgress task("Downloading " + source_url);

    QString tmp_fname = CacheManager::globalInstance()->makeLocalFile() + ".Downloader";
    QNetworkAccessManager manager; // better make it a singleton
    QNetworkReply* reply = manager.get(QNetworkRequest(QUrl(source_url)));

    QEventLoop loop;
    QFile temp(tmp_fname);

    int num_bytes = 0;
    QTime timer;
    timer.start();

    if (!temp.open(QIODevice::WriteOnly)) {
        reply->abort();
        success = false;
        return;
    }
    QObject::connect(reply, &QNetworkReply::readyRead, [&]() {
        if (MLUtil::threadInterruptRequested()) {
            task.error("Thread interrupt requested");
            reply->abort();
        }
        QByteArray X=reply->readAll();
        temp.write(X);
        num_bytes+=X.count();
        {
            QMutexLocker locker(&m_mutex);
            m_num_bytes_downloaded=num_bytes;
        }
        if (size)
            task.setProgress(num_bytes*1.0/size);
    });
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    //TaskManager::TaskProgressMonitor::globalInstance()->incrementQuantity("bytes_downloaded", num_bytes);
    if (MLUtil::threadInterruptRequested()) {
        success = false;
        error = "Thread interrupted.";
        task.error(error);
        return;
    }
    if (!QFile::rename(tmp_fname, destination_file_name)) {
        success = false;
        error = "Unable to rename file: " + tmp_fname + " " + destination_file_name;
        task.error(error);
        return;
    }
    task.setLabel(QString("Downloaded %1 MB in %2 sec").arg(num_bytes * 1.0 / 1e6).arg(timer.elapsed() * 1.0 / 1000));
    success = true;
}

double Downloader::elapsed_msec()
{
    QMutexLocker locker(&m_mutex);
    return m_timer.elapsed();
}

int Downloader::num_bytes_downloaded()
{
    QMutexLocker locker(&m_mutex);
    return m_num_bytes_downloaded;
}

bool concatenate_files(QStringList file_paths, QString dest_path)
{
    /// Witold, this function should be improved by streaming the read/writes
    foreach (QString str, file_paths) {
        if (str.isEmpty())
            return false;
    }

    QFile f(dest_path);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << __FUNCTION__ << "Unable to open file for writing: " + dest_path;
        return false;
    }
    foreach (QString str, file_paths) {
        QFile g(str);
        if (!g.open(QIODevice::ReadOnly)) {
            qWarning() << __FUNCTION__ << "Unable to open file for reading: " + str;
            f.close();
            return false;
        }
        f.write(g.readAll());
        g.close();
    }

    f.close();
    return true;
}

void PrvParallelDownloader::run()
{
    TaskProgress task("Parallel Downloading " + source_url);

    {
        QMutexLocker locker(&m_mutex);
        m_timer.start();
        m_num_bytes_downloaded = 0;
    }

    //append the url so it can take some (more) query parameters
    QString url = source_url;
    if (!url.contains("?"))
        url += "?";
    else
        url += "&";

    //get the start and end bytes
    QList<int> start_bytes;
    QList<int> end_bytes;
    int incr = (int)(1 + size * 1.0 / num_threads);
    if (incr < 1000)
        incr = 1000; //let's be a bit reasonable
    int sum = 0;
    for (int i = 0; i < num_threads; i++) {
        if (sum < size) {
            int val = qMin(incr, size - sum);
            start_bytes << sum;
            end_bytes << sum + val - 1;
            sum += val;
        }
    }

    QList<Downloader*> downloaders;
    for (int i = 0; i < start_bytes.count(); i++) {
        QString url2 = url + QString("bytes=%1-%2").arg(start_bytes[i]).arg(end_bytes[i]);
        Downloader* DD = new Downloader;
        DD->source_url = url2;
        DD->destination_file_name = CacheManager::globalInstance()->makeLocalFile() + ".PrvParallelDownloader";
        DD->size = end_bytes[i] - start_bytes[i] + 1;
        downloaders << DD;
        task.log() << "Starting " + url2;
        DD->start();
    }

    printf("Using %d downloaders", downloaders.count());
    bool done = false;
    success = true;
    while ((!done) && (success)) {
        done = true;
        if (MLUtil::threadInterruptRequested()) {
            task.error("Thread interrupt requested");
            for (int j = 0; j < downloaders.count(); j++) {
                downloaders[j]->requestInterruption();
            }
            success = false;
            error = "Thread interrupted";
        }
        for (int i = 0; i < downloaders.count(); i++) {
            if (downloaders[i]->wait(100)) {
                if (downloaders[i]->success) {
                    task.log() << "Thread finished: " + downloaders[i]->source_url;
                }
                else {
                    task.error() << "Error in download thread: " + downloaders[i]->source_url + ": " + downloaders[i]->error;
                    for (int j = 0; j < downloaders.count(); j++) {
                        downloaders[j]->requestInterruption();
                    }
                    success = false;
                    error = "Error in download thread: " + downloaders[i]->source_url + ": " + downloaders[i]->error;
                }
            }
            else {
                done = false;
            }
        }
        int num_bytes = 0;
        for (int i = 0; i < downloaders.count(); i++) {
            num_bytes += downloaders[i]->num_bytes_downloaded();
        }
        if (size)
            task.setProgress(num_bytes * 1.0 / size);
        {
            QMutexLocker locker(&m_mutex);
            m_num_bytes_downloaded = num_bytes;
        }
    }
    if (!success) {
        for (int i = 0; i < downloaders.count(); i++) {
            QFile::remove(downloaders[i]->destination_file_name);
        }
        qDeleteAll(downloaders);
        return;
    }

    QStringList paths;
    for (int i = 0; i < downloaders.count(); i++) {
        paths << downloaders[i]->destination_file_name;
    }
    qDeleteAll(downloaders);

    if (!concatenate_files(paths, destination_file_name)) {
        success = false;
        error = "Problem concatenating files.";
    }

    //clean up
    foreach (QString fname, paths) {
        QFile::remove(fname);
    }
}

double PrvParallelDownloader::elapsed_msec()
{
    QMutexLocker locker(&m_mutex);
    return m_timer.elapsed();
}

int PrvParallelDownloader::num_bytes_downloaded()
{
    QMutexLocker locker(&m_mutex);
    return m_num_bytes_downloaded;
}
}
