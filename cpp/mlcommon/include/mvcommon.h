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

#ifndef MVCOMMON_H
#define MVCOMMON_H

#include <QTextCodec>
#include <QDebug>

class TextFile {
public:
    static QString read(const QString& fname, QTextCodec* codec = 0);
    static bool write(const QString& fname, const QString& txt, QTextCodec* codec = 0);
};

class MVUtil {
public:
    static QString makeRandomId(int numchars = 10);
    static bool threadInterruptRequested();
    static bool inGuiThread();
    static QString tempPath();
};

class MLNetwork {
public:
    static QString httpGetText(const QString& url);
    static QString httpGetBinaryFile(const QString& url);
};

#endif // TEXTFILE_H
