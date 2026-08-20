#include "sysbassbackend.h"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QScreen>
#include <QProcess>
#include <QTimer>
#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QtEndian>
#include <QIcon>
#include <QDBusObjectPath>

SysbassImageProvider::SysbassImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage SysbassImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    QString dest, path;
    SysbassBackend::parseServicePath(QUrl::fromPercentEncoding(id.toUtf8()), dest, path);
    QDBusMessage msg = QDBusMessage::createMethodCall(dest, path, QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("Get"));
    msg << QStringLiteral("org.kde.StatusNotifierItem") << QStringLiteral("IconPixmap");
    QDBusReply<QDBusVariant> reply = QDBusConnection::sessionBus().call(msg);
    if (!reply.isValid()) return QImage();

    QDBusArgument arg = reply.value().variant().value<QDBusArgument>();
    if (arg.currentType() != QDBusArgument::ArrayType) return QImage();

    QImage bestImage;
    arg.beginArray();
    while (!arg.atEnd()) {
        arg.beginStructure();
        int w, h;
        QByteArray data;
        arg >> w >> h >> data;
        arg.endStructure();

        QImage img(w, h, QImage::Format_ARGB32);
        const quint32* src = reinterpret_cast<const quint32*>(data.constData());
        quint32* dst = reinterpret_cast<quint32*>(img.bits());
        for (int i = 0; i < w * h; ++i) {
            dst[i] = qFromBigEndian(src[i]);
        }

        if (img.width() >= 24 && bestImage.isNull()) {
            bestImage = img.copy();
        } else if (img.width() > bestImage.width()) {
            bestImage = img.copy();
        }
    }
    arg.endArray();

    if (size) *size = bestImage.size();
    return bestImage;
}

SysbassBackend::SysbassBackend(QObject *parent)
    : QObject(parent)
{
}

void SysbassBackend::parseServicePath(const QString &serviceSpec, QString &dest, QString &path)
{
    if (serviceSpec.contains(QLatin1Char('/'))) {
        int slashIdx = serviceSpec.indexOf(QLatin1Char('/'));
        dest = serviceSpec.left(slashIdx);
        path = serviceSpec.mid(slashIdx);
    } else {
        dest = serviceSpec;
        path = QStringLiteral("/StatusNotifierItem");
    }
}

void SysbassBackend::ungrabMouseHack(QQuickItem *visualItem)
{
    if (visualItem && visualItem->window() && visualItem->window()->mouseGrabberItem()) {
        QTimer::singleShot(0, visualItem, [visualItem]() {
            if (visualItem->window() && visualItem->window()->mouseGrabberItem()) {
                visualItem->window()->mouseGrabberItem()->ungrabMouse();
            }
        });
    }
}

QPointF SysbassBackend::calculateGlobalPos(QQuickItem *visualItem, int x, int y)
{
    if (!visualItem) {
        return QPointF(x, y);
    }

    QPointF pos = visualItem->mapToScene(QPointF(x, y));
    QQuickWindow *window = visualItem->window();
    if (window && window->screen()) {
        pos = window->mapToGlobal(pos.toPoint());
        
        // Handle high DPI scaling and multi-monitor offset
        QScreen *screen = window->screen();
        qreal devicePixelRatio = screen->devicePixelRatio();
        QRect geometry = screen->geometry();
        
        // Multi-monitor scaling adjustment matching KDE systemtray logic
        pos = QPointF(geometry.topLeft() + (pos - geometry.topLeft()) * devicePixelRatio);
    }
    return pos;
}

void SysbassBackend::provideXdgActivationToken(const QString &dest, const QString &path, std::function<void()> callback)
{
    bool isWayland = QGuiApplication::platformName().contains(QStringLiteral("wayland"), Qt::CaseInsensitive);
    if (!isWayland) {
        if (callback) callback();
        return;
    }

    // Request activation token via session bus or portal interface
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.portal.Desktop"),
        QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.Request"),
        QStringLiteral("Close")
    );

    // Call callback to proceed with action
    if (callback) {
        callback();
    }
}

void SysbassBackend::sendDbusEvent(const QString &serviceSpec, const QString &method, int x, int y, QQuickItem *visualItem)
{
    if (serviceSpec.isEmpty()) return;

    ungrabMouseHack(visualItem);
    QPointF globalPos = calculateGlobalPos(visualItem, x, y);
    int gx = qRound(globalPos.x());
    int gy = qRound(globalPos.y());

    QString dest, path;
    parseServicePath(serviceSpec, dest, path);

    provideXdgActivationToken(dest, path, [this, dest, path, method, gx, gy]() {
        QDBusMessage msg = QDBusMessage::createMethodCall(
            dest,
            path,
            QStringLiteral("org.kde.StatusNotifierItem"),
            method
        );
        msg << gx << gy;

        QDBusConnection::sessionBus().asyncCall(msg);
    });
}

void SysbassBackend::activate(const QString &service, int x, int y, QQuickItem *visualItem)
{
    if (service.contains(QStringLiteral("steam"), Qt::CaseInsensitive)) {
        QProcess::startDetached(QStringLiteral("steam"), QStringList() << QStringLiteral("steam://open/main"));
        return;
    }

    ungrabMouseHack(visualItem);
    QPointF globalPos = calculateGlobalPos(visualItem, x, y);
    int gx = qRound(globalPos.x());
    int gy = qRound(globalPos.y());

    QString dest, path;
    parseServicePath(service, dest, path);

    provideXdgActivationToken(dest, path, [this, service, dest, path, gx, gy, visualItem]() {
        QDBusMessage msg = QDBusMessage::createMethodCall(
            dest,
            path,
            QStringLiteral("org.kde.StatusNotifierItem"),
            QStringLiteral("Activate")
        );
        msg << gx << gy;

        QDBusPendingCall call = QDBusConnection::sessionBus().asyncCall(msg);
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);

        // Fallback to ContextMenu if Activate returns error (e.g. libappindicator apps)
        connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, service, gx, gy, visualItem](QDBusPendingCallWatcher *w) {
            QDBusPendingReply<> reply = *w;
            if (reply.isError()) {
                contextMenu(service, gx, gy, visualItem);
            }
            w->deleteLater();
        });
    });
}

void SysbassBackend::secondaryActivate(const QString &service, int x, int y, QQuickItem *visualItem)
{
    sendDbusEvent(service, QStringLiteral("SecondaryActivate"), x, y, visualItem);
}

void SysbassBackend::contextMenu(const QString &service, int x, int y, QQuickItem *visualItem)
{
    if (service.contains(QStringLiteral("steam"), Qt::CaseInsensitive)) {
        secondaryActivate(service, x, y, visualItem);
        return;
    }

    ungrabMouseHack(visualItem);
    QPointF globalPos = calculateGlobalPos(visualItem, x, y);
    int gx = qRound(globalPos.x());
    int gy = qRound(globalPos.y());

    showDBusMenu(service, gx, gy, visualItem);
}

void SysbassBackend::showDBusMenu(const QString &service, int gx, int gy, QQuickItem *visualItem)
{
    QString dest, path;
    parseServicePath(service, dest, path);

    // Get Menu object path
    QDBusMessage msg = QDBusMessage::createMethodCall(dest, path, QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("Get"));
    msg << QStringLiteral("org.kde.StatusNotifierItem") << QStringLiteral("Menu");
    QDBusReply<QDBusVariant> reply = QDBusConnection::sessionBus().call(msg);
    
    if (reply.isValid() && reply.value().variant().canConvert<QDBusObjectPath>()) {
        QString menuPath = reply.value().variant().value<QDBusObjectPath>().path();
        
        QMenu *menu = buildMenu(dest, menuPath);
        if (menu) {
            if (m_contextMenu) m_contextMenu->deleteLater();
            m_contextMenu = menu;
            menu->popup(QPoint(gx, gy));
            return; // Success
        }
    }
    
    // Fallback to ContextMenu method if Menu property fails
    provideXdgActivationToken(dest, path, [this, service, dest, path, gx, gy, visualItem]() {
        QDBusMessage fallbackMsg = QDBusMessage::createMethodCall(dest, path, QStringLiteral("org.kde.StatusNotifierItem"), QStringLiteral("ContextMenu"));
        fallbackMsg << gx << gy;

        QDBusPendingCall call = QDBusConnection::sessionBus().asyncCall(fallbackMsg);
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, service, gx, gy, visualItem](QDBusPendingCallWatcher *w) {
            QDBusPendingReply<> r = *w;
            if (r.isError()) {
                secondaryActivate(service, gx, gy, visualItem);
            }
            w->deleteLater();
        });
    });
}

void SysbassBackend::addDBusMenuItem(QMenu *menu, const QDBusArgument &arg, const QString &dest, const QString &path)
{
    if (arg.currentType() != QDBusArgument::StructureType) return;
    
    arg.beginStructure();
    int id;
    arg >> id;
    QVariantMap properties;
    arg >> properties;
    
    QString type = properties.value(QStringLiteral("type")).toString();
    QString label = properties.value(QStringLiteral("label")).toString();
    bool enabled = properties.value(QStringLiteral("enabled"), true).toBool();
    bool visible = properties.value(QStringLiteral("visible"), true).toBool();
    
    if (visible && id != 0) { 
        if (type == QStringLiteral("separator")) {
            menu->addSeparator();
        } else {
            QAction *action = new QAction(label, menu);
            action->setEnabled(enabled);
            connect(action, &QAction::triggered, [dest, path, id]() {
                QDBusMessage eventMsg = QDBusMessage::createMethodCall(dest, path, QStringLiteral("com.canonical.dbusmenu"), QStringLiteral("Event"));
                eventMsg << id << QStringLiteral("clicked") << QVariant::fromValue(QDBusVariant(QStringLiteral(""))) << (uint)0;
                QDBusConnection::sessionBus().asyncCall(eventMsg);
            });
            menu->addAction(action);
        }
    }
    
    if (!arg.atEnd()) {
        arg.beginArray();
        while (!arg.atEnd()) {
            QDBusVariant v;
            arg >> v;
            if (v.variant().canConvert<QDBusArgument>()) {
                if (id == 0) {
                    addDBusMenuItem(menu, v.variant().value<QDBusArgument>(), dest, path);
                } else if (visible && type != QStringLiteral("separator")) {
                    // Create submenu if needed (simplified)
                    QMenu *subMenu = new QMenu(label, menu);
                    addDBusMenuItem(subMenu, v.variant().value<QDBusArgument>(), dest, path);
                    if (!subMenu->isEmpty()) {
                        QAction *lastAction = menu->actions().last();
                        lastAction->setMenu(subMenu);
                    } else {
                        subMenu->deleteLater();
                    }
                }
            }
        }
        arg.endArray();
    }
    arg.endStructure();
}

QMenu* SysbassBackend::buildMenu(const QString &dest, const QString &path)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(dest, path, QStringLiteral("com.canonical.dbusmenu"), QStringLiteral("GetLayout"));
    msg << 0 << -1 << QStringList();
    QDBusReply<QDBusArgument> reply = QDBusConnection::sessionBus().call(msg);
    
    if (!reply.isValid()) return nullptr;
    
    QDBusArgument arg = reply.value();
    uint revision;
    arg.beginStructure();
    arg >> revision;
    
    QDBusVariant layoutVar;
    arg >> layoutVar;
    arg.endStructure();
    
    QMenu *menu = new QMenu();
    if (layoutVar.variant().canConvert<QDBusArgument>()) {
        addDBusMenuItem(menu, layoutVar.variant().value<QDBusArgument>(), dest, path);
    }
    
    if (menu->isEmpty()) {
        delete menu;
        return nullptr;
    }
    
    return menu;
}

void SysbassBackend::scroll(const QString &service, int delta, const QString &orientation)
{
    if (service.isEmpty()) return;

    QString dest, path;
    parseServicePath(service, dest, path);

    QDBusMessage msg = QDBusMessage::createMethodCall(
        dest,
        path,
        QStringLiteral("org.kde.StatusNotifierItem"),
        QStringLiteral("Scroll")
    );
    msg << delta << orientation;

    QDBusConnection::sessionBus().asyncCall(msg);
}
