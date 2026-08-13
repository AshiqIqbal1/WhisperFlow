#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <QWidget>

// Custom window chrome: the OS title bar is hidden (frameless window) and
// this draws logo + app name + minimize/close, and drags the window.
class TitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
};

#endif // TITLEBAR_H
