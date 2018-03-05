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
#ifndef MPROCMAIN_H
#define MPROCMAIN_H

#include "processormanager.h"

#include <QString>
#include <QVariant>

struct MLProcessInfo {
    QDateTime start_time;
    QDateTime finish_time;
    QString processor_name;
    QVariantMap parameters;
    QString exe_command;
    int exit_code = 0;
    QString error;
    QString console_output;
};

void sig_handler(int signum);

bool list_processors(const QString &pattern, QVariantMap clp);
bool spec(QString arg2, QVariantMap clp, bool human = false);
bool requirements(QString arg2, const QMap<QString, QVariant> &clp = QMap<QString, QVariant>());
bool test_processor(QString arg2, QVariantMap clp);
int exec_run_or_queue(QString arg1, QString arg2, const QMap<QString, QVariant>& clp);

void print_usage();
void launch_process_and_wait(const MLProcessor& MLP, const QMap<QString, QVariant>& clp, QString monitor_file_name, MLProcessInfo& info, bool requirements_only);
bool process_already_completed(const MLProcessor& MLP, const QMap<QString, QVariant>& clp);
void record_completed_process(const MLProcessor& MLP, const QMap<QString, QVariant>& clp);
QString wait_until_ready_to_run(const MLProcessor& MLP, const QMap<QString, QVariant>& clp, bool* already_completed, bool force_run); //returns monitor file name
void write_process_output_file(QString fname, const MLProcessInfo& info);

#endif // MPROCMAIN_H
