#ifndef GLOBALHOTKEY_H
#define GLOBALHOTKEY_H

#include <QObject>

// Cross-platform "works even when the app isn't focused" hotkey.
// Fixed combo for now: Cmd+Shift+R on macOS, Ctrl+Shift+R on Windows — not
// user-remappable yet (see SettingsDialog for the follow-up note). Backed
// by Carbon's RegisterEventHotKey on macOS (globalhotkey_mac.mm) and
// Win32's RegisterHotKey (globalhotkey_win.cpp); pick whichever the
// platform build compiles in via CMakeLists.
class GlobalHotkey : public QObject
{
    Q_OBJECT

public:
    explicit GlobalHotkey(QObject *parent = nullptr);
    ~GlobalHotkey() override;

    // Returns false if the OS refused to register the combo (usually means
    // another app already grabbed it).
    bool registerHotkey();
    void unregisterHotkey();

    QString comboLabel() const;

signals:
    void activated();

private:
    struct Impl;
    Impl *m_impl = nullptr;
};

#endif // GLOBALHOTKEY_H
