#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QRect>
#include <QJsonObject>
#include <QMutex>

class ConfigManager : public QObject
{
    Q_OBJECT

public:
    // 获取全局唯一实例
    static ConfigManager& instance();

    // 禁用拷贝构造和赋值操作
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // 配置读取方法
    QString getOllamaUrl() const;
    QString getOllamaModel() const;
    int getOllamaTimeout() const;
    QRect getWindowGeometry() const;
    QString getWindowState() const;
    QString getTheme() const;
    bool isPandocEnabled() const;
    QString getPandocPath() const;
    int getPandocTimeout() const;
    QString getLoggingLevel() const;
    QString getLoggingPath() const;
    bool isAutoRetryEnabled() const;
    int getRetryAttempts() const;
    int getRetryDelayMs() const;

    // 通用 get 方法，支持点号路径（如 "ollama.url"）
    QVariant get(const QString &key, const QVariant &defaultValue = QVariant()) const;

    // 配置设置方法
    void setOllamaUrl(const QString &url);
    void setOllamaModel(const QString &modelName);
    void setOllamaTimeout(int seconds);
    void setWindowGeometry(const QRect &geometry);
    void setWindowState(const QString &state);
    void setTheme(const QString &theme);
    void setLoggingLevel(const QString &level);
    void setAutoRetry(bool enabled);

    // 通用 set 方法
    void set(const QString &key, const QVariant &value);

    // 持久化方法
    bool load();
    bool save() const;
    void resetToDefaults();

    // 工具方法
    QString getConfigDir() const;
    QString getConfigFilePath() const;
    bool validateConfig() const;
    QString getLastError() const;

signals:
    // 配置变更时发射
    void configChanged(const QString &key);

private:
    explicit ConfigManager(QObject *parent = nullptr);
    ~ConfigManager() = default;

    // 初始化默认配置
    void initializeDefaults();

    // 展开路径中的 ~ 符号
    QString expandPath(const QString &path) const;

    // 创建目录（如不存在）
    bool ensureDirectoryExists(const QString &dirPath) const;

    // 从 JSON 对象中按路径获取值
    QVariant getValueFromPath(const QJsonObject &obj, const QString &path, const QVariant &defaultValue) const;

    // 在 JSON 对象中按路径设置值
    void setValueAtPath(QJsonObject &obj, const QString &path, const QVariant &value);

    QJsonObject configData;
    mutable QString lastError;
    static QMutex mutex;
};

#endif // CONFIGMANAGER_H
