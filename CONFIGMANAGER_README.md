# ConfigManager 使用文档

## 概述

ConfigManager 是 FormulaRecognizer 的配置管理系统，提供线程安全的单例模式配置管理，支持配置的读取、写入、校验和默认值处理。

## 核心特性

- ✅ 单例模式，线程安全
- ✅ JSON 格式配置文件
- ✅ 自动配置验证和错误恢复
- ✅ 支持路径展开（~ 符号）
- ✅ 配置变更信号通知
- ✅ 完整的单元测试覆盖

## 配置文件位置

配置文件存储在：`~/.config/FormulaRecognizer/config.json`

## 配置结构

```json
{
  "version": "2.0",
  "ollama": {
    "url": "http://localhost:11434/api/generate",
    "modelName": "qwen2.5vl:7b",
    "timeout": 30
  },
  "ui": {
    "windowGeometry": {
      "x": 100,
      "y": 100,
      "width": 1200,
      "height": 800
    },
    "windowState": "normal",
    "theme": "dark"
  },
  "pandoc": {
    "enabled": true,
    "executablePath": "pandoc",
    "timeout": 10
  },
  "logging": {
    "level": "INFO",
    "filePath": "~/.config/FormulaRecognizer/logs/",
    "maxFileSizeMB": 10,
    "maxBackupFiles": 5
  },
  "advanced": {
    "autoRetry": true,
    "retryAttempts": 3,
    "retryDelayMs": 1000
  }
}
```

## 基本使用

### 1. 获取配置管理器实例

```cpp
#include "configmanager.h"

// 获取单例实例
ConfigManager &config = ConfigManager::instance();
```

### 2. 读取配置

```cpp
// 读取 Ollama 配置
QString ollamaUrl = config.getOllamaUrl();
QString modelName = config.getOllamaModel();
int timeout = config.getOllamaTimeout();

// 读取窗口配置
QRect windowGeometry = config.getWindowGeometry();
QString windowState = config.getWindowState();
QString theme = config.getTheme();

// 读取 Pandoc 配置
bool pandocEnabled = config.isPandocEnabled();
QString pandocPath = config.getPandocPath();

// 读取日志配置
QString logLevel = config.getLoggingLevel();
QString logPath = config.getLoggingPath();  // 自动展开 ~ 符号

// 读取高级配置
bool autoRetry = config.isAutoRetryEnabled();
int retryAttempts = config.getRetryAttempts();
int retryDelay = config.getRetryDelayMs();
```

### 3. 修改配置

```cpp
// 修改 Ollama 配置
config.setOllamaUrl("http://custom-server:11434/api/generate");
config.setOllamaModel("qwen2.5vl:14b");
config.setOllamaTimeout(60);

// 修改窗口配置
QRect newGeometry(200, 200, 1600, 900);
config.setWindowGeometry(newGeometry);
config.setWindowState("maximized");
config.setTheme("light");

// 修改日志配置
config.setLoggingLevel("DEBUG");

// 修改高级配置
config.setAutoRetry(false);

// 保存配置到文件
config.save();
```

### 4. 通用 get/set 方法

```cpp
// 使用点号路径访问配置
QVariant value = config.get("ollama.url");
QString url = value.toString();

// 设置配置
config.set("ollama.url", "http://new-url.com");
config.set("ui.theme", "light");

// 使用默认值
QVariant value = config.get("non.existent.key", "default-value");
```

### 5. 监听配置变更

```cpp
// 连接配置变更信号
connect(&config, &ConfigManager::configChanged, 
        [](const QString &key) {
    qDebug() << "Config changed:" << key;
});

// 当配置改变时，会自动发射信号
config.setOllamaUrl("http://new-url.com");
// 输出: Config changed: ollama.url
```

### 6. 重置到默认值

```cpp
// 重置所有配置到默认值并保存
config.resetToDefaults();
```

### 7. 配置验证

```cpp
// 验证当前配置的有效性
bool isValid = config.validateConfig();
if (!isValid) {
    QString error = config.getLastError();
    qWarning() << "Config validation failed:" << error;
}
```

## 在现有代码中集成

### 示例：在 MainWindow 中使用

```cpp
// mainwindow.cpp
#include "configmanager.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    // 获取配置管理器
    ConfigManager &config = ConfigManager::instance();
    
    // 从配置恢复窗口状态
    QRect geometry = config.getWindowGeometry();
    setGeometry(geometry);
    
    QString windowState = config.getWindowState();
    if (windowState == "maximized") {
        showMaximized();
    }
    
    // 应用主题
    QString theme = config.getTheme();
    applyTheme(theme);
    
    // 配置 OllamaClient
    QString ollamaUrl = config.getOllamaUrl();
    QString modelName = config.getOllamaModel();
    ollamaClient->updateSettings(ollamaUrl, modelName);
    
    // 监听配置变更
    connect(&config, &ConfigManager::configChanged, 
            this, &MainWindow::onConfigChanged);
}

MainWindow::~MainWindow()
{
    // 保存窗口状态
    ConfigManager &config = ConfigManager::instance();
    config.setWindowGeometry(geometry());
    
    if (isMaximized()) {
        config.setWindowState("maximized");
    } else if (isMinimized()) {
        config.setWindowState("minimized");
    } else {
        config.setWindowState("normal");
    }
    
    config.save();
    delete ui;
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
    } else if (key.startsWith("ui.theme")) {
        // 主题变更
        applyTheme(config.getTheme());
    }
}
```

## 错误处理

ConfigManager 具有强大的错误恢复能力：

1. **配置文件不存在**：自动创建并使用默认值
2. **配置文件损坏**：使用默认值并记录错误
3. **配置验证失败**：回退到默认值并保存
4. **写入权限错误**：记录错误信息，可通过 `getLastError()` 获取

```cpp
if (!config.save()) {
    QString error = config.getLastError();
    QMessageBox::warning(this, "保存失败", error);
}
```

## 测试

运行单元测试：

```bash
cd /home/engine/project
qmake ConfigManagerTest.pro
make
./ConfigManagerTest
```

测试覆盖：
- 单例唯一性测试
- 默认值初始化测试
- 配置文件读写测试
- 配置验证测试
- 路径展开测试
- 配置更新和保存测试
- 无效配置处理测试
- 损坏配置恢复测试
- 通用 get/set 测试
- 信号发射测试
- 重置默认值测试

## 线程安全

ConfigManager 使用 QMutex 保证单例实例化的线程安全。但是，配置的读写操作本身不是线程安全的。如果需要在多线程环境中使用，请确保：

1. 只在主线程中修改配置
2. 或者使用适当的互斥锁保护配置访问

## 未来扩展

ConfigManager 设计为易于扩展。添加新配置项的步骤：

1. 在 `initializeDefaults()` 中添加默认值
2. 在 `validateConfig()` 中添加验证逻辑
3. 添加相应的 getter/setter 方法
4. 更新测试用例

示例：

```cpp
// configmanager.h
QString getNewFeature() const;
void setNewFeature(const QString &value);

// configmanager.cpp
void ConfigManager::initializeDefaults()
{
    // ... 现有代码 ...
    
    QJsonObject newFeature;
    newFeature["enabled"] = true;
    newFeature["value"] = "default";
    defaults["newFeature"] = newFeature;
}

QString ConfigManager::getNewFeature() const
{
    return get("newFeature.value", "default").toString();
}

void ConfigManager::setNewFeature(const QString &value)
{
    set("newFeature.value", value);
}
```

## 最佳实践

1. **初始化时加载配置**：在应用启动时调用 `ConfigManager::instance()` 初始化配置
2. **退出时保存配置**：在应用退出前调用 `save()` 保存配置
3. **使用类型安全的方法**：优先使用专用的 getter/setter 方法而不是通用的 get/set
4. **监听配置变更**：使用 `configChanged` 信号实时响应配置变化
5. **定期验证配置**：在关键操作前调用 `validateConfig()` 确保配置有效
6. **错误处理**：总是检查 `save()` 和 `load()` 的返回值

## 许可证

与 FormulaRecognizer 项目相同。
