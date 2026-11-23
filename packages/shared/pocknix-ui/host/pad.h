#pragma once

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QObject>
#include <QtQml/qqmlregistration.h>

struct input_event;

// Exposes evdev gamepad buttons/hats as Qt key presses to QML (singleton "Pad").
class Pad : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit Pad(QObject *parent = nullptr);
    ~Pad() override;

Q_SIGNALS:
    void pressed(int key);

private:
    void scan();
    void drain(int fd);
    void handleEvent(const struct input_event &ev);

    QList<int> m_fds;
    QHash<int, QByteArray> m_buf;
};
