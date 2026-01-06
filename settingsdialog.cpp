#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "configmanager.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QDebug>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    initializeUI();
    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::initializeUI()
{
    // 连接 Ollama 标签页信号
    connect(ui->ollamaUrlEdit, &QLineEdit::textChanged, this, &SettingsDialog::onOllamaUrlChanged);
    connect(ui->modelNameEdit, &QLineEdit::textChanged, this, &SettingsDialog::onModelNameChanged);
    connect(ui->ollamaTimeoutSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &SettingsDialog::onOllamaTimeoutChanged);
    
    // 连接 UI 标签页信号
    connect(ui->themeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &SettingsDialog::onThemeChanged);
    
    // 连接 Pandoc 标签页信号
    connect(ui->pandocEnabledCheckBox, &QCheckBox::toggled, this, &SettingsDialog::onPandocToggled);
    connect(ui->pandocPathEdit, &QLineEdit::textChanged, this, &SettingsDialog::onPandocPathChanged);
    connect(ui->pandocTimeoutSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &SettingsDialog::onPandocTimeoutChanged);
    connect(ui->browsePandocButton, &QPushButton::clicked, this, &SettingsDialog::onBrowsePandocPath);
    
    // 连接日志标签页信号
    connect(ui->loggingLevelComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &SettingsDialog::onLoggingLevelChanged);
    connect(ui->openLogsFolderButton, &QPushButton::clicked, this, &SettingsDialog::onOpenLogsFolder);
    
    // 连接高级标签页信号
    connect(ui->autoRetryCheckBox, &QCheckBox::toggled, this, &SettingsDialog::onAutoRetryToggled);
    connect(ui->retryAttemptsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &SettingsDialog::onRetryAttemptsChanged);
    connect(ui->retryDelaySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &SettingsDialog::onRetryDelayChanged);
    
    // 连接对话框按钮
    connect(ui->buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, 
            this, &SettingsDialog::onApplyClicked);
    connect(ui->buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, 
            this, &SettingsDialog::onResetClicked);
    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, 
            this, &SettingsDialog::onOkClicked);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, 
            this, &SettingsDialog::onCancelClicked);
}

void SettingsDialog::loadSettings()
{
    ConfigManager &config = ConfigManager::instance();
    
    // 加载 Ollama 设置
    tempOllamaUrl = config.getOllamaUrl();
    ui->ollamaUrlEdit->setText(tempOllamaUrl);
    
    tempModelName = config.getOllamaModel();
    ui->modelNameEdit->setText(tempModelName);
    
    tempOllamaTimeout = config.getOllamaTimeout();
    ui->ollamaTimeoutSpinBox->setValue(tempOllamaTimeout);
    
    // 加载 UI 设置
    tempTheme = config.getTheme();
    if (tempTheme == "dark") {
        ui->themeComboBox->setCurrentIndex(0);
    } else {
        ui->themeComboBox->setCurrentIndex(1);
    }
    
    // 加载 Pandoc 设置
    tempPandocEnabled = config.isPandocEnabled();
    ui->pandocEnabledCheckBox->setChecked(tempPandocEnabled);
    
    tempPandocPath = config.getPandocPath();
    ui->pandocPathEdit->setText(tempPandocPath);
    
    tempPandocTimeout = config.getPandocTimeout();
    ui->pandocTimeoutSpinBox->setValue(tempPandocTimeout);
    
    // 更新 Pandoc 相关控件的启用状态
    ui->pandocPathEdit->setEnabled(tempPandocEnabled);
    ui->pandocPathLabel->setEnabled(tempPandocEnabled);
    ui->browsePandocButton->setEnabled(tempPandocEnabled);
    ui->pandocTimeoutSpinBox->setEnabled(tempPandocEnabled);
    ui->pandocTimeoutLabel->setEnabled(tempPandocEnabled);
    
    // 验证 Pandoc 路径
    validatePandocPath();
    
    // 加载日志设置
    tempLoggingLevel = config.getLoggingLevel();
    QStringList levels = {"DEBUG", "INFO", "WARN", "ERROR"};
    int levelIndex = levels.indexOf(tempLoggingLevel);
    if (levelIndex >= 0) {
        ui->loggingLevelComboBox->setCurrentIndex(levelIndex);
    }
    
    // 显示日志路径
    ui->loggingPathEdit->setText(config.getLoggingPath());
    
    // 加载高级设置
    tempAutoRetry = config.isAutoRetryEnabled();
    ui->autoRetryCheckBox->setChecked(tempAutoRetry);
    
    tempRetryAttempts = config.getRetryAttempts();
    ui->retryAttemptsSpinBox->setValue(tempRetryAttempts);
    
    tempRetryDelayMs = config.getRetryDelayMs();
    ui->retryDelaySpinBox->setValue(tempRetryDelayMs);
    
    // 更新重试相关控件的启用状态
    ui->retryAttemptsSpinBox->setEnabled(tempAutoRetry);
    ui->retryAttemptsLabel->setEnabled(tempAutoRetry);
    ui->retryDelaySpinBox->setEnabled(tempAutoRetry);
    ui->retryDelayLabel->setEnabled(tempAutoRetry);
}

void SettingsDialog::applySettings()
{
    // 验证设置
    if (!validateSettings()) {
        return;
    }
    
    ConfigManager &config = ConfigManager::instance();
    
    // 应用 Ollama 设置
    config.setOllamaUrl(tempOllamaUrl);
    config.setOllamaModel(tempModelName);
    config.setOllamaTimeout(tempOllamaTimeout);
    
    // 应用 UI 设置
    config.setTheme(tempTheme);
    
    // 应用 Pandoc 设置
    config.set("pandoc.enabled", tempPandocEnabled);
    config.set("pandoc.executablePath", tempPandocPath);
    config.set("pandoc.timeout", tempPandocTimeout);
    
    // 应用日志设置
    config.setLoggingLevel(tempLoggingLevel);
    
    // 应用高级设置
    config.setAutoRetry(tempAutoRetry);
    config.set("advanced.retryAttempts", tempRetryAttempts);
    config.set("advanced.retryDelayMs", tempRetryDelayMs);
    
    // 保存到文件
    if (!config.save()) {
        QMessageBox::critical(this, "保存失败", 
            QString("无法保存配置文件: %1").arg(config.getLastError()));
        return;
    }
    
    QMessageBox::information(this, "设置已保存", "配置已成功保存并应用。");
}

bool SettingsDialog::validateSettings()
{
    // 验证 Ollama URL
    if (tempOllamaUrl.isEmpty()) {
        QMessageBox::warning(this, "验证失败", "Ollama API URL 不能为空。");
        ui->tabWidget->setCurrentWidget(ui->ollamaTab);
        ui->ollamaUrlEdit->setFocus();
        return false;
    }
    
    if (!tempOllamaUrl.startsWith("http://") && !tempOllamaUrl.startsWith("https://")) {
        QMessageBox::warning(this, "验证失败", "Ollama API URL 必须以 http:// 或 https:// 开头。");
        ui->tabWidget->setCurrentWidget(ui->ollamaTab);
        ui->ollamaUrlEdit->setFocus();
        return false;
    }
    
    // 验证模型名称
    if (tempModelName.isEmpty()) {
        QMessageBox::warning(this, "验证失败", "模型名称不能为空。");
        ui->tabWidget->setCurrentWidget(ui->ollamaTab);
        ui->modelNameEdit->setFocus();
        return false;
    }
    
    // 验证 Pandoc 路径（如果启用）
    if (tempPandocEnabled) {
        if (tempPandocPath.isEmpty()) {
            QMessageBox::warning(this, "验证失败", "启用 Pandoc 时，可执行文件路径不能为空。");
            ui->tabWidget->setCurrentWidget(ui->pandocTab);
            ui->pandocPathEdit->setFocus();
            return false;
        }
        
        if (!isPandocAvailable(tempPandocPath)) {
            QMessageBox::StandardButton reply = QMessageBox::question(
                this, "Pandoc 不可用",
                "指定的 Pandoc 路径似乎不可用。是否仍要保存此设置？",
                QMessageBox::Yes | QMessageBox::No
            );
            if (reply == QMessageBox::No) {
                ui->tabWidget->setCurrentWidget(ui->pandocTab);
                ui->pandocPathEdit->setFocus();
                return false;
            }
        }
    }
    
    return true;
}

bool SettingsDialog::hasUnsavedChanges() const
{
    ConfigManager &config = ConfigManager::instance();
    
    // 检查是否有任何设置被修改
    return tempOllamaUrl != config.getOllamaUrl() ||
           tempModelName != config.getOllamaModel() ||
           tempOllamaTimeout != config.getOllamaTimeout() ||
           tempTheme != config.getTheme() ||
           tempPandocEnabled != config.isPandocEnabled() ||
           tempPandocPath != config.getPandocPath() ||
           tempPandocTimeout != config.getPandocTimeout() ||
           tempLoggingLevel != config.getLoggingLevel() ||
           tempAutoRetry != config.isAutoRetryEnabled() ||
           tempRetryAttempts != config.getRetryAttempts() ||
           tempRetryDelayMs != config.getRetryDelayMs();
}

void SettingsDialog::markAsModified()
{
    // 可以在这里添加视觉反馈，例如改变按钮状态
    // 目前我们让按钮始终可用
}

bool SettingsDialog::isPandocAvailable(const QString &path) const
{
    // 如果是绝对路径，检查文件是否存在
    QFileInfo fileInfo(path);
    if (fileInfo.isAbsolute()) {
        return fileInfo.exists() && fileInfo.isExecutable();
    }
    
    // 如果是命令名称，尝试执行 --version
    QProcess process;
    process.start(path, QStringList() << "--version");
    if (!process.waitForStarted(1000)) {
        return false;
    }
    if (!process.waitForFinished(3000)) {
        process.kill();
        return false;
    }
    
    return process.exitCode() == 0;
}

void SettingsDialog::validatePandocPath()
{
    QString path = ui->pandocPathEdit->text();
    if (path.isEmpty()) {
        ui->pandocStatusValueLabel->setText("未设置");
        ui->pandocStatusValueLabel->setStyleSheet("color: gray;");
        return;
    }
    
    if (isPandocAvailable(path)) {
        ui->pandocStatusValueLabel->setText("✓ 可用");
        ui->pandocStatusValueLabel->setStyleSheet("color: #2ecc71;");
    } else {
        ui->pandocStatusValueLabel->setText("✗ 不可用");
        ui->pandocStatusValueLabel->setStyleSheet("color: #e74c3c;");
    }
}

// Ollama 标签页槽函数
void SettingsDialog::onOllamaUrlChanged(const QString &text)
{
    tempOllamaUrl = text;
    markAsModified();
}

void SettingsDialog::onModelNameChanged(const QString &text)
{
    tempModelName = text;
    markAsModified();
}

void SettingsDialog::onOllamaTimeoutChanged(int value)
{
    tempOllamaTimeout = value;
    markAsModified();
}

// UI 标签页槽函数
void SettingsDialog::onThemeChanged(int index)
{
    tempTheme = (index == 0) ? "dark" : "light";
    markAsModified();
}

// Pandoc 标签页槽函数
void SettingsDialog::onPandocToggled(bool checked)
{
    tempPandocEnabled = checked;
    
    // 更新相关控件的启用状态
    ui->pandocPathEdit->setEnabled(checked);
    ui->pandocPathLabel->setEnabled(checked);
    ui->browsePandocButton->setEnabled(checked);
    ui->pandocTimeoutSpinBox->setEnabled(checked);
    ui->pandocTimeoutLabel->setEnabled(checked);
    
    markAsModified();
}

void SettingsDialog::onPandocPathChanged(const QString &text)
{
    tempPandocPath = text;
    validatePandocPath();
    markAsModified();
}

void SettingsDialog::onPandocTimeoutChanged(int value)
{
    tempPandocTimeout = value;
    markAsModified();
}

void SettingsDialog::onBrowsePandocPath()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "选择 Pandoc 可执行文件",
        QDir::homePath(),
        "可执行文件 (*);;所有文件 (*.*)"
    );
    
    if (!fileName.isEmpty()) {
        ui->pandocPathEdit->setText(fileName);
    }
}

// 日志标签页槽函数
void SettingsDialog::onLoggingLevelChanged(int index)
{
    QStringList levels = {"DEBUG", "INFO", "WARN", "ERROR"};
    if (index >= 0 && index < levels.size()) {
        tempLoggingLevel = levels[index];
        markAsModified();
    }
}

void SettingsDialog::onOpenLogsFolder()
{
    ConfigManager &config = ConfigManager::instance();
    QString logsPath = config.getLoggingPath();
    
    // 确保目录存在
    QDir dir(logsPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // 打开文件管理器
    QUrl url = QUrl::fromLocalFile(logsPath);
    if (!QDesktopServices::openUrl(url)) {
        QMessageBox::warning(this, "打开失败", 
            QString("无法打开日志文件夹: %1").arg(logsPath));
    }
}

// 高级标签页槽函数
void SettingsDialog::onAutoRetryToggled(bool checked)
{
    tempAutoRetry = checked;
    
    // 更新相关控件的启用状态
    ui->retryAttemptsSpinBox->setEnabled(checked);
    ui->retryAttemptsLabel->setEnabled(checked);
    ui->retryDelaySpinBox->setEnabled(checked);
    ui->retryDelayLabel->setEnabled(checked);
    
    markAsModified();
}

void SettingsDialog::onRetryAttemptsChanged(int value)
{
    tempRetryAttempts = value;
    markAsModified();
}

void SettingsDialog::onRetryDelayChanged(int value)
{
    tempRetryDelayMs = value;
    markAsModified();
}

// 对话框按钮槽函数
void SettingsDialog::onApplyClicked()
{
    applySettings();
}

void SettingsDialog::onResetClicked()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "重置设置",
        "是否要重新加载当前保存的设置？这将放弃所有未保存的修改。",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        loadSettings();
        QMessageBox::information(this, "设置已重置", "设置已恢复到上次保存的状态。");
    }
}

void SettingsDialog::onOkClicked()
{
    applySettings();
    accept();
}

void SettingsDialog::onCancelClicked()
{
    if (hasUnsavedChanges()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "未保存的修改",
            "您有未保存的修改。确定要放弃这些修改吗？",
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (reply == QMessageBox::No) {
            return;
        }
    }
    
    reject();
}
