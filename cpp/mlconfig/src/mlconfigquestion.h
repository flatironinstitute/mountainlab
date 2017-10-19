/******************************************************
** See the accompanying README and LICENSE files
** Author(s): Jeremy Magland
** Created: 3/8/2017
*******************************************************/
#ifndef MLCONFIGQUESTION_H
#define MLCONFIGQUESTION_H

#include <QJsonObject>

class MLConfigPage;
class MLConfigQuestionPrivate;
class MLConfigQuestion {
public:
    friend class MLConfigPage;
    friend class MLConfigQuestionPrivate;
    MLConfigQuestion();
    virtual ~MLConfigQuestion();
    virtual QString ask() = 0;
    virtual bool processResponse(QString str) = 0;

protected:
    QJsonObject* config() { return m_config; }

private:
    QJsonObject* m_config;
    MLConfigQuestionPrivate* d;
};

#endif // MLCONFIGQUESTION_H
