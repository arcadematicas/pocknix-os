// Keep the kernel header before Qt so its input_* names cannot clash with Qt macros.
#include <linux/input.h>

#include <QByteArray>
#include <QDebug>
#include <QList>
#include <QObject>
#include <QSocketNotifier>

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "pad.h"

namespace {

constexpr int kMaxEventDevices = 64;

bool hasBit(const unsigned char *bits, size_t size, int code)
{
    return code >= 0 && static_cast<size_t>(code) < size * 8
        && (bits[code / 8] & (1u << (code % 8))) != 0;
}

} // namespace

Pad::Pad(QObject *parent) : QObject(parent)
{
    scan();
}

Pad::~Pad()
{
    const auto notifiers = findChildren<QSocketNotifier *>();
    for (QSocketNotifier *notifier : notifiers)
        notifier->setEnabled(false);
    for (int fd : m_fds)
        ::close(fd);
}

void Pad::scan()
{
    const bool debug = qEnvironmentVariableIsSet("POCKNIX_UI_PAD_DEBUG");
    for (int i = 0; i < kMaxEventDevices; ++i) {
        const QByteArray path = QByteArrayLiteral("/dev/input/event") + QByteArray::number(i);
        const int fd = ::open(path.constData(), O_RDONLY | O_NONBLOCK);
        if (fd < 0)
            continue;

        unsigned char keyBits[(KEY_CNT + 7) / 8] = {};
        unsigned char absBits[(ABS_CNT + 7) / 8] = {};
        const bool hasKey = ::ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBits)), keyBits) >= 0;
        const bool hasAbs = ::ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absBits)), absBits) >= 0;
        const bool isPad = (hasKey && (hasBit(keyBits, sizeof(keyBits), BTN_SOUTH)
                                       || hasBit(keyBits, sizeof(keyBits), BTN_DPAD_UP)))
                        || (hasAbs && hasBit(absBits, sizeof(absBits), ABS_HAT0X));
        if (!isPad) {
            ::close(fd);
            continue;
        }

        if (debug) {
            char name[128] = {};
            if (::ioctl(fd, EVIOCGNAME(sizeof(name) - 1), name) > 0)
                qInfo().noquote() << "Pad:" << path.constData() << name;
            else
                qInfo().noquote() << "Pad:" << path.constData();
        }

        auto *notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
        // Qt 6 emits activated(QSocketDescriptor, QSocketNotifier::Type); a zero-argument
        // lambda keeps this independent of that signature.
        connect(notifier, &QSocketNotifier::activated, this, [this, fd] { drain(fd); });
        m_fds.append(fd);
    }
}

// Reads whole events only. A read that ends mid-event keeps the leftover bytes in m_buf so the
// stream never desyncs (devices deliver 24-byte records, but a short read is legal).
void Pad::drain(int fd)
{
    char chunk[sizeof(struct input_event) * 64];
    for (;;) {
        const ssize_t n = ::read(fd, chunk, sizeof(chunk));
        if (n > 0) {
            QByteArray &buf = m_buf[fd];
            buf.append(chunk, static_cast<int>(n));
            while (buf.size() >= static_cast<int>(sizeof(struct input_event))) {
                struct input_event ev = {};
                memcpy(&ev, buf.constData(), sizeof(ev));
                buf.remove(0, static_cast<int>(sizeof(ev)));
                handleEvent(ev);
            }
            continue;
        }
        if (n < 0 && errno == EINTR)
            continue;
        return; // EAGAIN or error: nothing left for now.
    }
}

void Pad::handleEvent(const struct input_event &ev)
{
    if (ev.type == EV_KEY) {
        if (ev.value != 1)
            return;
        int key = -1;
        switch (ev.code) {
        case BTN_DPAD_UP:    key = Qt::Key_Up;       break;
        case BTN_DPAD_DOWN:  key = Qt::Key_Down;     break;
        case BTN_DPAD_LEFT:  key = Qt::Key_Left;     break;
        case BTN_DPAD_RIGHT: key = Qt::Key_Right;    break;
        case BTN_SOUTH:      key = Qt::Key_Return;   break;
        case BTN_EAST:       key = Qt::Key_Escape;   break;
        case BTN_TL:         key = Qt::Key_PageUp;   break;
        case BTN_TR:         key = Qt::Key_PageDown; break;
        case BTN_START:      key = Qt::Key_Return;   break;
        default: break;
        }
        if (key >= 0)
            emit pressed(key);
        return;
    }

    if (ev.type == EV_ABS && ev.value != 0) {
        int key = -1;
        if (ev.code == ABS_HAT0X)
            key = ev.value < 0 ? Qt::Key_Left : Qt::Key_Right;
        else if (ev.code == ABS_HAT0Y)
            key = ev.value < 0 ? Qt::Key_Up : Qt::Key_Down;
        if (key >= 0)
            emit pressed(key);
    }
}
