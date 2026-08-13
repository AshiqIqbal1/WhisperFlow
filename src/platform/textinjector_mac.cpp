#include "textinjector.h"

#include <QGuiApplication>
#include <QClipboard>
#include <QTimer>

#include <ApplicationServices/ApplicationServices.h>

bool TextInjector::canInject()
{
    return AXIsProcessTrusted();
}

void TextInjector::requestPermission()
{
    // Shows the system prompt and lists the app in
    // System Settings -> Privacy & Security -> Accessibility.
    const void *keys[] = {kAXTrustedCheckOptionPrompt};
    const void *values[] = {kCFBooleanTrue};
    CFDictionaryRef options = CFDictionaryCreate(kCFAllocatorDefault, keys, values, 1,
                                                 &kCFTypeDictionaryKeyCallBacks,
                                                 &kCFTypeDictionaryValueCallBacks);
    AXIsProcessTrustedWithOptions(options);
    CFRelease(options);
}

void TextInjector::pasteIntoActiveApp(const QString &text)
{
    QGuiApplication::clipboard()->setText(text);

    // Give the pasteboard a beat to sync before we fire Cmd+V, otherwise the
    // target app occasionally pastes the previous clipboard contents.
    QTimer::singleShot(150, [] {
        const CGKeyCode kVK_V = 9; // kVK_ANSI_V

        CGEventRef vDown = CGEventCreateKeyboardEvent(nullptr, kVK_V, true);
        CGEventRef vUp = CGEventCreateKeyboardEvent(nullptr, kVK_V, false);
        CGEventSetFlags(vDown, kCGEventFlagMaskCommand);
        CGEventSetFlags(vUp, kCGEventFlagMaskCommand);

        CGEventPost(kCGHIDEventTap, vDown);
        CGEventPost(kCGHIDEventTap, vUp);

        CFRelease(vDown);
        CFRelease(vUp);
    });
}
