# 设置对话框 (SettingsDialog) 使用说明

## 概述

SettingsDialog 是一个完整的设置管理界面，允许用户图形化地查看和修改应用配置，与 ConfigManager 完全集成。

## 功能特性

### 1. Ollama 设置标签页

**配置项：**
- **API URL**: Ollama 服务的 API 端点地址
  - 默认值: `http://localhost:11434/api/generate`
  - 验证: 必须以 `http://` 或 `https://` 开头
  - 实时提示: 确保 Ollama 服务正在运行

- **模型名称**: 使用的 Ollama 模型
  - 默认值: `qwen2.5vl:7b`
  - 支持: 任何 Ollama 支持的多模态模型（如 llava）

- **超时时间**: API 请求超时设置
  - 范围: 5-120 秒
  - 默认值: 30 秒

### 2. UI 设置标签页

**配置项：**
- **主题选择**: 应用外观主题
  - 选项: 暗色主题 (Dark) / 亮色主题 (Light)
  - 默认: 暗色主题
  - 注: 主题更改后需要重启应用生效

- **窗口状态信息**: 只读显示
  - 说明: 应用会自动保存和恢复窗口位置、大小和状态

### 3. Pandoc 设置标签页

**配置项：**
- **启用 Pandoc 转换功能**: 总开关
  - 控制是否使用 Pandoc 进行文档转换
  - 禁用后，相关功能将不可用

- **可执行文件路径**: Pandoc 程序位置
  - 默认值: `pandoc`（系统 PATH 中查找）
  - 支持: 绝对路径或命令名称
  - 浏览按钮: 选择本地 Pandoc 可执行文件
  - 状态指示器:
    - ✓ 可用 (绿色): Pandoc 路径有效且可执行
    - ✗ 不可用 (红色): Pandoc 路径无效或不可执行
    - 未设置 (灰色): 路径为空

- **超时时间**: Pandoc 转换超时设置
  - 范围: 1-30 秒
  - 默认值: 10 秒

### 4. 日志设置标签页

**配置项：**
- **日志级别**: 日志输出详细程度
  - 选项: DEBUG / INFO / WARN / ERROR
  - 默认值: INFO
  - 说明:
    - DEBUG: 最详细，包含调试信息
    - INFO: 一般信息
    - WARN: 警告信息
    - ERROR: 仅错误信息

- **日志路径**: 日志文件存储位置
  - 只读显示
  - 默认: `~/.config/FormulaRecognizer/logs/`

- **打开日志文件夹**: 按钮
  - 功能: 在文件管理器中打开日志目录
  - 自动创建目录（如果不存在）

- **日志轮转信息**: 只读显示
  - 最大文件大小: 10MB
  - 保留备份文件数: 5

### 5. 高级设置标签页

**配置项：**
- **启用自动重试**: 网络请求失败自动重试
  - 默认: 启用
  - 说明: 避免因临时网络问题导致识别失败

- **重试次数**: 失败后重试的次数
  - 范围: 1-10 次
  - 默认值: 3 次

- **重试延迟**: 每次重试之间的延迟时间
  - 范围: 100-5000 毫秒
  - 默认值: 1000 毫秒（1 秒）
  - 步进: 100 毫秒

## 对话框操作

### 按钮功能

- **应用 (Apply)**: 
  - 保存所有修改到配置文件
  - 对话框保持打开状态
  - 验证所有输入的有效性

- **重置 (Reset)**:
  - 恢复到上次保存的设置
  - 放弃所有未保存的修改
  - 需要确认操作

- **确定 (OK)**:
  - 保存所有修改并关闭对话框
  - 相当于 "应用 + 关闭"

- **取消 (Cancel)**:
  - 放弃所有未保存的修改
  - 关闭对话框
  - 如有未保存修改，会提示确认

### 快捷键

- **Ctrl+,**: 从主窗口打开设置对话框
- **Ctrl+Q**: 退出应用

## 访问设置对话框

从主窗口菜单栏：
1. 点击 "文件(F)" 菜单
2. 选择 "设置(S)..." 菜单项
3. 或使用快捷键 Ctrl+,

## 输入验证

对话框会在保存前验证所有输入：

### Ollama 设置验证
- URL 不能为空
- URL 必须以 `http://` 或 `https://` 开头
- 模型名称不能为空

### Pandoc 设置验证
- 如果启用 Pandoc，路径不能为空
- 如果路径无效，会提示是否仍要保存

### 所有超时值验证
- 必须在指定范围内
- 自动限制在最小/最大值之间

## 配置持久化

- 所有设置通过 ConfigManager 保存到 JSON 文件
- 配置文件位置: `~/.config/FormulaRecognizer/config.json`
- 保存失败时会显示错误信息
- ConfigManager 会发射 `configChanged` 信号通知其他组件

## 与主窗口的集成

### 配置更新流程

1. 用户在设置对话框中修改配置
2. 点击 "应用" 或 "确定" 保存
3. ConfigManager 保存配置到文件
4. ConfigManager 发射 `configChanged` 信号
5. 主窗口的 `onConfigChanged` 槽函数被调用
6. 主窗口更新相关组件（如 OllamaClient）

### 自动同步

- Ollama URL 和模型名称会自动同步到主窗口显示
- 主题更改需要重启应用生效
- 窗口几何状态在应用退出时自动保存

## 样式和外观

- 继承主窗口的暗色主题样式 (style.qss)
- 标签页组织清晰，易于导航
- 响应式布局，支持调整窗口大小
- 状态指示器使用颜色编码
  - 绿色 (#2ecc71): 成功/可用
  - 红色 (#e74c3c): 错误/不可用
  - 灰色: 未设置/中性

## 代码结构

### 文件组成

- **settingsdialog.h**: 头文件，类定义
- **settingsdialog.cpp**: 实现文件，逻辑代码
- **settingsdialog.ui**: Qt Designer UI 文件
- **ui_settingsdialog.h**: 自动生成的 UI 头文件

### 类成员

**临时存储变量：**
```cpp
QString tempOllamaUrl;
QString tempModelName;
int tempOllamaTimeout;
// ... 其他临时变量
```

这些变量存储用户在对话框中的修改，只有在点击 "应用" 或 "确定" 时才会保存到 ConfigManager。

**核心方法：**
- `initializeUI()`: 连接所有信号和槽
- `loadSettings()`: 从 ConfigManager 加载配置到 UI
- `applySettings()`: 从 UI 保存配置到 ConfigManager
- `validateSettings()`: 验证所有输入的有效性

### 信号槽连接

每个 UI 控件的变化都连接到对应的槽函数：
- 文本框: `textChanged` → `onXxxChanged`
- 复选框: `toggled` → `onXxxToggled`
- 下拉框: `currentIndexChanged` → `onXxxChanged`
- 数字框: `valueChanged` → `onXxxChanged`

## 扩展和自定义

### 添加新配置项

1. 在 ConfigManager 中添加新的 getter/setter
2. 在 settingsdialog.ui 中添加新的 UI 控件
3. 在 settingsdialog.h 中添加临时存储变量
4. 在 settingsdialog.cpp 中：
   - 在 `initializeUI()` 中连接信号槽
   - 在 `loadSettings()` 中加载配置
   - 在 `applySettings()` 中保存配置
   - 在 `validateSettings()` 中验证（如需要）

### 添加新标签页

1. 在 settingsdialog.ui 中使用 Qt Designer 添加新标签页
2. 添加相应的控件和布局
3. 按照上述步骤添加配置项的加载和保存逻辑

## 常见问题

### Q: 修改后没有生效？
A: 确保点击了 "应用" 或 "确定" 按钮。某些设置（如主题）可能需要重启应用。

### Q: Pandoc 状态显示不可用？
A: 检查 Pandoc 是否已正确安装，路径是否正确。使用 "浏览" 按钮选择正确的 Pandoc 可执行文件。

### Q: 配置文件在哪里？
A: `~/.config/FormulaRecognizer/config.json`

### Q: 如何恢复默认设置？
A: 删除配置文件并重启应用，或在 ConfigManager 中调用 `resetToDefaults()`。

## 开发注意事项

- 所有配置访问必须通过 ConfigManager 进行
- 使用临时变量存储未保存的修改
- 验证用户输入以防止无效配置
- 提供清晰的错误提示
- 保持代码注释为中文，符合项目风格

## 测试建议

1. 测试所有输入验证逻辑
2. 测试 "应用"、"重置"、"确定"、"取消" 按钮
3. 测试 Pandoc 路径验证和状态显示
4. 测试 "打开日志文件夹" 功能
5. 测试配置持久化和加载
6. 测试与主窗口的配置同步

## 版本历史

- v1.0.0 (2024-01): 初始版本
  - 5 个标签页完整实现
  - 与 ConfigManager 完全集成
  - 输入验证和错误处理
  - 暗色主题样式支持
