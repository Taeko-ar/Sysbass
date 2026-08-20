#ifndef SYSBASSBACKEND_H
#define SYSBASSBACKEND_H

#include <QObject>
#include <QString>
#include <QPoint>
#include <QtQuick/QQuickItem>
#include <QQuickImageProvider>
#include <QMenu>
#include <QPointer>
#include <QDBusArgument>
#include <functional>

class SysbassImageProvider : public QQuickImageProvider
{
public:
    SysbassImageProvider();
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
};

class SysbassBackend : public QObject
{
    Q_OBJECT

public:
    explicit SysbassBackend(QObject *parent = nullptr);
    ~SysbassBackend() override = default;

    Q_INVOKABLE void activate(const QString &service, int x, int y, QQuickItem *visualItem = nullptr);
    Q_INVOKABLE void secondaryActivate(const QString &service, int x, int y, QQuickItem *visualItem = nullptr);
    Q_INVOKABLE void contextMenu(const QString &service, int x, int y, QQuickItem *visualItem = nullptr);
    Q_INVOKABLE void scroll(const QString &service, int delta, const QString &orientation = QStringLiteral("vertical"));

    static void parseServicePath(const QString &serviceSpec, QString &dest, QString &path);

private:
    void sendDbusEvent(const QString &serviceSpec, const QString &method, int x, int y, QQuickItem *visualItem = nullptr);
    void ungrabMouseHack(QQuickItem *visualItem);
    void provideXdgActivationToken(const QString &dest, const QString &path, std::function<void()> callback);
    QPointF calculateGlobalPos(QQuickItem *visualItem, int x, int y);
    
    // Minimal DBusMenu client helpers
    void showDBusMenu(const QString &service, int gx, int gy, QQuickItem *visualItem);
    QMenu* buildMenu(const QString &dest, const QString &path);
    void addDBusMenuItem(QMenu *menu, const QDBusArgument &arg, const QString &dest, const QString &path);

    QPointer<QMenu> m_contextMenu;
};

#endif // SYSBASSBACKEND_H
