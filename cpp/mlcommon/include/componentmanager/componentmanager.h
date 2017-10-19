#ifndef COMPONENTMANAGER_H
#define COMPONENTMANAGER_H

#include <QObject>
#include <QList>
#include "icomponent.h"

class ComponentManager : public QObject {
    Q_OBJECT
public:
    ComponentManager(QObject* parent = 0);
    ~ComponentManager();
    void addComponent(IComponent* c);
    void loadComponents();
    void unloadComponents();
    QList<IComponent*> loadedComponents() const;

protected:
    QList<IComponent*> resolveDependencies() const;

private:
    QList<IComponent*> m_components;
    QList<IComponent*> m_loaded;
};

#endif // COMPONENTMANAGER_H
