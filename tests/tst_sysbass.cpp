// Tests what a user can do with the tray: click, scroll, open and use app menus, place icons.
// A fake tray app runs on its own DBus connection; run under dbus-run-session (see CMakeLists.txt).

#include "../src/sysbassbackend.h"

#include <QAction>
#include <QApplication>
#include <QDBusConnection>
#include <QDBusContext>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusVariant>
#include <QFile>
#include <QJSEngine>
#include <QMenu>
#include <QPointer>
#include <QTemporaryDir>
#include <QTest>

// dbusmenu layout node (ia{sv}av), sent by the fake app
struct MenuLayout {
    int id = 0;
    QVariantMap props;
    QList<MenuLayout> children;
};
Q_DECLARE_METATYPE(MenuLayout)

QDBusArgument &operator<<(QDBusArgument &arg, const MenuLayout &node)
{
    arg.beginStructure();
    arg << node.id << node.props;
    arg.beginArray(qMetaTypeId<QDBusVariant>());
    for (const MenuLayout &child : node.children) {
        arg << QDBusVariant(QVariant::fromValue(child));
    }
    arg.endArray();
    arg.endStructure();
    return arg;
}

// The fake app only sends layouts, never reads them
const QDBusArgument &operator>>(const QDBusArgument &arg, MenuLayout &)
{
    return arg;
}

// Fake StatusNotifierItem; records every call it gets
class FakeItem : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.StatusNotifierItem")
    Q_PROPERTY(QDBusObjectPath Menu READ menu)

public:
    QStringList calls;
    bool rejectActivate = false; // like libappindicator apps
    QString menuPath = QStringLiteral("/MenuBar");

    QDBusObjectPath menu() const { return QDBusObjectPath(menuPath); }

public Q_SLOTS:
    void Activate(int x, int y)
    {
        calls << QStringLiteral("Activate %1 %2").arg(x).arg(y);
        if (rejectActivate) {
            sendErrorReply(QDBusError::NotSupported, QStringLiteral("not supported"));
        }
    }
    void SecondaryActivate(int x, int y) { calls << QStringLiteral("SecondaryActivate %1 %2").arg(x).arg(y); }
    void ContextMenu(int x, int y) { calls << QStringLiteral("ContextMenu %1 %2").arg(x).arg(y); }
    void Scroll(int delta, const QString &orientation) { calls << QStringLiteral("Scroll %1 %2").arg(delta).arg(orientation); }
};

// Fake com.canonical.dbusmenu
class FakeMenu : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.canonical.dbusmenu")

public:
    QStringList calls;
    MenuLayout layout;

public Q_SLOTS:
    bool AboutToShow(int id)
    {
        calls << QStringLiteral("AboutToShow %1").arg(id);
        return false;
    }
    uint GetLayout(int parentId, int depth, const QStringList &, MenuLayout &out)
    {
        calls << QStringLiteral("GetLayout %1 %2").arg(parentId).arg(depth);
        out = layout;
        return 1;
    }
    void Event(int id, const QString &eventId, const QDBusVariant &, uint)
    {
        calls << QStringLiteral("Event %1 %2").arg(id).arg(eventId);
    }
};

static MenuLayout item(int id, const QVariantMap &props, const QList<MenuLayout> &children = {})
{
    return MenuLayout{id, props, children};
}

static QMenu *visibleMenu()
{
    for (QWidget *w : QApplication::topLevelWidgets()) {
        auto *menu = qobject_cast<QMenu *>(w);
        if (menu && menu->isVisible() && !menu->parentWidget()) {
            return menu;
        }
    }
    return nullptr;
}

static QAction *actionNamed(QMenu *menu, const QString &text)
{
    for (QAction *a : menu->actions()) {
        if (a->text() == text) {
            return a;
        }
    }
    return nullptr;
}

class TestSysbass : public QObject
{
    Q_OBJECT

    QDBusConnection appBus{QStringLiteral("unset")};
    FakeItem fakeItem;
    FakeMenu fakeMenu;
    QString source;

private Q_SLOTS:
    void initTestCase()
    {
        qDBusRegisterMetaType<MenuLayout>();
        // Separate connection, so the backend talks to the "app" through the bus like in real life
        appBus = QDBusConnection::connectToBus(QDBusConnection::SessionBus, QStringLiteral("fake-app"));
        QVERIFY2(appBus.isConnected(), "no session bus; run the test under dbus-run-session");
        QVERIFY(appBus.registerObject(QStringLiteral("/StatusNotifierItem"), &fakeItem,
                                      QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllProperties));
        QVERIFY(appBus.registerObject(QStringLiteral("/MenuBar"), &fakeMenu, QDBusConnection::ExportAllSlots));
        source = appBus.baseService() + QStringLiteral("/StatusNotifierItem");

        fakeMenu.layout = item(0, {}, {
            item(1, {{QStringLiteral("label"), QStringLiteral("_Open")}}),
            item(2, {{QStringLiteral("type"), QStringLiteral("separator")}}),
            item(3, {{QStringLiteral("label"), QStringLiteral("Invisible")}, {QStringLiteral("visible"), false}}),
            item(4, {{QStringLiteral("label"), QStringLiteral("Disabled")}, {QStringLiteral("enabled"), false}}),
            item(5, {{QStringLiteral("label"), QStringLiteral("Mute")},
                     {QStringLiteral("toggle-type"), QStringLiteral("checkmark")},
                     {QStringLiteral("toggle-state"), 1}}),
            item(6, {{QStringLiteral("label"), QStringLiteral("Status")}, {QStringLiteral("children-display"), QStringLiteral("submenu")}}, {
                item(7, {{QStringLiteral("label"), QStringLiteral("Online")}}),
                item(8, {{QStringLiteral("label"), QStringLiteral("Away")}}),
            }),
            item(9, {{QStringLiteral("label"), QStringLiteral("_Quit")}}),
        });
    }

    void init()
    {
        fakeItem.calls.clear();
        fakeItem.rejectActivate = false;
        fakeItem.menuPath = QStringLiteral("/MenuBar");
        fakeMenu.calls.clear();
    }

    void cleanup()
    {
        while (QMenu *menu = visibleMenu()) {
            menu->hide();
        }
    }

    void leftClickActivatesApp()
    {
        SysbassBackend backend;
        backend.activate(source, nullptr, 10, 20);
        QTRY_COMPARE(fakeItem.calls, QStringList{QStringLiteral("Activate 10 20")});
        QTest::qWait(100);
        QVERIFY(!visibleMenu());
    }

    void leftClickShowsMenuWhenAppRejectsActivate()
    {
        fakeItem.rejectActivate = true;
        SysbassBackend backend;
        backend.activate(source, nullptr, 10, 20);
        QTRY_VERIFY(visibleMenu());
        QCOMPARE(fakeItem.calls, QStringList{QStringLiteral("Activate 10 20")});
    }

    void leftClickOnSteamOpensSteam()
    {
        // Fake "steam" binary first in PATH records its arguments
        QTemporaryDir dir;
        const QString script = dir.filePath(QStringLiteral("steam"));
        QFile file(script);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("#!/bin/sh\necho \"$@\" > \"$(dirname \"$0\")/args\"\n");
        file.close();
        file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
        const QByteArray oldPath = qgetenv("PATH");
        qputenv("PATH", dir.path().toUtf8() + ':' + oldPath);

        SysbassBackend backend;
        backend.activate(appBus.baseService() + QStringLiteral("/org/ayatana/NotificationItem/steam"), nullptr, 10, 20);
        qputenv("PATH", oldPath);

        QFile args(dir.filePath(QStringLiteral("args")));
        QTRY_VERIFY(args.exists());
        QVERIFY(args.open(QIODevice::ReadOnly));
        QCOMPARE(args.readAll().trimmed(), QByteArray("steam://open/main"));
        QVERIFY(fakeItem.calls.isEmpty());
    }

    void middleClickRunsSecondaryAction()
    {
        SysbassBackend backend;
        backend.secondaryActivate(source, nullptr, 5, 6);
        QTRY_COMPARE(fakeItem.calls, QStringList{QStringLiteral("SecondaryActivate 5 6")});
    }

    void wheelScrollsApp()
    {
        SysbassBackend backend;
        backend.scroll(source, 120, QStringLiteral("vertical"));
        backend.scroll(source, -120, QStringLiteral("horizontal"));
        QTRY_COMPARE(fakeItem.calls, (QStringList{QStringLiteral("Scroll 120 vertical"), QStringLiteral("Scroll -120 horizontal")}));
    }

    void rightClickShowsAppMenu()
    {
        SysbassBackend backend;
        backend.contextMenu(source, nullptr, 10, 20);
        QTRY_VERIFY(visibleMenu());
        QMenu *menu = visibleMenu();

        // Apps that fill their menu lazily (Electron) get AboutToShow first
        QCOMPARE(fakeMenu.calls, (QStringList{QStringLiteral("AboutToShow 0"), QStringLiteral("GetLayout 0 -1")}));

        QStringList texts;
        for (QAction *a : menu->actions()) {
            texts << (a->isSeparator() ? QStringLiteral("---") : a->text());
        }
        QCOMPARE(texts, (QStringList{QStringLiteral("&Open"), QStringLiteral("---"), QStringLiteral("Disabled"),
                                     QStringLiteral("Mute"), QStringLiteral("Status"), QStringLiteral("&Quit")}));

        QVERIFY(!actionNamed(menu, QStringLiteral("Disabled"))->isEnabled());
        QVERIFY(actionNamed(menu, QStringLiteral("Mute"))->isChecked());

        QMenu *status = actionNamed(menu, QStringLiteral("Status"))->menu();
        QVERIFY(status);
        QCOMPARE(status->actions().size(), 2);
        QCOMPARE(status->actions().at(1)->text(), QStringLiteral("Away"));
    }

    void clickingMenuEntryTellsApp()
    {
        SysbassBackend backend;
        backend.contextMenu(source, nullptr, 10, 20);
        QTRY_VERIFY(visibleMenu());

        actionNamed(visibleMenu(), QStringLiteral("&Quit"))->trigger();
        QTRY_VERIFY(fakeMenu.calls.contains(QStringLiteral("Event 9 clicked")));

        QMenu *status = actionNamed(visibleMenu(), QStringLiteral("Status"))->menu();
        status->actions().at(1)->trigger();
        QTRY_VERIFY(fakeMenu.calls.contains(QStringLiteral("Event 8 clicked")));
    }

    void rightClickWithoutMenuAsksAppToDrawOne()
    {
        fakeItem.menuPath = QStringLiteral("/");
        SysbassBackend backend;
        backend.contextMenu(source, nullptr, 10, 20);
        QTRY_COMPARE(fakeItem.calls, QStringList{QStringLiteral("ContextMenu 10 20")});
        QVERIFY(fakeMenu.calls.isEmpty());
    }

    void secondRightClickReplacesMenu()
    {
        SysbassBackend backend;
        backend.contextMenu(source, nullptr, 10, 20);
        QTRY_VERIFY(visibleMenu());
        QPointer<QMenu> first = visibleMenu();

        backend.contextMenu(source, nullptr, 10, 20);
        QTRY_VERIFY(!first);
        QTRY_VERIFY(visibleMenu());
    }

    void menuLabels_data()
    {
        QTest::addColumn<QString>("dbus");
        QTest::addColumn<QString>("qt");
        QTest::newRow("mnemonic") << QStringLiteral("_Quit") << QStringLiteral("&Quit");
        QTest::newRow("literal underscore") << QStringLiteral("snake__case") << QStringLiteral("snake_case");
        QTest::newRow("ampersand") << QStringLiteral("Tom & Jerry") << QStringLiteral("Tom && Jerry");
        QTest::newRow("trailing underscore") << QStringLiteral("Trailing_") << QStringLiteral("Trailing&");
        QTest::newRow("empty") << QStringLiteral("") << QStringLiteral("");
    }

    void menuLabels()
    {
        QFETCH(QString, dbus);
        QFETCH(QString, qt);
        QCOMPARE(dbusMenuLabelToQt(dbus), qt);
    }

    // The same placement.js runs in the panel and in the settings page
    void iconPlacement()
    {
        QFile file(QStringLiteral(PLACEMENT_JS));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QJSEngine js;
        const QJSValue loaded = js.evaluate(QString::fromUtf8(file.readAll()), file.fileName());
        QVERIFY2(!loaded.isError(), qPrintable(loaded.toString()));

        auto eval = [&js](const char *code) {
            return js.evaluate(QStringLiteral("JSON.stringify(%1)").arg(QString::fromUtf8(code))).toString();
        };

        // New apps land in the overflow popup
        QCOMPARE(eval("placement('steam', [], [])"), QStringLiteral("0"));
        QCOMPARE(eval("placement('steam', ['steam'], [])"), QStringLiteral("1"));
        QCOMPARE(eval("placement('steam', [], ['steam'])"), QStringLiteral("2"));

        // Moving an icon removes it from its old list
        QCOMPARE(eval("setPlacement('steam', Panel, [], ['steam'])"), QStringLiteral(R"({"shown":["steam"],"hidden":[]})"));
        QCOMPARE(eval("setPlacement('steam', Hidden, ['a', 'steam'], [])"), QStringLiteral(R"({"shown":["a"],"hidden":["steam"]})"));
        QCOMPARE(eval("setPlacement('steam', Overflow, ['steam'], ['b'])"), QStringLiteral(R"({"shown":[],"hidden":["b"]})"));
        // Picking the same place twice doesn't duplicate it
        QCOMPARE(eval("setPlacement('steam', Panel, ['steam'], [])"), QStringLiteral(R"({"shown":["steam"],"hidden":[]})"));
    }
};

QTEST_MAIN(TestSysbass)
#include "tst_sysbass.moc"
