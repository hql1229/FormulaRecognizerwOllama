#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QFileDialog> // For saving image if needed
#include <QTimer>
#include <QClipboard>    // 用于 QClipboard
#include <QDebug>        // 用于调试输出
#include <QMimeData>
#include <QDir>        // 用于处理路径和临时文件
#include <QTemporaryFile> // 用于创建临时文件 (更安全)
#include <QDesktopServices> // 用于打开文件

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // --- Set Window Icon ---
    QIcon icon = QIcon::fromTheme("ollama");
    if (icon.isNull()) {
        icon = QIcon("qrc:/resources/ollama.ico");
    }
    if (!icon.isNull()) {
        this->setWindowIcon(icon);
    } else {
        qDebug() << "Failed to load icon";
    }

    // --- 从 ConfigManager 加载配置 ---
    ConfigManager &config = ConfigManager::instance();

    // 恢复窗口几何状态
    QRect geometry = config.getWindowGeometry();
    if (!geometry.isEmpty()) {
        setGeometry(geometry);
    }

    QString windowState = config.getWindowState();
    if (windowState == "maximized") {
        showMaximized();
    } else if (windowState == "minimized") {
        showMinimized();
    }

    // --- UI Setup ---
    // 从配置加载 Ollama 设置
    ui->ollamaUrlLineEdit->setText(config.getOllamaUrl());
    ui->modelNameLineEdit->setText(config.getOllamaModel());

    // --- Set Window Border Color ---
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, QColor(42, 42, 42));  // 深灰色背景
    palette.setColor(QPalette::WindowText, QColor(236, 240, 241));  // 文字颜色
    palette.setColor(QPalette::Highlight, QColor(52, 152, 219));  // 选中时的高亮色
    this->setPalette(palette);

    // --- Load Style Sheet ---
    QFile styleFile("style.qss");
    if (styleFile.exists()) {
        styleFile.open(QFile::ReadOnly);
        QString style = QLatin1String(styleFile.readAll());
        this->setStyleSheet(style);
        styleFile.close();
    }else{
        qDebug() << "style file not exits.";
    }

    // --- Ollama Client ---
    ollamaClient = new OllamaClient(this);
    connect(ollamaClient, &OllamaClient::recognitionSuccess, this, &MainWindow::handleRecognitionSuccess);
    connect(ollamaClient, &OllamaClient::recognitionError, this, &MainWindow::handleRecognitionError);

    // --- 连接 Ollama 配置变更信号 ---
    connect(ui->ollamaUrlLineEdit, &QLineEdit::textChanged,
            this, [this](const QString &text) {
                ollamaClient->updateSettings(text, ui->modelNameLineEdit->text());
                // 保存到配置
                ConfigManager::instance().setOllamaUrl(text);
            });

    connect(ui->modelNameLineEdit, &QLineEdit::textChanged,
            this, [this](const QString &text) {
                ollamaClient->updateSettings(ui->ollamaUrlLineEdit->text(), text);
                // 保存到配置
                ConfigManager::instance().setOllamaModel(text);
            });

    // 连接配置变更信号
    connect(&config, &ConfigManager::configChanged,
            this, &MainWindow::onConfigChanged);

    // 初始化 Ollama 客户端设置
    ollamaClient->updateSettings(config.getOllamaUrl(), config.getOllamaModel());

    // --- Initial state for result text edit (supports some Markdown) ---
    ui->resultTextEdit->setMarkdown(""); // Clear initially
//    ui->resultTextEdit->setReadOnly(true);

    // --- Status Bar ---
    statusBar()->showMessage("Ready.");
}

MainWindow::~MainWindow()
{
    // 保存窗口几何状态到配置
    ConfigManager &config = ConfigManager::instance();
    config.setWindowGeometry(geometry());

    if (isMaximized()) {
        config.setWindowState("maximized");
    } else if (isMinimized()) {
        config.setWindowState("minimized");
    } else {
        config.setWindowState("normal");
    }

    // 保存配置到文件
    config.save();

    delete ui;
}

void MainWindow::on_captureButton_clicked()
{
    // Hide main window temporarily to not include it in screenshot
    this->hide();
     QTimer::singleShot(300, this, [this]() { // Small delay to ensure window is hidden
        QPixmap capturedPixmap = ScreenshotOverlay::takeScreenshot();
        this->show(); // Show main window again

        if (!capturedPixmap.isNull()) {
            ui->screenshotLabel->setPixmap(capturedPixmap.scaled(ui->screenshotLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            ui->resultTextEdit->setMarkdown("*Processing...*");
            statusBar()->showMessage("Sending image to Ollama...");

            // Update Ollama client settings if you have LineEdits for them
            // ollamaClient->setOllamaUrl(ui->ollamaUrlLineEdit->text());
            // ollamaClient->setModelName(ui->modelNameLineEdit->text());

            ollamaClient->recognizeFormula(capturedPixmap);
        } else {
            statusBar()->showMessage("Screenshot cancelled or failed.");
            ui->screenshotLabel->setText("Screenshot cancelled or invalid.");
        }
     });
}

void MainWindow::handleRecognitionSuccess(const QString &markdownFormula)
{
//    ui->resultTextEdit->setMarkdown(markdownFormula);
    ui->resultTextEdit->setPlainText(markdownFormula);
    statusBar()->showMessage("Recognition successful!", 5000);
}

void MainWindow::handleRecognitionError(const QString &errorString)
{
    ui->resultTextEdit->setMarkdown("**Error:**\n" + errorString);
    QMessageBox::critical(this, "Recognition Error", errorString);
    statusBar()->showMessage("Recognition failed.", 5000);
}

QString MainWindow::convertMarkdownToMathML_Pandoc(const QString &markdownText)
{
    if (markdownText.trimmed().isEmpty()) {
        return QString();
    }

    QProcess pandoc;

    QStringList arguments;
    // -f markdown: 输入格式为 markdown
    // -t mathml: 输出格式为 mathml
    // --mathjax:  一个可选的尝试，有时能改善数学的解析，但对纯 mathml 输出可能不是必须
    //             也可以尝试不加，或者用 --webtex (如果需要 URL 编码的数学)
//    arguments << "-f" << "markdown" << "-t" << "mathml";

//    arguments << "-f" << "markdown" << "-t" << "html5" << "--mathml" << "--wrap=none";

    arguments << "--from=markdown" // 输入格式为 Markdown
                  << "--to=latex"    // 输出格式为 latex
                  << "-s";            // 独立文档模式（可选，视需求）

    // 查找 pandoc 执行文件 (更健壮的做法是允许用户配置路径或从系统 PATH 查找)
    QString pandocExecutable = "pandoc"; // 假设 pandoc 在系统 PATH 中

    pandoc.start(pandocExecutable, arguments);

    if (!pandoc.waitForStarted(3000)) { // 等待 Pandoc 启动，超时3秒
        qWarning() << "Pandoc failed to start! Command:" << pandocExecutable << arguments.join(" ");
        qWarning() << "Pandoc error:" << pandoc.errorString();
        return QString();
    }

    pandoc.write(markdownText.toUtf8());
    pandoc.closeWriteChannel(); // 关闭写入，表示输入结束

    if (!pandoc.waitForFinished(5000)) { // 等待 Pandoc 完成，超时5秒
        qWarning() << "Pandoc timed out or failed to finish processing.";
        pandoc.kill(); // 强制结束
        return QString();
    }

    if (pandoc.exitStatus() == QProcess::CrashExit || pandoc.exitCode() != 0) {
        qWarning() << "Pandoc execution failed. Exit code:" << pandoc.exitCode();
        qWarning() << "Pandoc stdout:" << QString::fromUtf8(pandoc.readAllStandardOutput());
        qWarning() << "Pandoc stderr:" << QString::fromUtf8(pandoc.readAllStandardError());
        return QString();
    }

    QString mathMLOutput = QString::fromUtf8(pandoc.readAllStandardOutput());
    qDebug() << "Pandoc MathML Output:\n" << mathMLOutput;
    return mathMLOutput;
}

bool MainWindow::convertMdFileToDocx_Pandoc(const QString &mdFilePath, const QString &docxFilePath)
{
    if (mdFilePath.isEmpty() || docxFilePath.isEmpty()) {
        qWarning() << "MD或DOCX文件路径为空。";
        return false;
    }

    QProcess pandoc;
    QStringList arguments;
    // -f markdown: 输入格式
    // -t docx: 输出格式
    // -o <output_file>: 指定输出文件名
    // --standalone (-s): 确保生成一个完整的、可独立打开的 docx 文件
    arguments << "-f" << "markdown" << "-s" << "-t" << "docx" << "-o" << docxFilePath << mdFilePath;

    QString pandocExecutable = "pandoc"; // 或 Pandoc 的完整路径
    qInfo() << "Pandoc command:" << pandocExecutable << arguments.join(" ");

    pandoc.start(pandocExecutable, arguments);

    if (!pandoc.waitForStarted(3000)) {
        qWarning() << "Pandoc (MD->DOCX) failed to start! Command:" << pandocExecutable << arguments.join(" ");
        qWarning() << "Pandoc error:" << pandoc.errorString();
        return false;
    }

    if (!pandoc.waitForFinished(10000)) { // 转换可能需要更长时间，增加超时
        qWarning() << "Pandoc (MD->DOCX) timed out or failed to finish processing.";
        pandoc.kill();
        return false;
    }
    qDebug() << "pandoc.exitStatus():"<< pandoc.exitStatus();
    qDebug() << "pandoc.exitCode()"<< pandoc.exitCode();

    if (pandoc.exitStatus() == QProcess::CrashExit || pandoc.exitCode() != 0) {
        qWarning() << "Pandoc (MD->DOCX) execution failed. Exit code:" << pandoc.exitCode();
        // 读取标准输出和标准错误以获取更多信息
        QByteArray stdOutData = pandoc.readAllStandardOutput();
        QByteArray stdErrData = pandoc.readAllStandardError();
        if (!stdOutData.isEmpty()) {
            qWarning() << "Pandoc stdout:" << QString::fromUtf8(stdOutData);
        }
        if (!stdErrData.isEmpty()) {
            qWarning() << "Pandoc stderr:" << QString::fromUtf8(stdErrData);
        }
        return false;
    }

    qInfo() << "Pandoc (MD->DOCX) conversion successful for" << docxFilePath;
    return true;
}

void MainWindow::on_copyButton_clicked()
{
    // 从 QTextEdit 获取 Markdown 文本 (Ollama 的原始输出)
    QString markdownSourceText = ui->resultTextEdit->toPlainText();

    if (markdownSourceText.trimmed().isEmpty()) {
        statusBar()->showMessage("结果为空，无法复制。", 3000);
        return;
    }

    QClipboard *clipboard = QGuiApplication::clipboard();
    QMimeData *mimeData = new QMimeData();

    // 1. 设置纯文本 (原始 Markdown/LaTeX)
    //    这是最通用的，如果其他格式粘贴失败，Word 会使用这个。
    mimeData->setText(markdownSourceText);
    qDebug() << "复制到剪贴板 (text/plain):" << markdownSourceText;

    // 2. 尝试使用 Pandoc 将 Markdown 转换为 MathML
    //    Pandoc 可以处理 Markdown 中的 LaTeX 数学块 ($...$, $$...$$)
    QString mathMLContent = convertMarkdownToMathML_Pandoc(markdownSourceText);

    if (!mathMLContent.isEmpty()) {
        // Word 期望的 MathML MIME 类型是 "application/mathml+xml"
        // 或者 "application/mathml-presentation+xml"
        mimeData->setData("application/mathml+xml", mathMLContent.toUtf8());
        qDebug() << "复制到剪贴板 (application/mathml+xml):\n" << mathMLContent;
    } else {
        qDebug() << "Pandoc 转换 MathML 失败或返回空结果。";
    }

    // (可选) 尝试生成 OMML
    // 如前所述，直接从 Markdown/LaTeX 生成纯 OMML 片段比较困难。
    // 如果你确实需要，可能要通过 Pandoc 生成 .docx 然后解析，或者找到专门的库。
    // 对于大多数情况，高质量的 MathML 和原始 LaTeX 已经足够好。

    clipboard->setMimeData(mimeData); // mimeData 的所有权转移给剪贴板

    if (!mathMLContent.isEmpty()) {
        statusBar()->showMessage("公式已复制 (含MathML和纯文本)", 4000);
    } else {
        statusBar()->showMessage("公式已复制 (纯文本，MathML转换失败)", 4000);
    }
}

void MainWindow::on_exportButton_clicked()
{
    QString markdownSourceText = ui->resultTextEdit->toPlainText();
    if (markdownSourceText.trimmed().isEmpty()) {
        statusBar()->showMessage("结果为空，无法导出。", 3000);
        return;
    }

    // 1. 创建一个临时的 .md 文件来保存 Markdown 内容
    // 使用 QTemporaryFile 来确保文件名唯一且在作用域结束时可以自动删除（如果需要）
    // 或者，我们可以指定一个固定的临时文件名，并在之后手动删除。
    // 为了简单起见，我们先用一个固定目录下的临时文件名。

    QString tempDir = QDir::currentPath(); // 获取系统临时目录
    QString tempMdFileName = "formula_temp.md";
    QString tempMdFilePath = tempDir + QDir::separator() + tempMdFileName;

    QFile tempMdFile(tempMdFilePath);
    if (!tempMdFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "无法创建临时 Markdown 文件:" << tempMdFilePath << tempMdFile.errorString();
        statusBar()->showMessage("错误：无法创建临时文件。", 5000);
        return;
    }
    QTextStream outMd(&tempMdFile);
    outMd.setCodec("UTF-8"); // 确保使用 UTF-8 编码
    outMd << markdownSourceText;
    tempMdFile.close();
    qInfo() << "Markdown内容已保存到临时文件:" << tempMdFilePath;

    // 2. 定义输出的 .docx 文件名
    QString tempDocxFileName = "formula_output.docx";
    QString tempDocxFilePath = tempDir + QDir::separator() + tempDocxFileName;

    // 删除可能已存在的旧的 docx 文件，以避免 Pandoc 写入权限问题或追加内容
    if (QFile::exists(tempDocxFilePath)) {
        if (!QFile::remove(tempDocxFilePath)) {
            qWarning() << "无法删除已存在的旧 DOCX 文件:" << tempDocxFilePath;
            // 可以选择继续尝试，Pandoc 可能会覆盖它
        }
    }

    // 3. 调用 Pandoc 将 .md 文件转换为 .docx 文件
    statusBar()->showMessage("正在转换到 Word 文档...", 0); // 0 表示一直显示直到被覆盖
    qApp->processEvents(); // 处理事件，确保状态栏消息显示

    if (convertMdFileToDocx_Pandoc(tempMdFilePath, tempDocxFilePath)) {
        statusBar()->showMessage("转换成功！正在打开 Word 文档...", 3000);
        qInfo() << "DOCX 文件已生成:" << tempDocxFilePath;

        // 4. 使用默认程序打开生成的 .docx 文件
        bool opened = QDesktopServices::openUrl(QUrl::fromLocalFile(tempDocxFilePath));
        if (!opened) {
            qWarning() << "无法使用默认程序打开 DOCX 文件:" << tempDocxFilePath;
            QMessageBox::warning(this, "打开文件失败",
                                 QString("无法自动打开 Word 文档：\n%1\n\n请尝试手动打开。")
                                 .arg(QDir::toNativeSeparators(tempDocxFilePath)));
            statusBar()->showMessage("无法自动打开 Word 文档，请手动打开。", 5000);
        } else {
            statusBar()->showMessage("Word 文档已打开。", 5000);
        }
    } else {
        qWarning() << "Pandoc 转换 MD 到 DOCX 失败。";
        QMessageBox::critical(this, "转换失败",
                              "使用 Pandoc 将 Markdown 转换为 Word 文档失败。\n请检查 Pandoc 是否已正确安装并配置在系统 PATH 中，以及是否有写入临时目录的权限。");
        statusBar()->showMessage("转换为 Word 文档失败。", 5000);
    }

    // 5. (可选) 清理临时的 .md 文件
    // 如果希望保留 .md 文件用于调试，可以注释掉下面这行
    if (QFile::exists(tempMdFilePath)) {
        if (!tempMdFile.remove()) { // tempMdFile 对象在这里仍然有效
             qWarning() << "无法删除临时 Markdown 文件:" << tempMdFilePath << tempMdFile.errorString();
        } else {
            qInfo() << "临时 Markdown 文件已删除:" << tempMdFilePath;
        }
    }
}

void MainWindow::on_editable_checkBox_clicked()
{
    ui->resultTextEdit->setReadOnly(!ui->editable_checkBox->checkState());
}

void MainWindow::onConfigChanged(const QString &key)
{
    ConfigManager &config = ConfigManager::instance();

    if (key.startsWith("ollama.")) {
        // Ollama 配置变更，更新客户端
        ollamaClient->updateSettings(
            config.getOllamaUrl(),
            config.getOllamaModel()
        );
        qDebug() << "Ollama 配置已更新:" << key;
    } else if (key.startsWith("ui.theme")) {
        // 主题变更
        QString theme = config.getTheme();
        // 如果需要支持主题切换，可以在这里添加逻辑
        qDebug() << "主题已变更为:" << theme;
    } else if (key.startsWith("ui.")) {
        // 其他 UI 配置变更
        qDebug() << "UI 配置已变更:" << key;
    }
}
