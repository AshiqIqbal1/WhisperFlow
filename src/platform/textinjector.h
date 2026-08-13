#ifndef TEXTINJECTOR_H
#define TEXTINJECTOR_H

#include <QString>

// Dictation output: place text on the clipboard and synthesize a paste
// keystroke into whichever app currently has focus. Platform backends:
//   mac — CGEvent Cmd+V; requires the Accessibility permission
//   win — SendInput Ctrl+V; no permission needed
namespace TextInjector {

// Can we synthesize keystrokes right now? (mac: Accessibility granted)
bool canInject();

// Trigger the OS permission flow if there is one (mac shows the prompt and
// System Settings pane). No-op on Windows.
void requestPermission();

// Clipboard-set + paste keystroke. Call only when canInject() is true.
void pasteIntoActiveApp(const QString &text);

} // namespace TextInjector

#endif // TEXTINJECTOR_H
