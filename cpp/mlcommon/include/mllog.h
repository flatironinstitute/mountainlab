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
#ifndef MLLOG_H
#define MLLOG_H

#include <QString>
#include <QVariant>

namespace MLLog {

void setLogMode(bool val);
void message(
    QString key1, QVariant value1 = QVariant(),
    QString key2 = "", QVariant value2 = QVariant(),
    QString key3 = "", QVariant value3 = QVariant(),
    QString key4 = "", QVariant value4 = QVariant(),
    QString key5 = "", QVariant value5 = QVariant(),
    QString key6 = "", QVariant value6 = QVariant());
}

#endif // MLLOG_H
