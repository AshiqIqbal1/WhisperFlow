#ifndef ICONS_H
#define ICONS_H

#include <QIcon>
#include <QColor>

// Vector icons drawn from inline SVG so the app ships with zero image assets
// and stays crisp on any DPI (Windows 125%/150% scaling included).
namespace Icons {

enum Name {
    Search,
    Mic,
    Play,
    Copy,
    Retry,
    Trash,
    Settings,
    ChevronDown,
    ChevronUp,
    FileDown,
    Waveform,
    Minimize,
    Close
};

QIcon icon(Name name, const QColor &color, int px = 18);

} // namespace Icons

#endif // ICONS_H
