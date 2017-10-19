/******************************************************
** See the accompanying README and LICENSE files
** Author(s): Jeremy Magland
** Created: 7/6/2016
*******************************************************/

#ifndef MLNETWORK_H
#define MLNETWORK_H

#include <QString>

class MLNetwork {
public:
    static QString httpGetText(const QString& url);
    static QString httpGetBinaryFile(const QString& url);
};

#endif // MLNETWORK_H
