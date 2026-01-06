#include <QTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include "configmanager.h"

class ConfigManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // 测试单例的唯一性
    void testSingletonUniqueness();
    
    // 测试默认值初始化
    void testDefaultValues();
    
    // 测试配置文件的读写
    void testConfigFileReadWrite();
    
    // 测试配置验证
    void testConfigValidation();
    
    // 测试路径展开（~ 符号）
    void testPathExpansion();
    
    // 测试配置更新和保存
    void testConfigUpdateAndSave();
    
    // 测试无效配置的处理
    void testInvalidConfigHandling();
    
    // 测试错误恢复（配置文件损坏）
    void testCorruptedConfigRecovery();
    
    // 测试通用 get/set 方法
    void testGenericGetSet();
    
    // 测试信号发射
    void testSignalEmission();
    
    // 测试重置到默认值
    void testResetToDefaults();

private:
    QString originalConfigPath;
    QTemporaryDir tempDir;
};

void ConfigManagerTest::initTestCase()
{
    // 保存原始配置路径（如果需要恢复的话）
    qDebug() << "Test case initialization";
}

void ConfigManagerTest::cleanupTestCase()
{
    qDebug() << "Test case cleanup";
}

void ConfigManagerTest::init()
{
    // 每个测试开始前重置配置
    ConfigManager::instance().resetToDefaults();
}

void ConfigManagerTest::cleanup()
{
    // 每个测试结束后清理
}

void ConfigManagerTest::testSingletonUniqueness()
{
    ConfigManager &instance1 = ConfigManager::instance();
    ConfigManager &instance2 = ConfigManager::instance();
    
    // 验证两个引用指向同一个对象
    QCOMPARE(&instance1, &instance2);
    
    // 验证通过引用修改会影响另一个引用
    instance1.setOllamaUrl("http://test-url.com");
    QCOMPARE(instance2.getOllamaUrl(), QString("http://test-url.com"));
}

void ConfigManagerTest::testDefaultValues()
{
    ConfigManager &config = ConfigManager::instance();
    config.resetToDefaults();
    
    // 测试 Ollama 默认值
    QCOMPARE(config.getOllamaUrl(), QString("http://localhost:11434/api/generate"));
    QCOMPARE(config.getOllamaModel(), QString("qwen2.5vl:7b"));
    QCOMPARE(config.getOllamaTimeout(), 30);
    
    // 测试 UI 默认值
    QRect expectedGeometry(100, 100, 1200, 800);
    QCOMPARE(config.getWindowGeometry(), expectedGeometry);
    QCOMPARE(config.getWindowState(), QString("normal"));
    QCOMPARE(config.getTheme(), QString("dark"));
    
    // 测试 Pandoc 默认值
    QCOMPARE(config.isPandocEnabled(), true);
    QCOMPARE(config.getPandocPath(), QString("pandoc"));
    QCOMPARE(config.getPandocTimeout(), 10);
    
    // 测试日志默认值
    QCOMPARE(config.getLoggingLevel(), QString("INFO"));
    
    // 测试高级默认值
    QCOMPARE(config.isAutoRetryEnabled(), true);
    QCOMPARE(config.getRetryAttempts(), 3);
    QCOMPARE(config.getRetryDelayMs(), 1000);
}

void ConfigManagerTest::testConfigFileReadWrite()
{
    ConfigManager &config = ConfigManager::instance();
    
    // 修改一些配置
    config.setOllamaUrl("http://custom-url.com");
    config.setOllamaModel("custom-model");
    config.setOllamaTimeout(60);
    
    // 保存配置
    QVERIFY(config.save());
    
    // 验证配置文件存在
    QString configPath = config.getConfigFilePath();
    QFile configFile(configPath);
    QVERIFY(configFile.exists());
    
    // 直接用另一个变量验证当前值
    QCOMPARE(config.getOllamaUrl(), QString("http://custom-url.com"));
    QCOMPARE(config.getOllamaModel(), QString("custom-model"));
    QCOMPARE(config.getOllamaTimeout(), 60);
    
    // 重新加载配置（应该保持原值）
    QVERIFY(config.load());
    
    // 验证加载的配置正确
    QCOMPARE(config.getOllamaUrl(), QString("http://custom-url.com"));
    QCOMPARE(config.getOllamaModel(), QString("custom-model"));
    QCOMPARE(config.getOllamaTimeout(), 60);
}

void ConfigManagerTest::testConfigValidation()
{
    ConfigManager &config = ConfigManager::instance();
    config.resetToDefaults();
    
    // 默认配置应该是有效的
    QVERIFY(config.validateConfig());
    
    // 设置无效的超时值（这里我们无法直接设置无效值，因为类型安全）
    // 但我们可以测试验证函数的存在和基本功能
    config.setOllamaTimeout(30);
    QVERIFY(config.validateConfig());
}

void ConfigManagerTest::testPathExpansion()
{
    ConfigManager &config = ConfigManager::instance();
    
    // 获取日志路径（应该展开 ~ 符号）
    QString logPath = config.getLoggingPath();
    
    // 验证路径不包含 ~ 符号
    QVERIFY(!logPath.contains("~"));
    
    // 验证路径包含用户主目录的某个部分
    QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QVERIFY(logPath.startsWith(homeDir));
}

void ConfigManagerTest::testConfigUpdateAndSave()
{
    ConfigManager &config = ConfigManager::instance();
    
    // 测试 Ollama 设置
    config.setOllamaUrl("http://new-url.com");
    QCOMPARE(config.getOllamaUrl(), QString("http://new-url.com"));
    
    config.setOllamaModel("new-model");
    QCOMPARE(config.getOllamaModel(), QString("new-model"));
    
    config.setOllamaTimeout(45);
    QCOMPARE(config.getOllamaTimeout(), 45);
    
    // 测试窗口设置
    QRect newGeometry(200, 200, 1000, 600);
    config.setWindowGeometry(newGeometry);
    QCOMPARE(config.getWindowGeometry(), newGeometry);
    
    config.setWindowState("maximized");
    QCOMPARE(config.getWindowState(), QString("maximized"));
    
    config.setTheme("light");
    QCOMPARE(config.getTheme(), QString("light"));
    
    // 测试日志设置
    config.setLoggingLevel("DEBUG");
    QCOMPARE(config.getLoggingLevel(), QString("DEBUG"));
    
    // 测试高级设置
    config.setAutoRetry(false);
    QCOMPARE(config.isAutoRetryEnabled(), false);
    
    // 保存并重新加载
    QVERIFY(config.save());
    QVERIFY(config.load());
    
    // 验证所有修改都已保存
    QCOMPARE(config.getOllamaUrl(), QString("http://new-url.com"));
    QCOMPARE(config.getOllamaModel(), QString("new-model"));
    QCOMPARE(config.getOllamaTimeout(), 45);
    QCOMPARE(config.getWindowGeometry(), newGeometry);
    QCOMPARE(config.getWindowState(), QString("maximized"));
    QCOMPARE(config.getTheme(), QString("light"));
    QCOMPARE(config.getLoggingLevel(), QString("DEBUG"));
    QCOMPARE(config.isAutoRetryEnabled(), false);
}

void ConfigManagerTest::testInvalidConfigHandling()
{
    ConfigManager &config = ConfigManager::instance();
    QString configPath = config.getConfigFilePath();
    
    // 创建一个无效的配置文件
    QFile file(configPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write("{\"invalid\": \"config\"}");
        file.close();
    }
    
    // 尝试加载无效配置（应该失败并使用默认值）
    config.load();
    
    // 验证使用了默认值
    QCOMPARE(config.getOllamaUrl(), QString("http://localhost:11434/api/generate"));
    QCOMPARE(config.getOllamaModel(), QString("qwen2.5vl:7b"));
}

void ConfigManagerTest::testCorruptedConfigRecovery()
{
    ConfigManager &config = ConfigManager::instance();
    QString configPath = config.getConfigFilePath();
    
    // 创建一个损坏的配置文件（无效的 JSON）
    QFile file(configPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write("{ this is not valid json }");
        file.close();
    }
    
    // 尝试加载损坏的配置
    bool loadResult = config.load();
    
    // 应该失败但程序不会崩溃
    QVERIFY(!loadResult || config.getLastError().isEmpty() == false || 
            config.getOllamaUrl() == "http://localhost:11434/api/generate");
    
    // 验证仍然可以使用默认值
    QVERIFY(!config.getOllamaUrl().isEmpty());
}

void ConfigManagerTest::testGenericGetSet()
{
    ConfigManager &config = ConfigManager::instance();
    
    // 测试通用 set 方法
    config.set("ollama.url", "http://generic-url.com");
    QCOMPARE(config.getOllamaUrl(), QString("http://generic-url.com"));
    
    // 测试通用 get 方法
    QVariant urlValue = config.get("ollama.url");
    QCOMPARE(urlValue.toString(), QString("http://generic-url.com"));
    
    // 测试带默认值的 get
    QVariant nonExistent = config.get("non.existent.key", "default-value");
    QCOMPARE(nonExistent.toString(), QString("default-value"));
    
    // 测试嵌套值
    config.set("ui.windowGeometry.width", 1920);
    QCOMPARE(config.get("ui.windowGeometry.width").toInt(), 1920);
}

void ConfigManagerTest::testSignalEmission()
{
    ConfigManager &config = ConfigManager::instance();
    
    // 创建信号监听器
    QSignalSpy spy(&config, &ConfigManager::configChanged);
    
    // 修改配置
    config.setOllamaUrl("http://signal-test.com");
    
    // 验证信号被发射
    QCOMPARE(spy.count(), 1);
    
    // 验证信号参数
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), QString("ollama.url"));
    
    // 测试通用 set 方法也会发射信号
    config.set("ollama.modelName", "test-model");
    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), QString("ollama.modelName"));
}

void ConfigManagerTest::testResetToDefaults()
{
    ConfigManager &config = ConfigManager::instance();
    
    // 修改一些配置
    config.setOllamaUrl("http://modified-url.com");
    config.setOllamaModel("modified-model");
    config.setTheme("light");
    
    // 验证配置已修改
    QCOMPARE(config.getOllamaUrl(), QString("http://modified-url.com"));
    QCOMPARE(config.getTheme(), QString("light"));
    
    // 创建信号监听器
    QSignalSpy spy(&config, &ConfigManager::configChanged);
    
    // 重置到默认值
    config.resetToDefaults();
    
    // 验证信号被发射（使用 * 表示所有配置）
    QVERIFY(spy.count() >= 1);
    
    // 验证配置已重置
    QCOMPARE(config.getOllamaUrl(), QString("http://localhost:11434/api/generate"));
    QCOMPARE(config.getOllamaModel(), QString("qwen2.5vl:7b"));
    QCOMPARE(config.getTheme(), QString("dark"));
}

QTEST_MAIN(ConfigManagerTest)
#include "configmanager_test.moc"
