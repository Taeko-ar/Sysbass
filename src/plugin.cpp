#include "plugin.h"
#include "sysbassbackend.h"

#include <QQmlEngine>

void SysbassPlugin::registerTypes(const char *uri)
{
    qmlRegisterType<SysbassBackend>(uri, 1, 0, "SysbassBackend");
}
