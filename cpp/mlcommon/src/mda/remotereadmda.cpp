/******************************************************
** See the accompanying README and LICENSE files
** Author(s): Jeremy Magland
** Created: 4/2/2016
*******************************************************/

#include "remotereadmda.h"
#include "diskreadmda.h"
#include <QStringList>
#include <QDir>
#include <QDateTime>
#include <mlnetwork.h>
#include <mda32.h>
#include <diskreadmda32.h>
#include "cachemanager.h"
#include "mlcommon.h"

#define REMOTE_READ_MDA_CHUNK_SIZE 5e5

struct RemoteReadMdaInfo {
    RemoteReadMdaInfo()
    {
        N1 = N2 = N3 = 0;
    }

    int N1, N2, N3;
    QString checksum;
    QDateTime file_last_modified;
};

class RemoteReadMdaPrivate {
public:
    RemoteReadMda* q;

    QString m_path;
    RemoteReadMdaInfo m_info;
    bool m_reshaped;
    bool m_info_downloaded;
    QString m_remote_datatype;
    int m_download_chunk_size;
    bool m_download_failed; //don't make excessive calls. Once we failed, that's it.

    void construct_and_clear();
    void copy_from(const RemoteReadMda& other);
    void download_info_if_needed();
    QString download_chunk_at_index(int ii);
};

RemoteReadMda::RemoteReadMda(const QString& path)
{
    d = new RemoteReadMdaPrivate;
    d->q = this;
    d->construct_and_clear();
    this->setPath(path);
}

RemoteReadMda::RemoteReadMda(const RemoteReadMda& other)
{
    d = new RemoteReadMdaPrivate;
    d->q = this;
    d = new RemoteReadMdaPrivate;
    d->q = this;
    d->copy_from(other);
}

void RemoteReadMda::operator=(const RemoteReadMda& other)
{
    d->copy_from(other);
}

RemoteReadMda::~RemoteReadMda()
{
    delete d;
}

void RemoteReadMda::setRemoteDataType(QString dtype)
{
    d->m_remote_datatype = dtype;
}

void RemoteReadMda::setDownloadChunkSize(int size)
{
    d->m_download_chunk_size = size;
}

int RemoteReadMda::downloadChunkSize()
{
    return d->m_download_chunk_size;
}

void RemoteReadMda::setPath(const QString& file_path)
{
    d->construct_and_clear();

    d->m_path = file_path;
}

QString RemoteReadMda::makePath() const
{
    return d->m_path;
}

bool RemoteReadMda::reshape(int N1b, int N2b, int N3b)
{
    if (this->N1() * this->N2() * this->N3() != N1b * N2b * N3b)
        return false;
    d->download_info_if_needed();
    d->m_info.N1 = N1b;
    d->m_info.N2 = N2b;
    d->m_info.N3 = N3b;
    d->m_reshaped = true;
    return true;
}

int RemoteReadMda::N1() const
{
    d->download_info_if_needed();
    return d->m_info.N1;
}

int RemoteReadMda::N2() const
{
    d->download_info_if_needed();
    return d->m_info.N2;
}

int RemoteReadMda::N3() const
{
    d->download_info_if_needed();
    return d->m_info.N3;
}

QDateTime RemoteReadMda::fileLastModified() const
{
    d->download_info_if_needed();
    return d->m_info.file_last_modified;
}

QString format_num(double num_entries)
{
    if (num_entries <= 500)
        return QString("%1").arg(num_entries);
    if (num_entries < 1000 * 1e3)
        return QString("%1K").arg(num_entries / 1e3, 0, 'f', 2);
    if (num_entries < 1000 * 1e6)
        return QString("%1M").arg(num_entries / 1e6, 0, 'f', 2);
    return QString("%1G").arg(num_entries / 1e9, 0, 'f', 2);
}

bool RemoteReadMda::readChunk(Mda& X, int i, int size) const
{
    if (d->m_download_failed) {
        //don't make excessive calls... once we fail, that's it.
        return false;
    }
    //double size_mb = size * datatype_size * 1.0 / 1e6;
    //TaskProgress task(TaskProgress::Download, "Downloading array chunk");
    //if (size_mb > 0.5) {
    //    task.setLabel(QString("Downloading array chunk: %1 MB").arg(size_mb));
    //}
    //read a chunk of the remote array considered as a 1D array

    TaskProgress task(TaskProgress::Download, QString("Downloading %1 numbers - %2 (%3x%4x%5)").arg(format_num(size)).arg(d->m_remote_datatype).arg(N1()).arg(N2()).arg(N3()));
    task.log() << "Reading chunk:" << this->makePath() << i << size;

    X.allocate(size, 1); //allocate the output array
    double* Xptr = X.dataPtr(); //pointer to the output data
    int ii1 = i; //start index of the remote array
    int ii2 = i + size - 1; //end index of the remote array
    int jj1 = ii1 / d->m_download_chunk_size; //start chunk index of the remote array
    int jj2 = ii2 / d->m_download_chunk_size; //end chunk index of the remote array
    if (jj1 == jj2) { //in this case there is only one chunk we need to worry about
        task.setProgress(0.2);
        QString fname = d->download_chunk_at_index(jj1); //download the single chunk
        if (fname.isEmpty()) {
            //task.error("fname is empty");
            if (!MLUtil::threadInterruptRequested()) {
                TaskProgress errtask("Download chunk at index");
                errtask.log() << QString("m_remote_data_type = %1, download chunk size = %2").arg(d->m_remote_datatype).arg(d->m_download_chunk_size);
                errtask.log() << d->m_path;
                errtask.error() << QString("Failed to download chunk at index %1 *").arg(jj1);
                d->m_download_failed = true;
            }
            return false;
        }
        DiskReadMda A(fname);
        A.readChunk(X, ii1 - jj1 * d->m_download_chunk_size, size); //starting reading at the offset of ii1 relative to the start index of the chunk
        return true;
    }
    else {
        for (int jj = jj1; jj <= jj2; jj++) { //otherwise we need to step through the chunks
            task.setProgress((jj - jj1 + 0.5) / (jj2 - jj1 + 1));
            if (MLUtil::threadInterruptRequested()) {
                //X = Mda(); //maybe it's better to return the right size.
                //task.error("Halted");
                return false;
            }
            QString fname = d->download_chunk_at_index(jj); //download the chunk at index jj
            if (fname.isEmpty()) {
                //task.error("fname is empty *");
                return false;
            }
            DiskReadMda A(fname);
            if (jj == jj1) { //case 1/3, this is the first chunk
                Mda tmp;
                int size0 = (jj1 + 1) * d->m_download_chunk_size - ii1; //the size is going to be the difference between ii1 and the start index of the next chunk
                A.readChunk(tmp, ii1 - jj1 * d->m_download_chunk_size, size0); //again we start reading at the offset of ii1 relative to the start index of the chunk
                double* tmp_ptr = tmp.dataPtr(); //copy the data directly from tmp_ptr to Xptr
                const int b = 0;
                std::copy(tmp_ptr, tmp_ptr + size0, Xptr + b);
                //                for (int a = 0; a < size0; a++) {
                //                    Xptr[b] = tmp_ptr[a];
                //                    b++;
                //                }
            }
            else if (jj == jj2) { //case 2/3, this is the last chunk
                Mda tmp;
                int size0 = ii2 + 1 - jj2 * d->m_download_chunk_size; //the size is going to be the difference between the start index of the last chunk and ii2+1
                A.readChunk(tmp, 0, size0); //we start reading at position zero
                double* tmp_ptr = tmp.dataPtr();
                //copy the data to the last part of X
                const int b = size - size0;
                std::copy(tmp_ptr, tmp_ptr + size0, Xptr + b);
                //                for (int a = 0; a < size0; a++) {
                //                    Xptr[b] = tmp_ptr[a];
                //                    b++;
                //                }
            }

            /*
             ///this was the old code, which was wrong!!!!!!!!! -- fixed on 5/16/2016
            else if (jj == jj2) {
                Mda tmp;
                int size0 = ii2 + 1 - jj2 * d->m_download_chunk_size;
                A.readChunk(tmp, d->m_download_chunk_size - size0, size0);
                double* tmp_ptr = tmp.dataPtr();
                int b = jj2 * d->m_download_chunk_size - ii1;
                for (int a = 0; a < size0; a++) {
                    Xptr[b] = tmp_ptr[a];
                    b++;
                }
            }
            */
            else { //case 3/3, this is a middle chunk
                Mda tmp;
                A.readChunk(tmp, 0, d->m_download_chunk_size); //read the entire chunk, because we'll use it all
                double* tmp_ptr = tmp.dataPtr();
                int b = jj * d->m_download_chunk_size - ii1; //we start writing at the offset between the start index of the chunk and the start index
                std::copy(tmp_ptr, tmp_ptr + d->m_download_chunk_size, Xptr + b);
                //                for (int a = 0; a < d->m_download_chunk_size; a++) {
                //                    Xptr[b] = tmp_ptr[a];
                //                    b++;
                //                }
            }
        }
        return true;
    }
}

bool RemoteReadMda::readChunk32(Mda32& X, int i, int size) const
{
    if (d->m_download_failed) {
        //don't make excessive calls... once we fail, that's it.
        return false;
    }
    //double size_mb = size * datatype_size * 1.0 / 1e6;
    //TaskProgress task(TaskProgress::Download, "Downloading array chunk");
    //if (size_mb > 0.5) {
    //    task.setLabel(QString("Downloading array chunk: %1 MB").arg(size_mb));
    //}
    //read a chunk of the remote array considered as a 1D array

    TaskProgress task(TaskProgress::Download, QString("Downloading %1 numbers -- %2 (%3x%4x%5)").arg(format_num(size)).arg(d->m_remote_datatype).arg(N1()).arg(N2()).arg(N3()));
    task.log(this->makePath());

    X.allocate(size, 1); //allocate the output array
    dtype32* Xptr = X.dataPtr(); //pointer to the output data
    int ii1 = i; //start index of the remote array
    int ii2 = i + size - 1; //end index of the remote array
    int jj1 = ii1 / d->m_download_chunk_size; //start chunk index of the remote array
    int jj2 = ii2 / d->m_download_chunk_size; //end chunk index of the remote array
    if (jj1 == jj2) { //in this case there is only one chunk we need to worry about
        task.setProgress(0.2);
        QString fname = d->download_chunk_at_index(jj1); //download the single chunk
        if (fname.isEmpty()) {
            //task.error("fname is empty");
            if (!MLUtil::threadInterruptRequested()) {
                TaskProgress errtask("Download chunk at index");
                errtask.log(QString("m_remote_data_type = %1, download chunk size = %2").arg(d->m_remote_datatype).arg(d->m_download_chunk_size));
                errtask.log(d->m_path);
                errtask.error(QString("Failed to download chunk at index %1 -").arg(jj1));
                d->m_download_failed = true;
            }
            return false;
        }
        DiskReadMda32 A(fname);
        A.readChunk(X, ii1 - jj1 * d->m_download_chunk_size, size); //starting reading at the offset of ii1 relative to the start index of the chunk
        return true;
    }
    else {
        for (int jj = jj1; jj <= jj2; jj++) { //otherwise we need to step through the chunks
            task.setProgress((jj - jj1 + 0.5) / (jj2 - jj1 + 1));
            if (MLUtil::threadInterruptRequested()) {
                //X = Mda32(); //maybe it's better to return the right size.
                //task.error("Halted");
                return false;
            }
            QString fname = d->download_chunk_at_index(jj); //download the chunk at index jj
            if (fname.isEmpty()) {
                //task.error("fname is empty *");
                return false;
            }
            DiskReadMda32 A(fname);
            if (jj == jj1) { //case 1/3, this is the first chunk
                Mda32 tmp;
                int size0 = (jj1 + 1) * d->m_download_chunk_size - ii1; //the size is going to be the difference between ii1 and the start index of the next chunk
                A.readChunk(tmp, ii1 - jj1 * d->m_download_chunk_size, size0); //again we start reading at the offset of ii1 relative to the start index of the chunk
                dtype32* tmp_ptr = tmp.dataPtr(); //copy the data directly from tmp_ptr to Xptr
                std::copy(tmp_ptr, tmp_ptr + size0, Xptr);
                //                int b = 0;
                //                for (int a = 0; a < size0; a++) {
                //                    Xptr[b] = tmp_ptr[a];
                //                    b++;
                //                }
            }
            else if (jj == jj2) { //case 2/3, this is the last chunk
                Mda32 tmp;
                int size0 = ii2 + 1 - jj2 * d->m_download_chunk_size; //the size is going to be the difference between the start index of the last chunk and ii2+1
                A.readChunk(tmp, 0, size0); //we start reading at position zero
                dtype32* tmp_ptr = tmp.dataPtr();
                //copy the data to the last part of X
                int b = size - size0;
                std::copy(tmp_ptr, tmp_ptr + size0, Xptr + b);
                //                for (int a = 0; a < size0; a++) {
                //                    Xptr[b] = tmp_ptr[a];
                //                    b++;
                //                }
            }

            /*
             ///this was the old code, which was wrong!!!!!!!!! -- fixed on 5/16/2016
            else if (jj == jj2) {
                Mda32 tmp;
                int size0 = ii2 + 1 - jj2 * d->m_download_chunk_size;
                A.readChunk(tmp, d->m_download_chunk_size - size0, size0);
                dtype32* tmp_ptr = tmp.dataPtr();
                int b = jj2 * d->m_download_chunk_size - ii1;
                for (int a = 0; a < size0; a++) {
                    Xptr[b] = tmp_ptr[a];
                    b++;
                }
            }
            */
            else { //case 3/3, this is a middle chunk
                Mda32 tmp;
                A.readChunk(tmp, 0, d->m_download_chunk_size); //read the entire chunk, because we'll use it all
                dtype32* tmp_ptr = tmp.dataPtr();
                //const int b = jj * d->m_download_chunk_size - ii1; //we start writing at the offset between the start index of the chunk and the start index
                std::copy(tmp_ptr, tmp_ptr + d->m_download_chunk_size, Xptr);
                //                for (int a = 0; a < d->m_download_chunk_size; a++) {
                //                    Xptr[b] = tmp_ptr[a];
                //                    b++;
                //                }
            }
        }
        return true;
    }
}

void RemoteReadMdaPrivate::construct_and_clear()
{
    this->m_download_chunk_size = REMOTE_READ_MDA_CHUNK_SIZE;
    this->m_download_failed = false;
    this->m_info = RemoteReadMdaInfo();
    this->m_info_downloaded = false;
    this->m_path = "";
    /// TODO (LOW) use enum instead of string "float64", "float32", etc
    this->m_remote_datatype = "float64";
    this->m_reshaped = false;
}

void RemoteReadMdaPrivate::copy_from(const RemoteReadMda& other)
{
    this->m_download_chunk_size = other.d->m_download_chunk_size;
    this->m_download_failed = other.d->m_download_failed;
    this->m_info = other.d->m_info;
    this->m_info_downloaded = other.d->m_info_downloaded;
    this->m_path = other.d->m_path;
    this->m_remote_datatype = other.d->m_remote_datatype;
    this->m_reshaped = other.d->m_reshaped;
}

void RemoteReadMdaPrivate::download_info_if_needed()
{
    if (m_info_downloaded)
        return;
    //if (in_gui_thread()) {
    //    qCritical() << "Cannot call download_info_if_needed from within the GUI thread!";
    //    exit(-1);
    //}
    m_info_downloaded = true;
    //QString url=file_url_for_remote_path(m_path);
    QString url = m_path;
    QString url2 = url + "?a=info&output=text";
    QString txt = MLNetwork::httpGetTextSync(url2);
    QStringList lines = txt.split("\n");
    QStringList sizes = lines.value(0).split(",");
    m_info.N1 = sizes.value(0).toLong();
    m_info.N2 = sizes.value(1).toLong();
    m_info.N3 = sizes.value(2).toLong();
    m_info.checksum = lines.value(1);
    m_info.file_last_modified = QDateTime::fromMSecsSinceEpoch(lines.value(2).toLong());
}

void unquantize8(Mda& X, double minval, double maxval);
QString RemoteReadMdaPrivate::download_chunk_at_index(int ii)
{
    TaskProgress task(QString("Download chunk at index %1 ---").arg(ii));
    download_info_if_needed();
    int Ntot = m_info.N1 * m_info.N2 * m_info.N3;
    int size = m_download_chunk_size;
    if (ii * m_download_chunk_size + size > Ntot) {
        size = Ntot - ii * m_download_chunk_size;
    }
    if (size <= 0) {
        task.log() << m_info.N1 << m_info.N2 << m_info.N3 << Ntot << m_download_chunk_size << ii;
        task.error() << "Size is:" << size;
        return "";
    }
    if (m_info.checksum.isEmpty()) {
        task.error() << "Info checksum is empty";
        return "";
    }
    QString file_name = m_info.checksum + "-" + QString("%1-%2").arg(m_download_chunk_size).arg(ii);
    QString fname = CacheManager::globalInstance()->makeLocalFile(file_name, CacheManager::ShortTerm);
    if (QFile::exists(fname))
        return fname;
    QString url = m_path;
    QString url0 = url + QString("?a=readChunk&output=text&index=%1&size=%2&datatype=%3").arg((int)(ii * m_download_chunk_size)).arg(size).arg(m_remote_datatype);
    QString binary_url = MLNetwork::httpGetTextSync(url0).trimmed();
    if (binary_url.isEmpty())
        return "";

    //the following is ugly
    int ind = m_path.indexOf("/mdaserver");
    if (ind > 0) {
        binary_url = m_path.mid(0, ind) + "/mdaserver/" + binary_url;
    }

    task.log() << "binary_url:" << binary_url;
    QString tmp_mda_fname = MLNetwork::httpGetBinaryFileSync(binary_url);
    if (tmp_mda_fname.isEmpty())
        return "";
    DiskReadMda tmp(tmp_mda_fname);
    if (tmp.totalSize() != size) {
        task.error() << "Unexpected total size problem: " << tmp.totalSize() << size;
        qWarning() << "Unexpected total size problem: " << tmp.totalSize() << size;
        QFile::remove(tmp_mda_fname);
        return "";
    }
    if (m_remote_datatype == "float32_q8") {
        QString dynamic_range_fname = MLNetwork::httpGetBinaryFileSync(binary_url + ".q8");
        if (dynamic_range_fname.isEmpty()) {
            qWarning() << "problem downloading .q8 file: " + binary_url + ".q8";
            task.error() << "problem downloading .q8 file: " + binary_url + ".q8";
            return "";
        }
        Mda dynamic_range(dynamic_range_fname);
        if (dynamic_range.totalSize() != 2) {
            qWarning() << QString("Problem in .q8 file. Unexpected size %1: ").arg(dynamic_range.totalSize()) + binary_url + ".q8";
            task.error() << QString("Problem in .q8 file. Unexpected size %1: ").arg(dynamic_range.totalSize()) + binary_url + ".q8";
            return "";
        }
        Mda chunk(tmp_mda_fname);
        unquantize8(chunk, dynamic_range.value(0), dynamic_range.value(1));
        if (!chunk.write32(fname)) {
            QFile::remove(tmp_mda_fname);
            qWarning() << "Unable to write file: " + fname;
            task.error() << "Unable to write file: " + fname;
            return "";
        }
        QFile::remove(tmp_mda_fname);
    }
    else {
        if (!QFile::rename(tmp_mda_fname, fname)) {
            QFile::remove(tmp_mda_fname);
            qWarning() << "Unable to rename file: " << tmp_mda_fname << fname;
            task.error() << "Unable to rename file: " << tmp_mda_fname << fname;
            return "";
        }
    }
    return fname;
}

void unit_test_remote_read_mda()
{
    QString url = "http://localhost:8000/firings.mda";
    RemoteReadMda X(url);
    Mda chunk;
    X.readChunk(chunk, 0, 100);
    for (int j = 0; j < 10; j++) {
        qDebug().noquote() << j << chunk.value(j); // unit_test
    }
}

#include "diskreadmda.h"
void unit_test_remote_read_mda_2(const QString& path)
{
    // run this test by calling
    // > mountainview unit_test remotereadmda2

    DiskReadMda X(path);
    for (int j = 0; j < 20; j++) {
        printf("%d: ", j);
        for (int i = 0; i < X.N1(); i++) {
            printf("%g, ", X.value(i, j));
        }
        printf("\n");
    }
}

void unquantize8(Mda& X, double minval, double maxval)
{
    int N = X.totalSize();
    double* Xptr = X.dataPtr();
    for (int i = 0; i < N; i++) {
        Xptr[i] = minval + (Xptr[i] / 255) * (maxval - minval);
    }
}
