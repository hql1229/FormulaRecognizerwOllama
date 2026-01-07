void MainWindow::createMenuBar()
{
    // 创建菜单栏
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);
    
    // 确保菜单栏可见
    menuBar->setVisible(true);
    menuBar->show();
    
    // 在某些平台上，可能需要显式设置菜单栏为非原生
    menuBar->setNativeMenuBar(false);
    
    // 确保菜单栏在窗口顶部
    menuBar->raise();
    
    // 确保菜单栏有足够的高度
    menuBar->setMinimumHeight(24);
    menuBar->setMaximumHeight(32);
    
    // 确保菜单栏背景不透明
    menuBar->setAutoFillBackground(true);

    // 创建"文件"菜单
    QMenu *fileMenu = menuBar->addMenu("文件(&F)");
    
    // 添加设置菜单项
    QAction *settingsAction = new QAction("设置(&S)...", this);
    settingsAction->setShortcut(QKeySequence("Ctrl+,"));
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onSettingsTriggered);
    fileMenu->addAction(settingsAction);
    
    fileMenu->addSeparator();
    
    // 添加退出菜单项
    QAction *exitAction = new QAction("退出(&Q)", this);
    exitAction->setShortcut(QKeySequence("Ctrl+Q"));
    connect(exitAction, &QAction::triggered, this, &MainWindow::close);
    fileMenu->addAction(exitAction);
    
    // 创建"帮助"菜单
    QMenu *helpMenu = menuBar->addMenu("帮助(&H)");
    
    // 添加关于菜单项
    QAction *aboutAction = new QAction("关于(&A)...", this);
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "关于公式识别器",
            "<h3>公式识别器</h3>"
            "<p>使用 Ollama 多模态模型识别图像中的数学公式</p>"
            "<p>版本: 1.0.0</p>"
            "<p>支持的模型: qwen2.5vl, llava 等</p>");
    });
    helpMenu->addAction(aboutAction);
}