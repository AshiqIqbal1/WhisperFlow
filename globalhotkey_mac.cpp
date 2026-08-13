// macOS backend for GlobalHotkey — Carbon's HIToolbox hotkey API.
//
// Why Carbon on a modern Mac: it's deprecated-but-functional, and it's the
// only systemwide-hotkey mechanism that doesn't require the user to grant
// Accessibility permission (the alternative, a CGEventTap, does). Utilities
// like Rectangle and Alfred lean on the exact same API for this reason.
#include "globalhotkey.h"

#include <Carbon/Carbon.h>

namespace {
constexpr UInt32 kHotKeySignature = 'WspF'; // arbitrary 4-char app namespace
constexpr UInt32 kHotKeyId = 1;
}

struct GlobalHotkey::Impl
{
    GlobalHotkey *owner = nullptr;
    EventHotKeyRef hotKeyRef = nullptr;
    EventHandlerRef handlerRef = nullptr;

    // Carbon fires this on the main thread, inside the app's normal event
    // loop, so emitting the Qt signal directly is safe — no queuing needed.
    static OSStatus callback(EventHandlerCallRef, EventRef event, void *userData)
    {
        auto *impl = static_cast<Impl *>(userData);

        EventHotKeyID hkID;
        GetEventParameter(event, kEventParamDirectObject, typeEventHotKeyID, nullptr,
                           sizeof(hkID), nullptr, &hkID);

        if (hkID.signature == kHotKeySignature && hkID.id == kHotKeyId)
            emit impl->owner->activated();

        return noErr;
    }
};

GlobalHotkey::GlobalHotkey(QObject *parent)
    : QObject(parent)
    , m_impl(new Impl{this, nullptr, nullptr})
{
}

GlobalHotkey::~GlobalHotkey()
{
    unregisterHotkey();
    delete m_impl;
}

bool GlobalHotkey::registerHotkey()
{
    if (m_impl->hotKeyRef)
        return true; // already registered

    EventTypeSpec spec{kEventClassKeyboard, kEventHotKeyPressed};
    InstallApplicationEventHandler(&GlobalHotkey::Impl::callback, 1, &spec,
                                    m_impl, &m_impl->handlerRef);

    // Cmd+Shift+R. kVK_ANSI_R is the physical-key code for "R" on a US
    // layout keyboard, independent of any modifier state.
    const EventHotKeyID hkID{kHotKeySignature, kHotKeyId};
    const OSStatus status = RegisterEventHotKey(kVK_ANSI_R, cmdKey | shiftKey, hkID,
                                                 GetApplicationEventTarget(), 0,
                                                 &m_impl->hotKeyRef);

    if (status != noErr) {
        RemoveEventHandler(m_impl->handlerRef);
        m_impl->handlerRef = nullptr;
        m_impl->hotKeyRef = nullptr;
        return false;
    }
    return true;
}

void GlobalHotkey::unregisterHotkey()
{
    if (m_impl->hotKeyRef) {
        UnregisterEventHotKey(m_impl->hotKeyRef);
        m_impl->hotKeyRef = nullptr;
    }
    if (m_impl->handlerRef) {
        RemoveEventHandler(m_impl->handlerRef);
        m_impl->handlerRef = nullptr;
    }
}

QString GlobalHotkey::comboLabel() const
{
    return QStringLiteral("⌘⇧R");
}
