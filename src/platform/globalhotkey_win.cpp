// Windows backend for GlobalHotkey — Win32 RegisterHotKey + a Qt native
// event filter to catch the WM_HOTKEY message. RegisterHotKey with a null
// HWND posts the message to the registering thread's queue, which Qt's
// event dispatcher drains, so the filter sees it on the GUI thread.
#include "globalhotkey.h"

#include <QAbstractNativeEventFilter>
#include <QCoreApplication>

#include <windows.h>

namespace {

constexpr int kHotKeyId = 0x5746; // 'WF'

// Qt logical key -> Win32 virtual-key code. Returns -1 when unsupported.
int winVirtualKey(Qt::Key key)
{
    if (key >= Qt::Key_A && key <= Qt::Key_Z)
        return 'A' + (key - Qt::Key_A);
    if (key >= Qt::Key_0 && key <= Qt::Key_9)
        return '0' + (key - Qt::Key_0);
    if (key >= Qt::Key_F1 && key <= Qt::Key_F12)
        return VK_F1 + (key - Qt::Key_F1);

    switch (key) {
    case Qt::Key_Space: return VK_SPACE;
    case Qt::Key_Left:  return VK_LEFT;
    case Qt::Key_Right: return VK_RIGHT;
    case Qt::Key_Up:    return VK_UP;
    case Qt::Key_Down:  return VK_DOWN;
    default: return -1;
    }
}

UINT winModifiers(Qt::KeyboardModifiers mods)
{
    UINT native = MOD_NOREPEAT; // no machine-gun toggling while held
    if (mods & Qt::ControlModifier) native |= MOD_CONTROL;
    if (mods & Qt::ShiftModifier)   native |= MOD_SHIFT;
    if (mods & Qt::AltModifier)     native |= MOD_ALT;
    if (mods & Qt::MetaModifier)    native |= MOD_WIN;
    return native;
}

} // namespace

struct GlobalHotkey::Impl : public QAbstractNativeEventFilter
{
    GlobalHotkey *owner = nullptr;
    bool filterInstalled = false;

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *) override
    {
        if (eventType != "windows_generic_MSG")
            return false;

        MSG *msg = static_cast<MSG *>(message);
        if (msg->message == WM_HOTKEY && msg->wParam == kHotKeyId) {
            emit owner->activated();
            return true; // swallow it — no one else needs this message
        }
        return false;
    }
};

GlobalHotkey::GlobalHotkey(QObject *parent)
    : QObject(parent)
    , m_impl(new Impl)
{
    m_impl->owner = this;
}

GlobalHotkey::~GlobalHotkey()
{
    unregisterHotkey();
    if (m_impl->filterInstalled && QCoreApplication::instance())
        QCoreApplication::instance()->removeNativeEventFilter(m_impl);
    delete m_impl;
}

bool GlobalHotkey::setSequence(const QKeySequence &seq)
{
    if (seq.isEmpty())
        return false;

    const QKeySequence old = m_seq;
    const bool wasRegistered = m_registered;

    unregisterNative();
    m_registered = false;

    if (registerNative(seq)) {
        m_seq = seq;
        m_registered = true;
        return true;
    }

    // Roll back so a rejected combo doesn't leave the user with nothing.
    if (wasRegistered && registerNative(old)) {
        m_seq = old;
        m_registered = true;
    }
    return false;
}

bool GlobalHotkey::registerNative(const QKeySequence &seq)
{
    const QKeyCombination combo = seq[0];
    const int vk = winVirtualKey(combo.key());
    if (vk < 0)
        return false;

    if (!RegisterHotKey(nullptr, kHotKeyId, winModifiers(combo.keyboardModifiers()), UINT(vk)))
        return false;

    if (!m_impl->filterInstalled) {
        QCoreApplication::instance()->installNativeEventFilter(m_impl);
        m_impl->filterInstalled = true;
    }
    return true;
}

void GlobalHotkey::unregisterNative()
{
    UnregisterHotKey(nullptr, kHotKeyId);
}

void GlobalHotkey::unregisterHotkey()
{
    unregisterNative();
    m_registered = false;
}
