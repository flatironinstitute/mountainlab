#include "mlnetwork.h"

#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <cachemanager.h>
#include "mlcommon.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>

namespace MLNetwork {

static QNetworkAccessManager* s_manager = 0;
QNetworkAccessManager* manager()
{
    if (!s_manager)
        s_manager = new QNetworkAccessManager;
    return s_manager;
}

Downloader::~Downloader()
{
    if (m_file) {
        m_file->close();
        delete m_file;
    }
}

void Downloader::start()
{
    m_task.setLabel("Downloading: " + source_url);

    //initialize private variables
    m_timer.start();
    m_num_bytes_downloaded = 0;
    success = true;

    m_task.setLabel("Downloading " + source_url);

    //make temporary file path
    m_tmp_fname = CacheManager::globalInstance()->makeLocalFile() + ".Downloader";

    //make the http request
    m_reply = manager()->get(QNetworkRequest(QUrl(source_url)));

    QObject::connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(slot_reply_error()));
    QObject::connect(m_reply, SIGNAL(readyRead()), this, SLOT(slot_reply_ready_read()));
    QObject::connect(m_reply, SIGNAL(finished()), this, SLOT(slot_reply_finished()));

    //check for an immediate error (not sure if this ever happens)
    if (m_reply->error() != QNetworkReply::NoError) {
        success = false;
        error = QString("Error in network request (%1): %2").arg(source_url).arg(m_reply->errorString());
        m_task.error() << error;
        m_reply->abort();
        this->setFinished();
        return;
    }

    m_file = new QFile(m_tmp_fname);

    if (!m_file->open(QIODevice::WriteOnly)) {
        success = false;
        error = QString("Error opening temporary file: %1").arg(m_tmp_fname);
        m_task.error() << error;
        m_reply->abort();
        this->setFinished();
        return;
    }
}

class FileChunkReader : public QIODevice {
public:
    FileChunkReader(QFile* file, int start_byte, int end_byte)
    {
        m_file = file;
        m_start_byte = start_byte;
        m_end_byte = end_byte;
        if (m_start_byte >= 0) {
            m_file->seek(m_start_byte);
        }
        this->open(QIODevice::ReadOnly);
    }

    virtual bool isSequential() const
    {
        return false;
    }
    virtual qint64 pos() const
    {
        if (m_start_byte < 0)
            return m_file->pos();
        return m_file->pos() - m_start_byte;
    }
    virtual qint64 size() const
    {
        if (m_start_byte < 0)
            return m_file->size();
        else {
            return m_end_byte - m_start_byte + 1;
        }
    }
    virtual bool seek(qint64 pos)
    {
        if (m_start_byte < 0) {
            return m_file->seek(pos);
        }
        else {
            return m_file->seek(m_start_byte + pos);
        }
    }

    virtual bool atEnd() const
    {
        if (m_end_byte < 0)
            return m_file->atEnd();
        else
            return ((m_file->atEnd()) || (m_file->pos() > m_end_byte));
    }
    virtual qint64 bytesAvailable() const
    {
        if (m_file->bytesAvailable() != m_end_byte - m_file->pos() + 1) {
            qWarning() << __FILE__ << __LINE__ << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
        }

        if (m_end_byte < 0)
            return m_file->bytesAvailable();
        else
            return m_end_byte - m_file->pos() + 1;
    }
    virtual bool waitForReadyRead(int msecs)
    {
        return m_file->waitForReadyRead(msecs);
    }

protected:
    virtual qint64 readData(char* data, qint64 maxSize)
    {
        if (m_start_byte < 0)
            return m_file->read(data, maxSize);
        else {
            if (m_file->pos() + maxSize > m_end_byte) {
                maxSize = m_end_byte - m_file->pos() + 1;
            }
            if (maxSize <= 0)
                return 0;
            qint64 num_read = m_file->read(data, maxSize);
            return num_read;
        }
    }

    virtual qint64 readLineData(char* data, qint64 maxSize)
    {
        Q_UNUSED(data);
        Q_UNUSED(maxSize);
        qWarning() << "readLineData not supported for FileChunkReader";
        return 0;
    }

    void setErrorString(const QString& str)
    {
        QIODevice::setErrorString(str);
    }

    //void setOpenMode(OpenMode openMode) {
    //    m_file->setOpenMode(openMode);
    //}

    virtual qint64 writeData(const char* data, qint64 maxSize)
    {
        Q_UNUSED(data)
        Q_UNUSED(maxSize)
        qWarning() << "writeData not supported for FileChunkReader";
        return 0;
    }

private:
    QFile* m_file = 0;
    int m_start_byte = -1;
    int m_end_byte = -1;
};

Uploader::~Uploader()
{
    if (m_file) {
        m_file->close();
        delete m_file;
    }
}

void Uploader::start()
{
    //initialize private variables
    m_timer.start();
    m_num_bytes_uploaded = 0;
    success = true;
    response_text = "";

    m_task.setLabel("Uploading " + source_file_name + " -> " + destination_url);

    //open the file for reading
    m_file = new QFile(source_file_name);
    if (!m_file->open(QIODevice::ReadOnly)) {
        m_task.error() << "Unable to open file for reading: " + source_file_name;
        return;
    }
    FileChunkReader* FCR = new FileChunkReader(m_file, start_byte, end_byte);

    QNetworkRequest request = QNetworkRequest(destination_url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    int content_length;
    if ((start_byte >= 0) && (end_byte >= 0))
        content_length = end_byte - start_byte + 1;
    else
        content_length = m_file->size();
    request.setHeader(QNetworkRequest::ContentLengthHeader, QString("%1").arg(content_length));
    m_reply = manager()->post(request, FCR);

    //check for an immediate error (not sure if this ever happens)
    if (m_reply->error() != QNetworkReply::NoError) {
        success = false;
        error = QString("Error in upload request (%1 -> %2): %3").arg(source_file_name).arg(destination_url).arg(m_reply->errorString());
        this->setFinished();
        return;
    }

    QObject::connect(m_reply, &QNetworkReply::uploadProgress, [&](qint64 bytesSent, qint64 bytesTotal) {
        if (stopRequested()) {
            m_task.error("Stop requested");
            qWarning() << "Aborting upload.";
            m_reply->abort();
            success=false;
            error="Stopped..";
            return;
        }
        if (!success) {
            //error occurred elsewhere
            qWarning() << "Aborting upload.";
            m_reply->abort();
            return;
        }
        {
            QMutexLocker locker(&m_mutex);
            m_num_bytes_uploaded=bytesSent;
        }
        //if (bytesTotal)
            m_task.setProgress(bytesSent*1.0/bytesTotal);
    });

    QObject::connect(m_reply, &QNetworkReply::readyRead, [&]() {
        if (stopRequested()) {
            m_task.error("Stop requested");
            m_reply->abort();
            success=false;
            error="Stopped.";
            return;
        }
        if (!success) {
            //error occurred elsewhere
            return;
        }
        QByteArray X=m_reply->readAll();
        m_task.log() << X;
        response_text+=X;
    });

    QObject::connect(m_reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), [&]() {
        if (stopRequested()) {
            m_task.error("Stop requested");
            qWarning() << "Aborting upload.";
            m_reply->abort();
            success=false;
            error="Stopped...";
            return;
        }
        success=false;
        error=QString("Error in upload (%1 -> %2): %3").arg(source_file_name).arg(destination_url).arg(m_reply->errorString());
        m_reply->abort();
    });

    //QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    QObject::connect(m_reply, &QNetworkReply::finished, [&]() {
        m_task.log() << "Finished" << response_text;
        if (success) {
            QJsonObject response=QJsonDocument::fromJson(response_text.toUtf8()).object();
            success=response["success"].toBool();
            error=response["error"].toString();
            if (!success) {
                m_task.error() << error;
            }
        }
        setFinished();
    });
}

double Uploader::elapsed_msec()
{
    return m_timer.elapsed();
}

int Uploader::num_bytes_uploaded()
{
    return m_num_bytes_uploaded;
}

double Downloader::elapsed_msec()
{
    return m_timer.elapsed();
}

int Downloader::num_bytes_downloaded()
{
    return m_num_bytes_downloaded;
}

void Downloader::slot_reply_error()
{
    if (isFinished())
        return;
    success = false;
    error = QString("Error in reply (%1): %2").arg(source_url).arg(m_reply->errorString());
    m_task.error() << error;
    m_reply->abort();
    this->setFinished();
}

void Downloader::slot_reply_ready_read()
{
    if (isFinished())
        return;
    if (stopRequested()) {
        success = false;
        error = "Stop requested.";
        this->setFinished();
        return;
    }
    QByteArray X = m_reply->readAll();
    m_file->write(X);
    m_num_bytes_downloaded += X.count();
    if (size)
        m_task.setProgress(m_num_bytes_downloaded * 1.0 / size);
    emit progress();
}

void Downloader::slot_reply_finished()
{
    if (isFinished())
        return;
    if ((size) && (m_num_bytes_downloaded != size)) {
        success = false;
        error = QString("Unexpected number of bytes downloaded %1 <> %2").arg(m_num_bytes_downloaded).arg(size);
        m_task.error(error);
        this->setFinished();
        return;
    }
    {
        m_file->close();
        delete m_file;
        m_file = 0;
    }
    if (!QFile::rename(m_tmp_fname, destination_file_name)) {
        success = false;
        error = "Unable to rename file: " + m_tmp_fname + " " + destination_file_name;
        m_task.error(error);
        this->setFinished();
        return;
    }
    m_task.log() << QString("Downloaded %1 MB in %2 sec").arg(m_num_bytes_downloaded * 1.0 / 1e6).arg(m_timer.elapsed() * 1.0 / 1000);
    this->setFinished();
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
        int chunk_size = 1e5;
        QTime timer;
        timer.start();
        while (!g.atEnd()) {
            QByteArray data0 = g.read(chunk_size);
            f.write(data0);
        }
        g.close();
    }

    f.close();
    return true;
}

class ConcatenateThread : public QThread {
public:
    QStringList file_names;
    QString dest_file_name;

    void run()
    {
        TaskProgress task("Concatenating files");
        task.log() << file_names;
        task.log() << dest_file_name;
        if (!concatenate_files(file_names, dest_file_name)) {
            task.error() << "Problem concatenating files";
            return;
        }
        foreach (QString file, file_names) {
            if (!QFile::remove(file)) {
                qWarning() << "Problem removing file: " + file;
            }
        }
    }
};

void PrvParallelDownloader::start()
{
    m_task.setLabel("Parallel Downloading:::: " + source_url);

    m_timer.start();
    m_num_bytes_downloaded = 0;

    //append the url so it can take some (more) query parameters
    QString url = source_url;
    if (!url.contains("?"))
        url += "?";
    else
        url += "&";

    m_task.log() << "size:" << size << "num_threads:" << num_threads;

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

    m_task.log() << "start_bytes:" << start_bytes;
    m_task.log() << "end_bytes:" << end_bytes;

    for (int i = 0; i < start_bytes.count(); i++) {
        QString url2 = url + QString("bytes=%1-%2&").arg(start_bytes[i]).arg(end_bytes[i]);
        m_task.log() << "Starting downloader: " + url2;
        Downloader* DD = new Downloader;
        DD->source_url = url2;
        DD->destination_file_name = CacheManager::globalInstance()->makeLocalFile() + ".PrvParallelDownloader";
        DD->size = end_bytes[i] - start_bytes[i] + 1;
        m_downloaders << DD;
        connect(DD, SIGNAL(progress()), this, SLOT(slot_downloader_progress()));
        connect(DD, SIGNAL(finished()), this, SLOT(slot_downloader_finished()));
        m_task.log() << "Starting " + url2;
        m_task.log() << "";
        DD->start();
    }

    /*
    printf("Using %d downloaders\n", downloaders.count());
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
    */
}

PrvParallelDownloader::~PrvParallelDownloader()
{
    qDeleteAll(m_downloaders);
}

double PrvParallelDownloader::elapsed_msec()
{
    return m_timer.elapsed();
}

int PrvParallelDownloader::num_bytes_downloaded()
{
    return m_num_bytes_downloaded;
}

void PrvParallelDownloader::slot_downloader_progress()
{
    int num_bytes = 0;
    for (int i = 0; i < m_downloaders.count(); i++) {
        num_bytes += m_downloaders[i]->num_bytes_downloaded();
    }

    if (size)
        m_task.setProgress(num_bytes * 1.0 / size);

    m_num_bytes_downloaded = num_bytes;
}

void PrvParallelDownloader::slot_downloader_finished()
{
    if (this->isFinished())
        return;
    bool all_finished = true;
    for (int i = 0; i < m_downloaders.count(); i++) {
        if (m_downloaders[i]->isFinished()) {
            if (!m_downloaders[i]->success) {
                if (success) {
                    //record the error on only the first non-success
                    success = false;
                    error = m_downloaders[i]->error;
                }
                for (int j = 0; j < m_downloaders.count(); j++) {
                    m_downloaders[j]->requestStop();
                }
            }
        }
        else {
            all_finished = false;
        }
    }
    if (all_finished) {
        //for the response text, either use the one where concatentated=true, or the one where success!=true
        QStringList file_names;
        for (int i = 0; i < m_downloaders.count(); i++) {
            file_names << m_downloaders[i]->destination_file_name;
        }
        m_task.log() << "Concatenating files" << file_names << this->destination_file_name;

        ConcatenateThread* T = new ConcatenateThread;
        T->file_names = file_names;
        T->dest_file_name = this->destination_file_name;
        QObject::connect(T, SIGNAL(finished()), this, SLOT(slot_concatenate_finished()));
        QObject::connect(T, SIGNAL(finished()), T, SLOT(deleteLater()));
        T->start();
    }
}

void PrvParallelDownloader::slot_concatenate_finished()
{
    if (!QFile::exists(this->destination_file_name)) {
        m_task.error() << "Problem concatenating files. File does not exist: " + this->destination_file_name;
        this->setFinished();
        return;
    }
    this->setFinished();
}

PrvParallelUploader::~PrvParallelUploader()
{
    qDeleteAll(m_uploaders);
}

void PrvParallelUploader::start()
{
    m_task.setLabel("Parallel Uploading " + source_file_name + " -> " + destination_url);

    m_timer.start();
    m_num_bytes_uploaded = 0;

    //append the url so it can take some (more) query parameters
    QString url = destination_url;
    if (!url.contains("?"))
        url += "?";
    else
        url += "&";

    url += "a=upload&";

    int size = QFileInfo(source_file_name).size();
    m_size = size;

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

    for (int i = 0; i < start_bytes.count(); i++) {
        QString url2 = url + QString("part=%1_of_%2&").arg(i + 1).arg(start_bytes.count());
        Uploader* UU = new Uploader;
        UU->source_file_name = source_file_name;
        UU->start_byte = start_bytes[i];
        UU->end_byte = end_bytes[i];
        UU->destination_url = url2;
        m_uploaders << UU;
        connect(UU, SIGNAL(progress()), this, SLOT(slot_uploader_progress()));
        connect(UU, SIGNAL(finished()), this, SLOT(slot_uploader_finished()));
        m_task.log() << "Starting " + url2;
        UU->start();
    }
}

double PrvParallelUploader::elapsed_msec()
{
    return m_timer.elapsed();
}

int PrvParallelUploader::num_bytes_uploaded()
{
    return m_num_bytes_uploaded;
}

void PrvParallelUploader::slot_uploader_progress()
{
    int num_bytes = 0;
    for (int i = 0; i < m_uploaders.count(); i++) {
        num_bytes += m_uploaders[i]->num_bytes_uploaded();
    }

    if (m_size)
        m_task.setProgress(num_bytes * 1.0 / m_size);

    m_num_bytes_uploaded = num_bytes;
}

void PrvParallelUploader::slot_uploader_finished()
{
    if (this->isFinished())
        return;
    bool all_finished = true;
    for (int i = 0; i < m_uploaders.count(); i++) {
        if (m_uploaders[i]->isFinished()) {
            if (!m_uploaders[i]->success) {
                if (success) {
                    //record the error on only the first non-success
                    success = false;
                    error = m_uploaders[i]->error;
                    response_text = m_uploaders[i]->response_text;
                }
                for (int j = 0; j < m_uploaders.count(); j++) {
                    m_uploaders[j]->requestStop();
                }
            }
        }
        else {
            all_finished = false;
        }
    }
    if (all_finished) {
        if (success) {
            //append the url so it can take some (more) query parameters
            QString url = destination_url;
            if (!url.contains("?"))
                url += "?";
            else
                url += "&";
            url += "a=concat-upload&";
            url += QString("num_parts=%1&").arg(m_uploaders.count());
            connect(&m_concat_upload, SIGNAL(finished()), this, SLOT(slot_concat_upload_finished()));
            m_task.log() << "Submitting concatenation request: " + url;
            m_concat_upload.destination_file_name = CacheManager::globalInstance()->makeLocalFile();
            m_concat_upload.source_url = url;
            m_concat_upload.start();
        }
        else {
            this->setFinished();
        }
    }
}

void PrvParallelUploader::slot_concat_upload_finished()
{
    m_task.log() << "concat upload finished.";
    if (!m_concat_upload.success) {
        success = false;
        error = m_concat_upload.error;
        m_task.error() << error;
    }
    else {
        m_task.log() << response_text;
        response_text = TextFile::read(m_concat_upload.destination_file_name);
        QJsonObject response = QJsonDocument::fromJson(response_text.toUtf8()).object();
        success = response["success"].toBool();
        error = response["error"].toString();
    }
    this->setFinished();
}

QString httpGetTextSync(QString url)
{
    Downloader downloader;
    downloader.source_url = url;
    downloader.destination_file_name = CacheManager::globalInstance()->makeLocalFile() + ".httpGetTextSync";
    downloader.start();
    downloader.waitForFinished(-1);
    QString ret;
    if (downloader.success) {
        ret = TextFile::read(downloader.destination_file_name);
    }
    else {
        qWarning() << "Problem in httpGetTextSync" << url << downloader.error;
    }
    QFile::remove(downloader.destination_file_name);
    return ret;
}

QString httpGetBinaryFileSync(QString url)
{
    Downloader downloader;
    downloader.source_url = url;
    downloader.destination_file_name = CacheManager::globalInstance()->makeLocalFile() + ".httpGetBinaryFileSync";
    downloader.start();
    downloader.waitForFinished(-1);
    QString ret;
    if (downloader.success) {
        return downloader.destination_file_name;
    }
    else {
        QFile::remove(downloader.destination_file_name);
        qWarning() << "Problem in httpGetBinaryFileSync" << url << downloader.error;
        return "";
    }
}

QString httpPostFileSync(QString file_name, QString url)
{
    Uploader uploader;
    uploader.source_file_name = file_name;
    uploader.destination_url = url;
    uploader.start();
    uploader.waitForFinished(-1);
    if (uploader.success) {
        return uploader.response_text;
    }
    else {
        qWarning() << "Problem in httpPostFileSync" << file_name << url << uploader.error;
        return "";
    }
}

QString httpPostFileParallelSync(QString file_name, QString url, int num_threads)
{
    PrvParallelUploader uploader;
    uploader.source_file_name = file_name;
    uploader.destination_url = url;
    uploader.num_threads = num_threads;
    uploader.start();
    uploader.waitForFinished(-1);
    if (uploader.success) {
        return uploader.response_text;
    }
    else {
        qWarning() << "Problem in httpPostFileParallelSync" << file_name << url << uploader.error;
        return "";
    }
}

bool Runner::isFinished()
{
    return m_is_finished;
}

bool Runner::waitForFinished(int msec)
{
    QTime timer;
    timer.start();
    while ((!m_is_finished) && ((msec < 0) || (timer.elapsed() <= msec))) {
        qApp->processEvents();
    }
    return m_is_finished;
}

void Runner::requestStop()
{
    m_stop_requested = true;
}

bool Runner::stopRequested() const
{
    return m_stop_requested;
}

void Runner::setFinished()
{
    m_is_finished = true;
    emit this->finished();
}
}
