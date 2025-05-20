#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ollamaclient.h" // Include ollamaclient
#include "screenshotoverlay.h" // Include screenshotoverlay
#include <QProcess>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_captureButton_clicked();
    void handleRecognitionSuccess(const QString &markdownFormula);
    void handleRecognitionError(const QString &errorString);
    void on_copyButton_clicked(); // 复制
    void on_exportButton_clicked(); // 导出
    // void handleScreenshotTaken(const QPixmap &pixmap); // If ScreenshotOverlay emits signal

    void on_editable_checkBox_clicked();

private:
    Ui::MainWindow *ui;
    OllamaClient *ollamaClient;
    // ScreenshotOverlay *overlay; // If using instance member

    QString convertMarkdownToMathML_Pandoc(const QString& markdownText);
    bool convertMdFileToDocx_Pandoc(const QString& mdFilePath, const QString& docxFilePath);
};
#endif // MAINWINDOW_H
