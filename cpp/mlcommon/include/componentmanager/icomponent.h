#ifndef ICOMPONENT_H
#define ICOMPONENT_H

#include <QObject>

class IComponent : public QObject {
    Q_OBJECT
public:
    IComponent(QObject* parent = 0);
    virtual QString name() const = 0;
    virtual bool initialize() = 0;
    virtual void extensionsReady();
    virtual void uninitialize() {}
    virtual QStringList dependencies() const;

protected:
    void addObject(QObject*);
    void addAutoReleasedObject(QObject*);
    void removeObject(QObject*);
};

#endif // ICOMPONENT_H
