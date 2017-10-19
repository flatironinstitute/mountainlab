#include "componentmanager.h"
#include <QHash>
#include <QList>
#include "icomponent.h"
#include "mlcommon.h"

struct ComponentSpec {
    IComponent* component;
    QList<ComponentSpec*> deps;
};

ComponentManager::ComponentManager(QObject* parent)
    : QObject(parent)
{
}

ComponentManager::~ComponentManager()
{
    if (!m_loaded.isEmpty())
        unloadComponents();
}

void ComponentManager::addComponent(IComponent* c)
{
    m_components << c;
}

void ComponentManager::loadComponents()
{
    // resolve deps
    QList<IComponent*> queue = resolveDependencies();
    // "foreach" works on a copy, thus we can safely modify queue
    foreach (IComponent* c, queue) {
        if (!c->initialize()) {
            // component failed
            queue.removeOne(c);
            // TODO: Fail dependant components
            continue;
        }
        m_loaded.append(c);
    }
    for (int i = queue.size() - 1; i >= 0; --i) {
        queue.at(i)->extensionsReady();
    }
}

void ComponentManager::unloadComponents()
{
    for (int i = m_loaded.size() - 1; i >= 0; --i) {
        m_loaded.at(i)->uninitialize();
    }
    m_loaded.clear();
}

QList<IComponent*> ComponentManager::loadedComponents() const
{
    return m_loaded;
}

bool loadQueue(ComponentSpec* spec, QList<IComponent*>& queue, QList<ComponentSpec*>& loopQueue)
{
    if (queue.contains(spec->component))
        return true;
    if (loopQueue.contains(spec)) {
        return false;
    }
    loopQueue.append(spec);
    for (ComponentSpec* dep : spec->deps) {
        if (!loadQueue(dep, queue, loopQueue)) {
            return false;
        }
    }
    queue.append(spec->component);
    return true;
}

QList<IComponent*> ComponentManager::resolveDependencies() const
{
    // create specs for all components
    QHash<QString, ComponentSpec*> componentHash;
    for (IComponent* c : m_components) {
        ComponentSpec* spec = new ComponentSpec;
        spec->component = c;
        componentHash.insert(c->name(), spec);
    }
    QList<ComponentSpec*> specs;
    for (ComponentSpec* spec : componentHash.values()) {
        bool failed = false;
        foreach (const QString& dep, spec->component->dependencies()) {
            auto depIter = componentHash.constFind(dep);
            if (depIter != componentHash.constEnd()) {
                spec->deps.append(*depIter);
            }
            else {
                failed = true;
                break;
            }
        }
        if (failed)
            continue;
        specs.append(spec);
    }

    QList<IComponent*> resolved;
    // reorder plugins according to dependencies

    foreach (ComponentSpec* spec, specs) {
        QList<ComponentSpec*> loopQueue;
        loadQueue(spec, resolved, loopQueue);
    }
    return resolved;
}
