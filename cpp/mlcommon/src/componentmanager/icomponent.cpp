#include "icomponent.h"
#include "objectregistry.h"

IComponent::IComponent(QObject* parent)
    : QObject(parent)
{
}

void IComponent::extensionsReady() {}

QStringList IComponent::dependencies() const
{
    return QStringList();
}

void IComponent::addObject(QObject* o)
{
    ObjectRegistry::addObject(o);
}

void IComponent::addAutoReleasedObject(QObject* o)
{
    ObjectRegistry::addAutoReleasedObject(o);
}

void IComponent::removeObject(QObject* o)
{
    ObjectRegistry::removeObject(o);
}
