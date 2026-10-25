#ifndef SYSBASS_PLUGIN_H
#define SYSBASS_PLUGIN_H

#include <QQmlExtensionPlugin>

class SysbassPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    void registerTypes(const char *uri) override;
};

#endif // SYSBASS_PLUGIN_H
