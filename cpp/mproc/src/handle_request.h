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
