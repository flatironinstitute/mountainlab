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

bool list_processors();
bool spec(QString arg2);
bool test_processor(QString arg2);
int exec_run_or_queue(QString arg1, QString arg2, const QMap<QString, QVariant>& clp);

void print_usage();
void launch_process_and_wait(const MLProcessor& MLP, const QMap<QString, QVariant>& clp, QString monitor_file_name, MLProcessInfo& info);
bool process_already_completed(const MLProcessor& MLP, const QMap<QString, QVariant>& clp);
void record_completed_process(const MLProcessor& MLP, const QMap<QString, QVariant>& clp);
QString wait_until_ready_to_run(const MLProcessor& MLP, const QMap<QString, QVariant>& clp, bool* already_completed, bool force_run); //returns monitor file name
void write_process_output_file(QString fname, const MLProcessInfo& info);

#endif // MPROCMAIN_H
