#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
    class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

private slots:
    // Ollama 标签页
    void onOllamaUrlChanged(const QString &text);
    void onModelNameChanged(const QString &text);
    void onOllamaTimeoutChanged(int value);
    
    // UI 标签页
    void onThemeChanged(int index);
    
    // Pandoc 标签页
    void onPandocToggled(bool checked);
    void onPandocPathChanged(const QString &text);
    void onPandocTimeoutChanged(int value);
    void onBrowsePandocPath();
    
    // 日志标签页
    void onLoggingLevelChanged(int index);
    void onOpenLogsFolder();
    
    // 高级标签页
    void onAutoRetryToggled(bool checked);
    void onRetryAttemptsChanged(int value);
    void onRetryDelayChanged(int value);
    
    // 对话框按钮
    void onApplyClicked();
    void onResetClicked();
    void onOkClicked();
    void onCancelClicked();
    
    // Pandoc 路径验证
    void validatePandocPath();

private:
    Ui::SettingsDialog *ui;
    
    // 临时存储修改（未应用）
    QString tempOllamaUrl;
    QString tempModelName;
    int tempOllamaTimeout;
    QString tempTheme;
    bool tempPandocEnabled;
    QString tempPandocPath;
    int tempPandocTimeout;
    QString tempLoggingLevel;
    bool tempAutoRetry;
    int tempRetryAttempts;
    int tempRetryDelayMs;
    
    // 初始化 UI
    void initializeUI();
    void loadSettings();
    void applySettings();
    bool validateSettings();
    
    // 标记是否有未保存的修改
    bool hasUnsavedChanges() const;
    void markAsModified();
    
    // 辅助方法
    bool isPandocAvailable(const QString &path) const;
};

#endif // SETTINGSDIALOG_H
