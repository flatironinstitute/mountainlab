/******************************************************
** See the accompanying README and LICENSE files
** Author(s): Jeremy Magland
** Created: 7/6/2016
*******************************************************/

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
