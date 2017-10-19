#ifndef PROCESS_REQUEST_H
#define PROCESS_REQUEST_H

#include <QJsonObject>
#include "processormanager.h"

QJsonObject handle_request(const QJsonObject& request, QString prvbucket_path, ProcessorManager* PM);

//qint64 run_command_as_bash_script(const QString& exe_command, QString monitor_file_name);
QString compute_unique_object_code(QJsonObject obj);
void sleep_msec(int msec);
void touch(const QString& filePath);

void set_terminate_requested(bool val);
bool terminate_requested();

#endif // PROCESS_REQUEST_H
