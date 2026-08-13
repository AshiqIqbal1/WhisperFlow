#include "icons.h"

#include <QGuiApplication>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>

namespace {

// Stroke-style glyph bodies, 24x24 viewBox.
QString body(Icons::Name name)
{
    switch (name) {
    case Icons::Search:
        return QStringLiteral(R"(<circle cx="11" cy="11" r="7"/><path d="m20 20-3.6-3.6"/>)");
    case Icons::Mic:
        return QStringLiteral(R"(<path d="M12 2a3 3 0 0 0-3 3v7a3 3 0 0 0 6 0V5a3 3 0 0 0-3-3Z"/>)"
                              R"(<path d="M19 10v2a7 7 0 0 1-14 0v-2"/><path d="M12 19v3"/>)");
    case Icons::Play:
        return QStringLiteral(R"(<path d="M8 5.5v13l11-6.5z" fill="currentFill" stroke-width="1.5"/>)");
    case Icons::Copy:
        return QStringLiteral(R"(<rect x="8" y="8" width="13" height="13" rx="2.5"/>)"
                              R"(<path d="M4.5 16A2.5 2.5 0 0 1 3 13.5v-9A2.5 2.5 0 0 1 5.5 3h8A2.5 2.5 0 0 1 16 5.5"/>)");
    case Icons::Retry:
        return QStringLiteral(R"(<path d="M20.5 12a8.5 8.5 0 1 1-2.49-6.01"/><path d="M20.5 3.5V9H15"/>)");
    case Icons::Trash:
        return QStringLiteral(R"(<path d="M3.5 6h17"/>)"
                              R"(<path d="M18.5 6v13a2 2 0 0 1-2 2h-9a2 2 0 0 1-2-2V6"/>)"
                              R"(<path d="M9 6V4.5A1.5 1.5 0 0 1 10.5 3h3A1.5 1.5 0 0 1 15 4.5V6"/>)");
    case Icons::Settings:
        return QStringLiteral(R"(<circle cx="12" cy="12" r="3"/>)"
                              R"(<path d="M19.4 15a1.6 1.6 0 0 0 .32 1.77l.06.06a2 2 0 1 1-2.83 2.83l-.06-.06a1.6 1.6 0 0 0-1.77-.32 1.6 1.6 0 0 0-1 1.47V21a2 2 0 1 1-4 0v-.11a1.6 1.6 0 0 0-1.05-1.46 1.6 1.6 0 0 0-1.77.32l-.06.06a2 2 0 1 1-2.83-2.83l.06-.06a1.6 1.6 0 0 0 .32-1.77 1.6 1.6 0 0 0-1.47-1H3a2 2 0 1 1 0-4h.11a1.6 1.6 0 0 0 1.46-1.05 1.6 1.6 0 0 0-.32-1.77l-.06-.06a2 2 0 1 1 2.83-2.83l.06.06a1.6 1.6 0 0 0 1.77.32H9a1.6 1.6 0 0 0 1-1.47V3a2 2 0 1 1 4 0v.11a1.6 1.6 0 0 0 1 1.47 1.6 1.6 0 0 0 1.77-.32l.06-.06a2 2 0 1 1 2.83 2.83l-.06.06a1.6 1.6 0 0 0-.32 1.77V9a1.6 1.6 0 0 0 1.47 1H21a2 2 0 1 1 0 4h-.11a1.6 1.6 0 0 0-1.47 1Z"/>)");
    case Icons::ChevronDown:
        return QStringLiteral(R"(<path d="m6 9.5 6 6 6-6"/>)");
    case Icons::ChevronUp:
        return QStringLiteral(R"(<path d="m6 14.5 6-6 6 6"/>)");
    case Icons::FileDown:
        return QStringLiteral(R"(<path d="M12 3v11"/><path d="m7.5 10 4.5 4.5 4.5-4.5"/>)"
                              R"(<path d="M4 17v2a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2v-2"/>)");
    case Icons::Waveform:
        return QStringLiteral(R"(<path d="M3 12h2"/><path d="M8 7v10"/><path d="M12 4v16"/>)"
                              R"(<path d="M16 8v8"/><path d="M20 11v2"/>)");
    case Icons::Minimize:
        return QStringLiteral(R"(<path d="M5 12h14"/>)");
    case Icons::Close:
        return QStringLiteral(R"(<path d="m6 6 12 12"/><path d="m18 6-12 12"/>)");
    }
    return QString();
}

} // namespace

QIcon Icons::icon(Name name, const QColor &color, int px)
{
    const QString hex = color.name(QColor::HexRgb);

    QString svg = QStringLiteral(
        R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" )"
        R"(stroke="%1" stroke-width="1.9" stroke-linecap="round" stroke-linejoin="round">%2</svg>)")
        .arg(hex, body(name));

    // The play glyph is the only filled shape; patch its placeholder.
    svg.replace(QStringLiteral("currentFill"), hex);

    // Render at device pixel ratio so it stays sharp on scaled displays.
    const qreal dpr = qApp ? qApp->devicePixelRatio() : 1.0;
    QPixmap pm(QSize(px, px) * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);

    QSvgRenderer renderer(svg.toUtf8());
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    renderer.render(&p, QRectF(0, 0, px, px));
    p.end();

    return QIcon(pm);
}
