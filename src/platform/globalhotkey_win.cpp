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
}

struct GlobalHotkey::Impl : public QAbstractNativeEventFilter
{
    GlobalHotkey *owner = nullptr;
    bool registered = false;

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
    delete m_impl;
}

bool GlobalHotkey::registerHotkey()
{
    if (m_impl->registered)
        return true;

    // Ctrl+Shift+R, systemwide. MOD_NOREPEAT stops auto-repeat from firing
    // the toggle machine-gun style while the keys are held.
    if (!RegisterHotKey(nullptr, kHotKeyId, MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT, 'R'))
        return false;

    QCoreApplication::instance()->installNativeEventFilter(m_impl);
    m_impl->registered = true;
    return true;
}

void GlobalHotkey::unregisterHotkey()
{
    if (!m_impl->registered)
        return;

    UnregisterHotKey(nullptr, kHotKeyId);
    QCoreApplication::instance()->removeNativeEventFilter(m_impl);
    m_impl->registered = false;
}

QString GlobalHotkey::comboLabel() const
{
    return QStringLiteral("Ctrl+Shift+R");
}
