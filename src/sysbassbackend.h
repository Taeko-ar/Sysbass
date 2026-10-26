#ifndef SYSBASSBACKEND_H
#define SYSBASSBACKEND_H

#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QString>

class QDBusArgument;
class QMenu;
class QQuickItem;

// Converts a dbusmenu label ("_" mnemonic, "__" literal) to Qt ("&" mnemonic, "&&" literal)
QString dbusMenuLabelToQt(const QString &label);

// Talks to StatusNotifierItems over DBus. "source" is the model's DataEngineSource ("bus/object/path").
class SysbassBackend : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    Q_INVOKABLE void activate(const QString &source, QQuickItem *item, qreal x, qreal y);
    Q_INVOKABLE void secondaryActivate(const QString &source, QQuickItem *item, qreal x, qreal y);
    Q_INVOKABLE void contextMenu(const QString &source, QQuickItem *item, qreal x, qreal y);
    Q_INVOKABLE void scroll(const QString &source, int delta, const QString &orientation);
    // Opens Klipper's history menu at the mouse pointer
    Q_INVOKABLE void showClipboard();

private:
    void callItem(const QString &source, const QString &method, QQuickItem *item, qreal x, qreal y);
    void requestMenu(const QString &dest, const QString &path, QPointer<QQuickItem> item, QPoint pos);
    void showMenu(const QString &dest, const QString &menuPath, const QDBusArgument &layout, QPointer<QQuickItem> item, QPoint pos);

    QPointer<QMenu> m_menu;
};

#endif // SYSBASSBACKEND_H
