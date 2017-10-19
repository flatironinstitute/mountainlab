/******************************************************
** See the accompanying README and LICENSE files
** Author(s): Jeremy Magland
** Created: 3/8/2017
*******************************************************/
#ifndef MLCONFIGPAGE_H
#define MLCONFIGPAGE_H

#include <QJsonObject>
#include <QList>
#include <QString>
#include "mlconfigquestion.h"

class MLConfigPage {
public:
    MLConfigPage(QJsonObject* config);
    virtual ~MLConfigPage();

    virtual QString title() = 0;
    virtual QString description() = 0;

    int questionCount() const;
    MLConfigQuestion* question(int num) const;

protected:
    void addQuestion(MLConfigQuestion* question);

private:
    QJsonObject* m_config = 0;
    QList<MLConfigQuestion*> m_questions;
};

#endif // MLCONFIGPAGE_H
