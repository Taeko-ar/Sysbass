#include "plugin.h"
#include "sysbassbackend.h"

void SysbassPlugin::registerTypes(const char *uri)
{
    qmlRegisterType<SysbassBackend>(uri, 1, 0, "SysbassBackend");
}

void SysbassPlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    Q_UNUSED(uri)
    engine->addImageProvider(QStringLiteral("sysbass"), new SysbassImageProvider);
}
