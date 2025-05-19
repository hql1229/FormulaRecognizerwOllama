#ifndef SCREENSHOTOVERLAY_H
#define SCREENSHOTOVERLAY_H

#include <QWidget>
#include <QPixmap>
#include <QRect>
#include <QMouseEvent>
#include <QScreen>
#include <QGuiApplication>
#include <QPainter>
#include <QDebug>

class ScreenshotOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit ScreenshotOverlay(QWidget *parent = nullptr);
    static QPixmap takeScreenshot(); // Static method to initiate and return screenshot

signals:
    void screenshotTaken(const QPixmap &pixmap);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QRect selectionRect;
    QPoint startPoint;
    bool selecting;
    QPixmap desktopPixmap;

    static ScreenshotOverlay* instance; // For the static method
};

#endif // SCREENSHOTOVERLAY_H
