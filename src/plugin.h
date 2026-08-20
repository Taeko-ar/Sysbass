#ifndef SYSBASS_PLUGIN_H
#define SYSBASS_PLUGIN_H

#include <QQmlExtensionPlugin>
#include <QtQml>
#include <QQmlEngine>

class SysbassPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    void registerTypes(const char *uri) override;
    void initializeEngine(QQmlEngine *engine, const char *uri) override;
};

#endif // SYSBASS_PLUGIN_H
