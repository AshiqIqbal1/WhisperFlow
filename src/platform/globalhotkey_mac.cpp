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

// Qt logical key -> Carbon *physical* keycode (ANSI layout positions).
// Carbon keycodes are scattered, so this has to be a table. Returns -1 for
// keys we don't support as a hotkey trigger.
int carbonKeyCode(Qt::Key key)
{
    switch (key) {
    case Qt::Key_A: return kVK_ANSI_A;
    case Qt::Key_B: return kVK_ANSI_B;
    case Qt::Key_C: return kVK_ANSI_C;
    case Qt::Key_D: return kVK_ANSI_D;
    case Qt::Key_E: return kVK_ANSI_E;
    case Qt::Key_F: return kVK_ANSI_F;
    case Qt::Key_G: return kVK_ANSI_G;
    case Qt::Key_H: return kVK_ANSI_H;
    case Qt::Key_I: return kVK_ANSI_I;
    case Qt::Key_J: return kVK_ANSI_J;
    case Qt::Key_K: return kVK_ANSI_K;
    case Qt::Key_L: return kVK_ANSI_L;
    case Qt::Key_M: return kVK_ANSI_M;
    case Qt::Key_N: return kVK_ANSI_N;
    case Qt::Key_O: return kVK_ANSI_O;
    case Qt::Key_P: return kVK_ANSI_P;
    case Qt::Key_Q: return kVK_ANSI_Q;
    case Qt::Key_R: return kVK_ANSI_R;
    case Qt::Key_S: return kVK_ANSI_S;
    case Qt::Key_T: return kVK_ANSI_T;
    case Qt::Key_U: return kVK_ANSI_U;
    case Qt::Key_V: return kVK_ANSI_V;
    case Qt::Key_W: return kVK_ANSI_W;
    case Qt::Key_X: return kVK_ANSI_X;
    case Qt::Key_Y: return kVK_ANSI_Y;
    case Qt::Key_Z: return kVK_ANSI_Z;
    case Qt::Key_0: return kVK_ANSI_0;
    case Qt::Key_1: return kVK_ANSI_1;
    case Qt::Key_2: return kVK_ANSI_2;
    case Qt::Key_3: return kVK_ANSI_3;
    case Qt::Key_4: return kVK_ANSI_4;
    case Qt::Key_5: return kVK_ANSI_5;
    case Qt::Key_6: return kVK_ANSI_6;
    case Qt::Key_7: return kVK_ANSI_7;
    case Qt::Key_8: return kVK_ANSI_8;
    case Qt::Key_9: return kVK_ANSI_9;
    case Qt::Key_F1: return kVK_F1;
    case Qt::Key_F2: return kVK_F2;
    case Qt::Key_F3: return kVK_F3;
    case Qt::Key_F4: return kVK_F4;
    case Qt::Key_F5: return kVK_F5;
    case Qt::Key_F6: return kVK_F6;
    case Qt::Key_F7: return kVK_F7;
    case Qt::Key_F8: return kVK_F8;
    case Qt::Key_F9: return kVK_F9;
    case Qt::Key_F10: return kVK_F10;
    case Qt::Key_F11: return kVK_F11;
    case Qt::Key_F12: return kVK_F12;
    case Qt::Key_Space: return kVK_Space;
    case Qt::Key_Left: return kVK_LeftArrow;
    case Qt::Key_Right: return kVK_RightArrow;
    case Qt::Key_Up: return kVK_UpArrow;
    case Qt::Key_Down: return kVK_DownArrow;
    default: return -1;
    }
}

// On macOS Qt swaps Control/Meta: Qt::ControlModifier is the Command key.
UInt32 carbonModifiers(Qt::KeyboardModifiers mods)
{
    UInt32 native = 0;
    if (mods & Qt::ControlModifier) native |= cmdKey;
    if (mods & Qt::MetaModifier)    native |= controlKey;
    if (mods & Qt::AltModifier)     native |= optionKey;
    if (mods & Qt::ShiftModifier)   native |= shiftKey;
    return native;
}

} // namespace

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
    const int keyCode = carbonKeyCode(combo.key());
    if (keyCode < 0)
        return false;

    if (!m_impl->handlerRef) {
        EventTypeSpec spec{kEventClassKeyboard, kEventHotKeyPressed};
        InstallApplicationEventHandler(&GlobalHotkey::Impl::callback, 1, &spec,
                                        m_impl, &m_impl->handlerRef);
    }

    const EventHotKeyID hkID{kHotKeySignature, kHotKeyId};
    const OSStatus status = RegisterEventHotKey(UInt32(keyCode),
                                                 carbonModifiers(combo.keyboardModifiers()),
                                                 hkID, GetApplicationEventTarget(), 0,
                                                 &m_impl->hotKeyRef);
    if (status != noErr) {
        m_impl->hotKeyRef = nullptr;
        return false;
    }
    return true;
}

void GlobalHotkey::unregisterNative()
{
    if (m_impl->hotKeyRef) {
        UnregisterEventHotKey(m_impl->hotKeyRef);
        m_impl->hotKeyRef = nullptr;
    }
}

void GlobalHotkey::unregisterHotkey()
{
    unregisterNative();
    if (m_impl->handlerRef) {
        RemoveEventHandler(m_impl->handlerRef);
        m_impl->handlerRef = nullptr;
    }
    m_registered = false;
}
