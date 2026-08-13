#ifndef GLOBALHOTKEY_H
#define GLOBALHOTKEY_H

#include <QKeySequence>
#include <QObject>

// Cross-platform "works even when the app isn't focused" hotkey.
// Backed by Carbon's RegisterEventHotKey on macOS (globalhotkey_mac.cpp)
// and Win32's RegisterHotKey (globalhotkey_win.cpp); CMakeLists compiles in
// whichever backend matches the platform.
//
// The combo is remappable: setSequence() re-registers on the fly and
// returns false (keeping the previous registration) if the OS refuses the
// combo or we can't translate the key to a native code. Write "Ctrl+Shift+R"
// style portable strings — Qt maps Ctrl to Cmd on macOS automatically.
class GlobalHotkey : public QObject
{
    Q_OBJECT

public:
    explicit GlobalHotkey(QObject *parent = nullptr);
    ~GlobalHotkey() override;

    static QKeySequence defaultSequence() { return QKeySequence(QStringLiteral("Ctrl+Shift+R")); }

    // Registers `seq` (first chord only), replacing any current registration.
    // On failure the old combo stays active and this returns false.
    bool setSequence(const QKeySequence &seq);
    QKeySequence sequence() const { return m_seq; }

    void unregisterHotkey();

    // Native-looking label for UI hints: "⌘⇧R" on mac, "Ctrl+Shift+R" on win.
    QString comboLabel() const { return m_seq.toString(QKeySequence::NativeText); }

signals:
    void activated();

private:
    // Platform backend: register exactly this combination, or fail cleanly.
    bool registerNative(const QKeySequence &seq);
    void unregisterNative();

    struct Impl;
    Impl *m_impl = nullptr;
    QKeySequence m_seq;
    bool m_registered = false;
};

#endif // GLOBALHOTKEY_H
