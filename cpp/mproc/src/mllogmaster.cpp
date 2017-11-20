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
#include "mllogmaster.h"
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <cachemanager.h>
#include "mlcommon.h"

#define EXIT_ON_CRITICAL_ERROR
void append_line_to_file_0(const QString& fname, const QString& line_in);
void mountainprocessMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg);

class MLLogMasterPrivate {
public:
    MLLogMaster* q;
};

MLLogMaster::MLLogMaster()
{
    d = new MLLogMasterPrivate;
    d->q = this;
}

MLLogMaster::~MLLogMaster()
{
    delete d;
}

void MLLogMaster::install()
{
    qInstallMessageHandler(mountainprocessMessageOutput);
}

void append_line_to_file_0(const QString& fname, const QString& line_in)
{
    QString line = line_in.split("\n").join("\\n"); //it should really be one line
    QFile ff(fname);
    if (ff.open(QFile::Append | QFile::WriteOnly)) {
        ff.write(line.toUtf8());
        ff.write(QByteArray("\n"));
        ff.close();
    }
    else {
        static bool reported = false;
        if (!reported) {
            reported = true;
            qInfo() << "Unable to write to log file" << fname; //don't use critical or it will exit
        }
    }
}

void append_to_log_file(QString fname, QString str)
{
    bool is_new_file = (!QFile::exists(fname));
    append_line_to_file_0(fname, str);
    if ((is_new_file) && (QFile::exists(fname))) {
        CacheManager::globalInstance()->setTemporaryFileDuration(fname, 60 * 60 * 48);
    }
}

void mountainprocessMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    (void)context;

    QString category = context.category;

    if ((category.isEmpty()) || (category == "default")) {
        QByteArray msg0 = msg.toLocal8Bit();
        switch (type) {
        case QtDebugMsg:
            fprintf(stderr, "%s\n", msg0.constData());
            break;
        case QtInfoMsg:
            fprintf(stderr, "%s\n", msg0.constData());
            break;
        case QtWarningMsg:
            fprintf(stderr, "Warning: %s\n", msg0.constData());
            break;
        case QtCriticalMsg:
            fprintf(stderr, "Critical: %s\n", msg0.constData());
#ifdef EXIT_ON_CRITICAL_ERROR
            exit(-1);
#endif
            break;
        case QtFatalMsg:
            fprintf(stderr, "Fatal: %s\n", msg0.constData());
            exit(-1);
        }
    }
    else {
        // Category is not default
        QString level = "";
        bool exit_needed = false;
        switch (type) {
        case QtDebugMsg:
            level = "debug";
            break;
        case QtInfoMsg:
            level = "info";
            break;
        case QtWarningMsg:
            level = "warning";
            break;
        case QtCriticalMsg:
            level = "critical";
#ifdef EXIT_ON_CRITICAL_ERROR
            exit_needed = true;
#endif
            break;
        case QtFatalMsg:
            level = "fatal";
            exit_needed = true;
        }
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd:hh-mm-ss-zzz");
        QString str = QString("%1\t%2\t%3\t%4").arg(timestamp).arg(category).arg(level).arg(msg);
        if (level != "info")
            fprintf(stderr, "%s\n", str.toUtf8().data());

        {
            QString fname = MLUtil::mlLogPath() + "/" + QString("%1-cat-%2").arg(QDate::currentDate().toString("yyyy-MM-dd")).arg(category);
            append_to_log_file(fname, str);
        }
        if ((level != "info") && (level != "debug")) {
            QString fname = MLUtil::mlLogPath() + "/" + QString("%1-level-%2").arg(QDate::currentDate().toString("yyyy-MM-dd")).arg(level);
            append_to_log_file(fname, str);
        }

        if (exit_needed) {
            exit(-1);
        }
    }
}
