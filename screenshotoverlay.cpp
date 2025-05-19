#include "screenshotoverlay.h"
#include <QApplication> // For QApplication::desktop() in older Qt, or QGuiApplication::primaryScreen()

ScreenshotOverlay* ScreenshotOverlay::instance = nullptr;

ScreenshotOverlay::ScreenshotOverlay(QWidget *parent) : QWidget(parent), selecting(false)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setCursor(Qt::CrossCursor);

    // Grab the entire desktop
    QScreen *screen = QGuiApplication::primaryScreen();
//    if (screen) {
//        desktopPixmap = screen->grabWindow(0); // 0 captures the whole screen
//    }
    if (screen) {
        desktopPixmap = screen->grabWindow(0); // 0 captures the whole screen
        qDebug() << "ScreenshotOverlay Constructor: desktopPixmap.isNull():" << desktopPixmap.isNull()
                 << "Size:" << desktopPixmap.size()
                 << "Depth:" << desktopPixmap.depth();
        if (desktopPixmap.isNull()) {
            qWarning() << "!!! CRITICAL: screen->grabWindow(0) returned a NULL pixmap!";
        } else {
            // Optional: Save the full desktop grab for initial check
            // QString savePath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
            // desktopPixmap.save(savePath + "/debug_fulldesktop_grab.png");
            // qDebug() << "Saved initial full desktop grab to " << savePath + "/debug_fulldesktop_grab.png";
        }
    } else {
        qDebug() << "!!! CRITICAL: ScreenshotOverlay Constructor: No primary screen found!";
    }
    if (!desktopPixmap.isNull()) {
        resize(desktopPixmap.size());
    } else {
        qWarning("Failed to grab desktop image in ScreenshotOverlay constructor. Overlay might not work correctly.");
        // Fallback size if grab failed, though functionality will be impaired
        resize(QGuiApplication::primaryScreen() ? QGuiApplication::primaryScreen()->geometry().size() : QSize(1024, 768));
    }
}

QPixmap ScreenshotOverlay::takeScreenshot() {
    if (instance) {
        instance->disconnect(); // Disconnect any previous connections
        instance->deleteLater();
    }
    instance = new ScreenshotOverlay(); // This will call the constructor and grab the screen
    if (instance->desktopPixmap.isNull()) {
        qWarning() << "takeScreenshot: Instance created, but its desktopPixmap is null. Aborting.";
        instance->deleteLater();
        instance = nullptr;
        return QPixmap(); // Return empty pixmap
    }
    instance->showFullScreen();

    // This is a bit tricky. We need to wait for the user to select.
    // For simplicity here, we'll use a modal approach.
    // A more robust solution would involve signals/slots properly.
    // For this example, we'll block until a selection is made or canceled.
    QPixmap capturedPixmap;
    QEventLoop loop;
    connect(instance, &ScreenshotOverlay::screenshotTaken, [&](const QPixmap& pix) {
        capturedPixmap = pix;
        loop.quit();
    });
    connect(instance, &ScreenshotOverlay::destroyed, &loop, &QEventLoop::quit); // Handle ESC
    loop.exec(); // This blocks until loop.quit() is called

    if (instance) {
       instance->deleteLater();
       instance = nullptr;
    }
    return capturedPixmap;
}


void ScreenshotOverlay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw the semi-transparent overlay
    painter.fillRect(rect(), QColor(0, 0, 0, 120));

    if (selecting && !selectionRect.isNull()) {
        // Clear the selected area to show what's underneath
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.fillRect(selectionRect, Qt::transparent);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

        // Draw a border around the selection
        painter.setPen(QPen(Qt::red, 2));
        painter.drawRect(selectionRect);
    }
}

void ScreenshotOverlay::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        selecting = true;
        startPoint = event->pos();
        selectionRect = QRect(startPoint, QSize());
        update();
    }
}

void ScreenshotOverlay::mouseMoveEvent(QMouseEvent *event)
{
    if (selecting) {
        selectionRect = QRect(startPoint, event->pos()).normalized();
        update();
    }
}

void ScreenshotOverlay::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && selecting) {
        selecting = false;
        if (!selectionRect.isNull() && selectionRect.width() > 5 && selectionRect.height() > 5) {
            QPixmap captured = desktopPixmap.copy(selectionRect);
            emit screenshotTaken(captured); // Emit the signal
        } else {
            emit screenshotTaken(QPixmap()); // Emit empty pixmap if selection is too small or invalid
        }
        close(); // Close the overlay
    }
}

void ScreenshotOverlay::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit screenshotTaken(QPixmap()); // Emit empty pixmap on cancel
        close();
    }
}
