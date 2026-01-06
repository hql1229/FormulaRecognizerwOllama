#include "configmanager.h"
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QMutexLocker>
#include <QDebug>
#include <QSaveFile>
#include <functional>

QMutex ConfigManager::mutex;

ConfigManager& ConfigManager::instance()
{
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
{
    initializeDefaults();
    load();
}

void ConfigManager::initializeDefaults()
{
    QJsonObject defaults;

    defaults["version"] = "2.0";

    QJsonObject ollama;
    ollama["url"] = "http://localhost:11434/api/generate";
    ollama["modelName"] = "qwen2.5vl:7b";
    ollama["timeout"] = 30;
    defaults["ollama"] = ollama;

    QJsonObject ui;
    QJsonObject windowGeometry;
    windowGeometry["x"] = 100;
    windowGeometry["y"] = 100;
    windowGeometry["width"] = 1200;
    windowGeometry["height"] = 800;
    ui["windowGeometry"] = windowGeometry;
    ui["windowState"] = "normal";
    ui["theme"] = "dark";
    defaults["ui"] = ui;

    QJsonObject pandoc;
    pandoc["enabled"] = true;
    pandoc["executablePath"] = "pandoc";
    pandoc["timeout"] = 10;
    defaults["pandoc"] = pandoc;

    QJsonObject logging;
    logging["level"] = "INFO";
    logging["filePath"] = "~/.config/FormulaRecognizer/logs/";
    logging["maxFileSizeMB"] = 10;
    logging["maxBackupFiles"] = 5;
    defaults["logging"] = logging;

    QJsonObject advanced;
    advanced["autoRetry"] = true;
    advanced["retryAttempts"] = 3;
    advanced["retryDelayMs"] = 1000;
    defaults["advanced"] = advanced;

    configData = defaults;
}

bool ConfigManager::load()
{
    QString configPath = getConfigFilePath();
    QFile file(configPath);

    if (!file.exists()) {
        qDebug() << "Config file does not exist, using defaults:" << configPath;
        // 首次创建时保存默认配置
        ensureDirectoryExists(getConfigDir());
        save();
        return true;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        lastError = QString("Failed to open config file: %1").arg(file.errorString());
        qWarning() << lastError;
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        lastError = QString("Failed to parse config JSON: %1").arg(parseError.errorString());
        qWarning() << lastError;
        return false;
    }

    if (!doc.isObject()) {
        lastError = "Config file root is not a JSON object";
        qWarning() << lastError;
        return false;
    }

    configData = doc.object();

    if (!validateConfig()) {
        qWarning() << "Config validation failed, using defaults";
        initializeDefaults();
        save();
        return false;
    }

    qDebug() << "Config loaded successfully from:" << configPath;
    return true;
}

bool ConfigManager::save() const
{
    QString configPath = getConfigFilePath();
    QString configDir = getConfigDir();

    if (!ensureDirectoryExists(configDir)) {
        lastError = QString("Failed to create config directory: %1").arg(configDir);
        qWarning() << lastError;
        return false;
    }

    // 使用 QSaveFile 实现原子写入
    QSaveFile file(configPath);
    if (!file.open(QIODevice::WriteOnly)) {
        lastError = QString("Failed to open config file for writing: %1").arg(file.errorString());
        qWarning() << lastError;
        return false;
    }

    QJsonDocument doc(configData);
    QByteArray jsonData = doc.toJson(QJsonDocument::Indented);

    if (file.write(jsonData) == -1) {
        lastError = QString("Failed to write config data: %1").arg(file.errorString());
        qWarning() << lastError;
        return false;
    }

    if (!file.commit()) {
        lastError = QString("Failed to commit config file: %1").arg(file.errorString());
        qWarning() << lastError;
        return false;
    }

    qDebug() << "Config saved successfully to:" << configPath;
    return true;
}

void ConfigManager::resetToDefaults()
{
    qDebug() << "Resetting configuration to defaults";
    initializeDefaults();
    save();
    emit configChanged("*");
}

bool ConfigManager::validateConfig() const
{
    // 检查版本号
    if (!configData.contains("version")) {
        qWarning() << "Config missing 'version' field";
        return false;
    }

    // 检查必需的顶级键
    QStringList requiredKeys = {"ollama", "ui", "pandoc", "logging", "advanced"};
    for (const QString &key : requiredKeys) {
        if (!configData.contains(key)) {
            qWarning() << "Config missing required key:" << key;
            return false;
        }
        if (!configData[key].isObject()) {
            qWarning() << "Config key is not an object:" << key;
            return false;
        }
    }

    // 验证 Ollama 配置
    QJsonObject ollama = configData["ollama"].toObject();
    if (!ollama.contains("url") || !ollama["url"].isString()) {
        qWarning() << "Invalid ollama.url";
        return false;
    }
    if (!ollama.contains("modelName") || !ollama["modelName"].isString()) {
        qWarning() << "Invalid ollama.modelName";
        return false;
    }
    if (!ollama.contains("timeout") || !ollama["timeout"].isDouble()) {
        qWarning() << "Invalid ollama.timeout";
        return false;
    }
    if (ollama["timeout"].toInt() <= 0) {
        qWarning() << "ollama.timeout must be > 0";
        return false;
    }

    // 验证 UI 配置
    QJsonObject ui = configData["ui"].toObject();
    if (!ui.contains("windowGeometry") || !ui["windowGeometry"].isObject()) {
        qWarning() << "Invalid ui.windowGeometry";
        return false;
    }
    QJsonObject windowGeometry = ui["windowGeometry"].toObject();
    QStringList geometryKeys = {"x", "y", "width", "height"};
    for (const QString &key : geometryKeys) {
        if (!windowGeometry.contains(key) || !windowGeometry[key].isDouble()) {
            qWarning() << "Invalid ui.windowGeometry." << key;
            return false;
        }
    }
    if (!ui.contains("windowState") || !ui["windowState"].isString()) {
        qWarning() << "Invalid ui.windowState";
        return false;
    }
    if (!ui.contains("theme") || !ui["theme"].isString()) {
        qWarning() << "Invalid ui.theme";
        return false;
    }

    // 验证 Pandoc 配置
    QJsonObject pandoc = configData["pandoc"].toObject();
    if (!pandoc.contains("enabled") || !pandoc["enabled"].isBool()) {
        qWarning() << "Invalid pandoc.enabled";
        return false;
    }
    if (!pandoc.contains("executablePath") || !pandoc["executablePath"].isString()) {
        qWarning() << "Invalid pandoc.executablePath";
        return false;
    }
    if (!pandoc.contains("timeout") || !pandoc["timeout"].isDouble()) {
        qWarning() << "Invalid pandoc.timeout";
        return false;
    }
    if (pandoc["timeout"].toInt() <= 0) {
        qWarning() << "pandoc.timeout must be > 0";
        return false;
    }

    // 验证日志配置
    QJsonObject logging = configData["logging"].toObject();
    if (!logging.contains("level") || !logging["level"].isString()) {
        qWarning() << "Invalid logging.level";
        return false;
    }
    QStringList validLevels = {"DEBUG", "INFO", "WARN", "ERROR"};
    if (!validLevels.contains(logging["level"].toString())) {
        qWarning() << "Invalid logging.level value:" << logging["level"].toString();
        return false;
    }
    if (!logging.contains("filePath") || !logging["filePath"].isString()) {
        qWarning() << "Invalid logging.filePath";
        return false;
    }

    // 验证高级配置
    QJsonObject advanced = configData["advanced"].toObject();
    if (!advanced.contains("autoRetry") || !advanced["autoRetry"].isBool()) {
        qWarning() << "Invalid advanced.autoRetry";
        return false;
    }
    if (!advanced.contains("retryAttempts") || !advanced["retryAttempts"].isDouble()) {
        qWarning() << "Invalid advanced.retryAttempts";
        return false;
    }
    if (advanced["retryAttempts"].toInt() < 0) {
        qWarning() << "advanced.retryAttempts must be >= 0";
        return false;
    }
    if (!advanced.contains("retryDelayMs") || !advanced["retryDelayMs"].isDouble()) {
        qWarning() << "Invalid advanced.retryDelayMs";
        return false;
    }
    if (advanced["retryDelayMs"].toInt() < 0) {
        qWarning() << "advanced.retryDelayMs must be >= 0";
        return false;
    }

    return true;
}

QString ConfigManager::getConfigDir() const
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    return QDir(configDir).filePath("FormulaRecognizer");
}

QString ConfigManager::getConfigFilePath() const
{
    return QDir(getConfigDir()).filePath("config.json");
}

QString ConfigManager::getLastError() const
{
    return lastError;
}

QString ConfigManager::expandPath(const QString &path) const
{
    if (path.startsWith("~")) {
        QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        return path.mid(1).prepend(homePath);
    }
    return path;
}

bool ConfigManager::ensureDirectoryExists(const QString &dirPath) const
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning() << "Failed to create directory:" << dirPath;
            return false;
        }
        qDebug() << "Created directory:" << dirPath;
    }
    return true;
}

QVariant ConfigManager::getValueFromPath(const QJsonObject &obj, const QString &path, const QVariant &defaultValue) const
{
    QStringList keys = path.split('.');
    QJsonValue value = obj;

    for (const QString &key : keys) {
        if (!value.isObject()) {
            return defaultValue;
        }
        QJsonObject currentObj = value.toObject();
        if (!currentObj.contains(key)) {
            return defaultValue;
        }
        value = currentObj[key];
    }

    return value.toVariant();
}

void ConfigManager::setValueAtPath(QJsonObject &obj, const QString &path, const QVariant &value)
{
    QStringList keys = path.split('.');
    if (keys.isEmpty()) {
        return;
    }

    QJsonObject *currentObj = &obj;
    for (int i = 0; i < keys.size() - 1; ++i) {
        const QString &key = keys[i];
        if (!currentObj->contains(key) || !(*currentObj)[key].isObject()) {
            currentObj->insert(key, QJsonObject());
        }
        QJsonValue val = (*currentObj)[key];
        // 需要通过临时对象来修改
        QJsonObject tempObj = val.toObject();
        currentObj->insert(key, tempObj);
        currentObj = &tempObj;
    }

    currentObj->insert(keys.last(), QJsonValue::fromVariant(value));

    // 向上传播修改
    for (int i = keys.size() - 2; i >= 0; --i) {
        QJsonObject parentObj = obj;
        QStringList parentPath = keys.mid(0, i + 1);
        
        QJsonObject *parent = &obj;
        for (int j = 0; j < i; ++j) {
            QJsonValue val = (*parent)[parentPath[j]];
            QJsonObject temp = val.toObject();
            parent = &temp;
        }
        
        parent->insert(parentPath.last(), *currentObj);
    }
}

// Getter 方法实现
QString ConfigManager::getOllamaUrl() const
{
    return get("ollama.url", "http://localhost:11434/api/generate").toString();
}

QString ConfigManager::getOllamaModel() const
{
    return get("ollama.modelName", "qwen2.5vl:7b").toString();
}

int ConfigManager::getOllamaTimeout() const
{
    return get("ollama.timeout", 30).toInt();
}

QRect ConfigManager::getWindowGeometry() const
{
    int x = get("ui.windowGeometry.x", 100).toInt();
    int y = get("ui.windowGeometry.y", 100).toInt();
    int width = get("ui.windowGeometry.width", 1200).toInt();
    int height = get("ui.windowGeometry.height", 800).toInt();
    return QRect(x, y, width, height);
}

QString ConfigManager::getWindowState() const
{
    return get("ui.windowState", "normal").toString();
}

QString ConfigManager::getTheme() const
{
    return get("ui.theme", "dark").toString();
}

bool ConfigManager::isPandocEnabled() const
{
    return get("pandoc.enabled", true).toBool();
}

QString ConfigManager::getPandocPath() const
{
    return get("pandoc.executablePath", "pandoc").toString();
}

int ConfigManager::getPandocTimeout() const
{
    return get("pandoc.timeout", 10).toInt();
}

QString ConfigManager::getLoggingLevel() const
{
    return get("logging.level", "INFO").toString();
}

QString ConfigManager::getLoggingPath() const
{
    QString path = get("logging.filePath", "~/.config/FormulaRecognizer/logs/").toString();
    return expandPath(path);
}

bool ConfigManager::isAutoRetryEnabled() const
{
    return get("advanced.autoRetry", true).toBool();
}

int ConfigManager::getRetryAttempts() const
{
    return get("advanced.retryAttempts", 3).toInt();
}

int ConfigManager::getRetryDelayMs() const
{
    return get("advanced.retryDelayMs", 1000).toInt();
}

QVariant ConfigManager::get(const QString &key, const QVariant &defaultValue) const
{
    return getValueFromPath(configData, key, defaultValue);
}

// Setter 方法实现
void ConfigManager::setOllamaUrl(const QString &url)
{
    set("ollama.url", url);
}

void ConfigManager::setOllamaModel(const QString &modelName)
{
    set("ollama.modelName", modelName);
}

void ConfigManager::setOllamaTimeout(int seconds)
{
    set("ollama.timeout", seconds);
}

void ConfigManager::setWindowGeometry(const QRect &geometry)
{
    set("ui.windowGeometry.x", geometry.x());
    set("ui.windowGeometry.y", geometry.y());
    set("ui.windowGeometry.width", geometry.width());
    set("ui.windowGeometry.height", geometry.height());
    emit configChanged("ui.windowGeometry");
}

void ConfigManager::setWindowState(const QString &state)
{
    set("ui.windowState", state);
}

void ConfigManager::setTheme(const QString &theme)
{
    set("ui.theme", theme);
}

void ConfigManager::setLoggingLevel(const QString &level)
{
    set("logging.level", level);
}

void ConfigManager::setAutoRetry(bool enabled)
{
    set("advanced.autoRetry", enabled);
}

void ConfigManager::set(const QString &key, const QVariant &value)
{
    QStringList keys = key.split('.');
    if (keys.isEmpty()) {
        return;
    }

    // 递归辅助函数：设置嵌套值
    std::function<void(QJsonObject&, const QStringList&, const QVariant&, int)> setNestedValue;
    setNestedValue = [&setNestedValue](QJsonObject &obj, const QStringList &keys, const QVariant &value, int index) {
        if (index >= keys.size()) {
            return;
        }
        
        const QString &key = keys[index];
        
        if (index == keys.size() - 1) {
            // 最后一个键，直接设置值
            obj[key] = QJsonValue::fromVariant(value);
        } else {
            // 中间键，递归处理
            QJsonObject nested;
            if (obj.contains(key) && obj[key].isObject()) {
                nested = obj[key].toObject();
            }
            setNestedValue(nested, keys, value, index + 1);
            obj[key] = nested;
        }
    };
    
    // 使用递归辅助函数设置嵌套值
    QJsonObject result = configData;
    setNestedValue(result, keys, value, 0);
    configData = result;
    
    emit configChanged(key);
}
