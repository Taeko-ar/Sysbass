#include "sysbassbackend.h"

#include <KWaylandExtras>
#include <KWindowSystem>

#include <QAction>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusVariant>
#include <QFuture>
#include <QIcon>
#include <QMenu>
#include <QPixmap>
#include <QProcess>
#include <QQuickItem>
#include <QQuickWindow>
#include <QWindow>

namespace
{
const QString sniInterface = QStringLiteral("org.kde.StatusNotifierItem");
const QString menuInterface = QStringLiteral("com.canonical.dbusmenu");

// DataEngineSource is "bus/object/path"; a bare bus name means the default path
void splitSource(const QString &source, QString &dest, QString &path)
{
    const int slash = source.indexOf(QLatin1Char('/'));
    dest = slash < 0 ? source : source.left(slash);
    path = slash < 0 ? QStringLiteral("/StatusNotifierItem") : source.mid(slash);
}

QPoint globalPos(QQuickItem *item, qreal x, qreal y)
{
    return item ? item->mapToGlobal(QPointF(x, y)).toPoint() : QPoint(qRound(x), qRound(y));
}

QDBusMessage sniCall(const QString &dest, const QString &path, const QString &method, QPoint pos)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(dest, path, sniInterface, method);
    msg << pos.x() << pos.y();
    return msg;
}

// Async call; fn gets the reply (or error) message. Calls never block plasmashell.
template<typename F>
void callAsync(const QDBusMessage &msg, QObject *context, F fn)
{
    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(msg), context);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, context, [fn](QDBusPendingCallWatcher *w) {
        w->deleteLater();
        fn(w->reply());
    });
}

bool isError(const QDBusMessage &reply)
{
    return reply.type() == QDBusMessage::ErrorMessage;
}

// One dbusmenu layout node: (ia{sv}av)
struct MenuNode {
    int id = 0;
    QVariantMap props;
    QList<QDBusArgument> children;
};

MenuNode readNode(const QDBusArgument &arg)
{
    MenuNode node;
    arg.beginStructure();
    arg >> node.id >> node.props;
    arg.beginArray();
    while (!arg.atEnd()) {
        QDBusVariant child;
        arg >> child;
        node.children << child.variant().value<QDBusArgument>();
    }
    arg.endArray();
    arg.endStructure();
    return node;
}

void fillMenu(QMenu *menu, const MenuNode &node, const QString &dest, const QString &menuPath)
{
    for (const QDBusArgument &childArg : node.children) {
        const MenuNode child = readNode(childArg);
        const QVariantMap &p = child.props;

        if (!p.value(QStringLiteral("visible"), true).toBool()) {
            continue;
        }
        if (p.value(QStringLiteral("type")).toString() == QLatin1String("separator")) {
            menu->addSeparator();
            continue;
        }

        QAction *action = menu->addAction(dbusMenuLabelToQt(p.value(QStringLiteral("label")).toString()));
        action->setEnabled(p.value(QStringLiteral("enabled"), true).toBool());

        const QString iconName = p.value(QStringLiteral("icon-name")).toString();
        if (!iconName.isEmpty()) {
            action->setIcon(QIcon::fromTheme(iconName));
        } else {
            QPixmap pixmap;
            if (pixmap.loadFromData(p.value(QStringLiteral("icon-data")).toByteArray())) {
                action->setIcon(QIcon(pixmap));
            }
        }

        const QString toggleType = p.value(QStringLiteral("toggle-type")).toString();
        if (toggleType == QLatin1String("checkmark") || toggleType == QLatin1String("radio")) {
            action->setCheckable(true);
            action->setChecked(p.value(QStringLiteral("toggle-state")).toInt() == 1);
        }

        if (!child.children.isEmpty()) {
            auto *subMenu = new QMenu(menu);
            fillMenu(subMenu, child, dest, menuPath);
            action->setMenu(subMenu);
            continue;
        }

        const int id = child.id;
        QObject::connect(action, &QAction::triggered, menu, [dest, menuPath, id]() {
            QDBusMessage msg = QDBusMessage::createMethodCall(dest, menuPath, menuInterface, QStringLiteral("Event"));
            msg << id << QStringLiteral("clicked") << QVariant::fromValue(QDBusVariant(QString())) << 0u;
            QDBusConnection::sessionBus().asyncCall(msg);
        });
    }
}
} // namespace

QString dbusMenuLabelToQt(const QString &label)
{
    QString out;
    out.reserve(label.size());
    for (int i = 0; i < label.size(); ++i) {
        const QChar c = label.at(i);
        if (c == QLatin1Char('_')) {
            const bool literal = i + 1 < label.size() && label.at(i + 1) == QLatin1Char('_');
            out += literal ? QLatin1Char('_') : QLatin1Char('&');
            i += literal ? 1 : 0;
        } else if (c == QLatin1Char('&')) {
            out += QStringLiteral("&&");
        } else {
            out += c;
        }
    }
    return out;
}

void SysbassBackend::activate(const QString &source, QQuickItem *item, qreal x, qreal y)
{
    // Steam's tray icon rejects Activate; ask Steam itself to show its main window
    if (source.contains(QLatin1String("steam"), Qt::CaseInsensitive)) {
        QProcess::startDetached(QStringLiteral("steam"), {QStringLiteral("steam://open/main")});
        return;
    }
    callItem(source, QStringLiteral("Activate"), item, x, y);
}

void SysbassBackend::secondaryActivate(const QString &source, QQuickItem *item, qreal x, qreal y)
{
    callItem(source, QStringLiteral("SecondaryActivate"), item, x, y);
}

void SysbassBackend::contextMenu(const QString &source, QQuickItem *item, qreal x, qreal y)
{
    if (source.isEmpty()) {
        return;
    }
    QString dest, path;
    splitSource(source, dest, path);
    requestMenu(dest, path, item, globalPos(item, x, y));
}

void SysbassBackend::scroll(const QString &source, int delta, const QString &orientation)
{
    if (source.isEmpty()) {
        return;
    }
    QString dest, path;
    splitSource(source, dest, path);
    QDBusMessage msg = QDBusMessage::createMethodCall(dest, path, sniInterface, QStringLiteral("Scroll"));
    msg << delta << orientation;
    QDBusConnection::sessionBus().asyncCall(msg);
}

void SysbassBackend::showClipboard()
{
    // Klipper lives in the Clipboard widget; it must be loaded somewhere (panel or desktop)
    QDBusConnection::sessionBus().asyncCall(QDBusMessage::createMethodCall(QStringLiteral("org.kde.klipper"),
                                                                           QStringLiteral("/klipper"),
                                                                           QStringLiteral("org.kde.klipper.klipper"),
                                                                           QStringLiteral("showKlipperPopupMenu")));
}

void SysbassBackend::callItem(const QString &source, const QString &method, QQuickItem *item, qreal x, qreal y)
{
    if (source.isEmpty()) {
        return;
    }
    QString dest, path;
    splitSource(source, dest, path);
    const QPoint pos = globalPos(item, x, y);
    const QPointer<QQuickItem> guard(item);

    auto send = [this, dest, path, method, pos, guard]() {
        callAsync(sniCall(dest, path, method, pos), this, [this, dest, path, method, pos, guard](const QDBusMessage &reply) {
            // libappindicator items (Steam, Electron apps) reject Activate; show their menu like the stock tray
            if (isError(reply) && method == QLatin1String("Activate")) {
                requestMenu(dest, path, guard, pos);
            }
        });
    };

    QWindow *window = item ? item->window() : nullptr;
    if (!KWindowSystem::isPlatformWayland() || !window) {
        send();
        return;
    }

    // On Wayland the app needs a token from the compositor to raise its window
    KWaylandExtras::xdgActivationToken(window, QString()).then(this, [dest, path, send](const QString &token) {
        QDBusMessage msg = QDBusMessage::createMethodCall(dest, path, sniInterface, QStringLiteral("ProvideXdgActivationToken"));
        msg << token;
        QDBusConnection::sessionBus().asyncCall(msg); // same bus and destination, so it arrives before the activation call
        send();
    });
}

void SysbassBackend::requestMenu(const QString &dest, const QString &path, QPointer<QQuickItem> item, QPoint pos)
{
    auto fallback = [dest, path, pos]() {
        // Item has no dbusmenu: ask it to draw its own menu
        QDBusConnection::sessionBus().asyncCall(sniCall(dest, path, QStringLiteral("ContextMenu"), pos));
    };

    QDBusMessage getMenu = QDBusMessage::createMethodCall(dest, path, QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("Get"));
    getMenu << sniInterface << QStringLiteral("Menu");

    callAsync(getMenu, this, [this, dest, item, pos, fallback](const QDBusMessage &reply) {
        const QString menuPath =
            isError(reply) ? QString() : reply.arguments().value(0).value<QDBusVariant>().variant().value<QDBusObjectPath>().path();
        if (menuPath.isEmpty() || menuPath == QLatin1String("/")) {
            fallback();
            return;
        }

        // Some apps (Electron) fill the menu only after AboutToShow; its result does not matter
        QDBusMessage aboutToShow = QDBusMessage::createMethodCall(dest, menuPath, menuInterface, QStringLiteral("AboutToShow"));
        aboutToShow << 0;
        callAsync(aboutToShow, this, [this, dest, menuPath, item, pos, fallback](const QDBusMessage &) {
            QDBusMessage getLayout = QDBusMessage::createMethodCall(dest, menuPath, menuInterface, QStringLiteral("GetLayout"));
            getLayout << 0 << -1 << QStringList();
            callAsync(getLayout, this, [this, dest, menuPath, item, pos, fallback](const QDBusMessage &layoutReply) {
                // GetLayout returns (u revision, (ia{sv}av) layout)
                if (isError(layoutReply) || layoutReply.arguments().size() < 2) {
                    fallback();
                    return;
                }
                showMenu(dest, menuPath, layoutReply.arguments().at(1).value<QDBusArgument>(), item, pos);
            });
        });
    });
}

void SysbassBackend::showMenu(const QString &dest, const QString &menuPath, const QDBusArgument &layout, QPointer<QQuickItem> item, QPoint pos)
{
    auto *menu = new QMenu;
    fillMenu(menu, readNode(layout), dest, menuPath);
    if (menu->isEmpty()) {
        delete menu;
        return;
    }
    // Deferred, so a triggered action still runs before the menu goes away
    connect(menu, &QMenu::aboutToHide, menu, &QObject::deleteLater);

    if (m_menu) {
        m_menu->hide();
        m_menu->deleteLater();
    }
    m_menu = menu;

    // Wayland places popups relative to a parent window
    if (item && item->window()) {
        menu->winId();
        menu->windowHandle()->setTransientParent(item->window());
    }
    menu->popup(pos);
}
