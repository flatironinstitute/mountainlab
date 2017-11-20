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

#include "sumit.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QDir>
#include <QStringList>
#include <QTime>
#include <QDataStream>
#include <sys/stat.h>

QString compute_the_file_hash(const QString& path, int num_bytes)
{
    // Do not printf here!
    QCryptographicHash hash(QCryptographicHash::Sha1);
    QFile FF(path);
    if (!FF.open(QFile::ReadOnly))
        return "";
    int num_bytes_processed = 0;
    while ((!FF.atEnd()) && ((num_bytes == 0) || (num_bytes_processed < num_bytes))) {
        QByteArray tmp = FF.read(10000);
        if (num_bytes != 0) {
            if (num_bytes_processed + tmp.count() > num_bytes) {
                tmp = tmp.mid(0, num_bytes - num_bytes_processed);
            }
        }
        hash.addData(tmp);
        num_bytes_processed += tmp.count();
    }

    QString ret = QString(hash.result().toHex());
    return ret;
}

QString compute_the_string_hash(const QString& str)
{
    QCryptographicHash X(QCryptographicHash::Sha1);
    X.addData(str.toLatin1());
    return QString(X.result().toHex());
}

void create_directory_if_doesnt_exist(const QString& dirname)
{
    QString parent_dirname = QFileInfo(dirname).path();
    QString folder_name = QFileInfo(dirname).fileName();
    if (QDir(parent_dirname).exists(folder_name))
        return;

    if (!parent_dirname.isEmpty()) {
        create_directory_if_doesnt_exist(parent_dirname);
    }
    QDir(parent_dirname).mkdir(folder_name);
}

inline QString read_text_file(const QString& path)
{
    QFile f(path);
    if (!f.open(QFile::ReadOnly | QFile::Text))
        return "";
    QTextStream in(&f);
    return in.readAll();
}
inline void write_text_file(const QString& path, const QString& txt)
{
    QDir().mkpath(QFileInfo(path).dir().absolutePath());
    QFile f(path);
    if (!f.open(QFile::WriteOnly | QFile::Text))
        return;
    QTextStream out(&f);
    out << txt;
}

void create_hash_file(const QString& path, const QString& hash_path)
{
    QString the_hash = compute_the_file_hash(path, 0);
    write_text_file(hash_path, the_hash);
}

QString sumit(const QString& path, int num_bytes, const QString& temporary_path)
{
    if (num_bytes != 0) {
        return compute_the_file_hash(path, num_bytes);
    }
    //the file id is a hashed function of device,inode,size, and modification time (in seconds)
    //note that it is not dependent on the file name
    struct stat SS;
    stat(path.toLatin1().data(), &SS);

    //didn't compile on mac so using the following which only depends on size and modification date

    QFileInfo info(path);
    QString id_string = QString("%1:%2:%3:%4").arg(path).arg(info.size()).arg(info.lastModified().toMSecsSinceEpoch()).arg(info.size());

    //QString id_string = QString("%1:%2:%3:%4").arg(SS.st_dev).arg(SS.st_ino).arg(SS.st_size).arg(SS.st_mtim.tv_sec);
    QString file_id = compute_the_string_hash(id_string);

    QString dirname = QString(temporary_path + "/sumit/sha1/%1").arg(file_id.mid(0, 4));
    create_directory_if_doesnt_exist(dirname);
    QString hash_path = QString("%1/%2").arg(dirname).arg(file_id);

    QString hash_sum = read_text_file(hash_path);
    if (hash_sum.isEmpty()) {
        create_hash_file(path, hash_path);
        hash_sum = read_text_file(hash_path);
    }
    if (hash_sum.count() != 40) {
        create_hash_file(path, hash_path);
        hash_sum = read_text_file(hash_path);
    }
    return hash_sum;
}

QString sumit_dir(const QString& path, const QString& temporary_path)
{
    QStringList files = QDir(path).entryList(QStringList("*"), QDir::Files, QDir::Name);
    QStringList dirs = QDir(path).entryList(QStringList("*"), QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    QString str = "";
    for (int i = 0; i < dirs.count(); i++) {
        str += QString("%1 %2\n").arg(sumit_dir(path + "/" + dirs[i], temporary_path)).arg(dirs[i]);
    }
    for (int i = 0; i < files.count(); i++) {
        str += QString("%1 %2\n").arg(sumit(path + "/" + files[i], 0, temporary_path)).arg(files[i]);
    }

    return compute_the_string_hash(str);
}
