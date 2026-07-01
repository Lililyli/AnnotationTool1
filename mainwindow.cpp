#include "mainwindow.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QFileInfo>
#include <QDir>
#include <QShortcut>
#include <QKeySequence>
#include <QTextStream>
#include <QDomDocument>
#include <QListWidgetItem>
#include <QApplication>
#include <QScreen>
#include <QDebug>
#include "trainwidget.h"
#include "dbviewer.h"   // ← 新增

static const QStringList IMAGE_FILTERS = {
    "*.jpg", "*.jpeg", "*.png", "*.bmp", "*.gif", "*.tiff", "*.webp"
};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_currentImageIndex(-1)
{
    setupUI();
    setupMenuBar();
    setupToolBar();
    setupDockWidgets();
    setupStatusBar();
    setupShortcuts();
    updateWindowTitle();

    // 窗口大小 & 居中
    resize(900, 620);
    move(QApplication::primaryScreen()->geometry().center() - rect().center());
}

MainWindow::~MainWindow() {}

// ======================== UI 初始化 ========================

void MainWindow::setupUI()
{
    m_imageView = new ImageView(this);
    m_imageView->setLabelManager(&m_labelManager);

    // 1. 提前实例化所有子页面（用局部变量或成员变量均可）
    TrainWidget* trainWidget       = new TrainWidget(this);
    QuantizeWidget* quantizeWidget = new QuantizeWidget(this);
    InferWidget* inferWidget       = new InferWidget(this);

    // 2. 创建 Tab 容器并添加页面
    QTabWidget* tabWidget = new QTabWidget(this);
    tabWidget->addTab(m_imageView,    "标注");   // 第 0 页
    tabWidget->addTab(trainWidget,    "训练");   // 第 1 页
    tabWidget->addTab(quantizeWidget, "量化");   // 第 2 页
    tabWidget->addTab(inferWidget,    "推理");   // 第 3 页

    setCentralWidget(tabWidget); // 把 Tab 容器设为主界面

    // ==================== 适配你的信号与槽 ====================

    // (1) 量化完成后自动把模型路径传给推理页面
    // 注意：这里需要确保信号带有 QString 类型的路径参数
    connect(quantizeWidget, &QuantizeWidget::quantizeFinished,
            inferWidget,    &InferWidget::setModelPath);

    // (2) 量化完成后自动切换到推理Tab
    // 注意：lambda 表达式的方括号 [=] 表示捕获上下文中的 tabWidget 和 inferWidget 变量
    connect(quantizeWidget, &QuantizeWidget::quantizeFinished,
            this, [=](){
                tabWidget->setCurrentWidget(inferWidget);
            });
    setCentralWidget(tabWidget);          // 把 Tab 容器设为主界面

    connect(m_imageView, &ImageView::annotationChanged,
            this, &MainWindow::onAnnotationChanged);
    connect(m_imageView, &ImageView::annotationSelected,
            this, &MainWindow::onAnnotationSelected);
    connect(m_imageView, &ImageView::statusMessage,
            this, &MainWindow::showStatusMsg);
}

void MainWindow::setupMenuBar()
{
    // ---- File ----
    QMenu* fileMenu = menuBar()->addMenu("&File");

    fileMenu->addAction("Open Image",   this, &MainWindow::openImage,
                        QKeySequence::Open);
    fileMenu->addAction("Open Folder",  this, &MainWindow::openFolder,
                        QKeySequence("Ctrl+Shift+O"));
    fileMenu->addSeparator();
    fileMenu->addAction("Previous Image", this, &MainWindow::prevImage,
                        QKeySequence("A"));
    fileMenu->addAction("Next Image",     this, &MainWindow::nextImage,
                        QKeySequence("D"));
    fileMenu->addSeparator();
    fileMenu->addAction("Save Annotations", this, &MainWindow::saveAnnotations,
                        QKeySequence::Save);
    fileMenu->addAction("Load Annotations", this, &MainWindow::loadAnnotations,
                        QKeySequence("Ctrl+L"));
    fileMenu->addSeparator();
    fileMenu->addAction("Quit", qApp, &QApplication::quit, QKeySequence::Quit);

    // ---- Edit ----
    QMenu* editMenu = menuBar()->addMenu("&Edit");
    editMenu->addAction("Delete Selected", this, &MainWindow::deleteSelectedAnnotation,
                        QKeySequence::Delete);
    editMenu->addAction("Clear All",       this, &MainWindow::clearAnnotations);
    editMenu->addSeparator();
    editMenu->addAction("Add Label",       this, &MainWindow::addLabel);
    editMenu->addAction("Remove Label",    this, &MainWindow::removeLabel);

    // ---- View ----
    QMenu* viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction("Zoom In",    this, &MainWindow::zoomIn,      QKeySequence("="));
    viewMenu->addAction("Zoom Out",   this, &MainWindow::zoomOut,     QKeySequence("-"));
    viewMenu->addAction("Fit Window", this, &MainWindow::fitToWindow, QKeySequence("F"));
    viewMenu->addAction("Reset Zoom", this, &MainWindow::resetZoom,   QKeySequence("R"));
    viewMenu->addSeparator();
    // 面板显示/隐藏（setupDockWidgets之后才能用，先占位，在setupDockWidgets末尾添加）
    // ---- 数据库 ----   ★★★ 新增 ★★★
    QMenu* dbMenu = menuBar()->addMenu("&数据库");
    dbMenu->addAction("📊 查看记录", this, [this]() {
        DbViewer viewer(this);
        viewer.exec();
    });
    // ---- Help ----
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("Shortcuts", this, [this]() {
        QMessageBox::information(this, "快捷键说明",
                                 "鼠标左键拖拽  - 绘制标注框\n"
                                 "鼠标中键拖拽  - 平移图像\n"
                                 "鼠标滚轮      - 缩放\n"
                                 "右键          - 取消选中\n"
                                 "Delete        - 删除选中框\n"
                                 "Escape        - 取消当前操作\n"
                                 "A / D         - 上一张 / 下一张\n"
                                 "S             - 保存标注\n"
                                 "= / -         - 放大 / 缩小\n"
                                 "F             - 适应窗口\n"
                                 "R             - 重置缩放\n"
                                 );
    });
}

void MainWindow::setupToolBar()
{
    QToolBar* tb = addToolBar("工具栏");
    tb->setMovable(false);
    tb->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    tb->addAction("Open",   this, &MainWindow::openImage);
    tb->addAction("Folder", this, &MainWindow::openFolder);
    tb->addSeparator();
    tb->addAction("Prev",   this, &MainWindow::prevImage);
    tb->addAction("Next",   this, &MainWindow::nextImage);
    tb->addSeparator();
    tb->addAction("Save",   this, &MainWindow::saveAnnotations);
    tb->addSeparator();
    tb->addAction("Zoom+",  this, &MainWindow::zoomIn);
    tb->addAction("Zoom-",  this, &MainWindow::zoomOut);
    tb->addAction("Fit",    this, &MainWindow::fitToWindow);
    tb->addAction("↻ 顺转", this, &MainWindow::rotateClockwise);      // ← 新增
    tb->addAction("↺ 逆转", this, &MainWindow::rotateCounterClockwise); // ← 新增
    tb->addSeparator();
    tb->addAction("Delete", this, &MainWindow::deleteSelectedAnnotation);
    tb->addAction("Clear",  this, &MainWindow::clearAnnotations);
}

void MainWindow::setupDockWidgets()
{
    // ===== 左侧：文件列表 =====
    m_fileDock = new QDockWidget("图像文件", this);
    m_fileDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    QWidget* fileWidget = new QWidget;
    QVBoxLayout* fileLayout = new QVBoxLayout(fileWidget);
    fileLayout->setContentsMargins(4, 4, 4, 4);

    m_fileList = new QListWidget;
    m_fileList->setMaximumWidth(200);
    connect(m_fileList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0 && row < m_imageFiles.size()) {
            m_currentImageIndex = row;
            loadImageFile(m_imageFiles[row]);
        }
    });
    fileLayout->addWidget(m_fileList);
    m_fileDock->setWidget(fileWidget);
    addDockWidget(Qt::LeftDockWidgetArea, m_fileDock);

    // ===== 右侧：标签 & 标注列表 =====
    m_annDock = new QDockWidget("标注面板", this);
    m_annDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    QWidget* annWidget = new QWidget;
    QVBoxLayout* annLayout = new QVBoxLayout(annWidget);
    annLayout->setContentsMargins(4, 4, 4, 4);
    annLayout->setSpacing(6);

    // 图像信息
    m_imageInfoLabel = new QLabel("无图像");
    m_imageInfoLabel->setAlignment(Qt::AlignCenter);
    m_imageInfoLabel->setStyleSheet(
        "background:#2a2a2a;color:#aaa;padding:4px;border-radius:3px;");
    annLayout->addWidget(m_imageInfoLabel);

    // 标签选择
    annLayout->addWidget(new QLabel("当前标签:"));
    m_labelCombo = new QComboBox;
    m_labelCombo->setEditable(false);
    for (const QString& lbl : m_labelManager.getLabels())
        m_labelCombo->addItem(lbl);
    connect(m_labelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onLabelComboChanged);
    annLayout->addWidget(m_labelCombo);

    // 添加/删除标签按钮
    QHBoxLayout* btnLayout = new QHBoxLayout;
    QPushButton* addLblBtn = new QPushButton("+ 添加标签");
    QPushButton* delLblBtn = new QPushButton("- 删除标签");
    addLblBtn->setStyleSheet("background:#3a6ea5;color:white;border-radius:3px;padding:3px;");
    delLblBtn->setStyleSheet("background:#a53a3a;color:white;border-radius:3px;padding:3px;");
    connect(addLblBtn, &QPushButton::clicked, this, &MainWindow::addLabel);
    connect(delLblBtn, &QPushButton::clicked, this, &MainWindow::removeLabel);
    btnLayout->addWidget(addLblBtn);
    btnLayout->addWidget(delLblBtn);
    annLayout->addLayout(btnLayout);

    annLayout->addWidget(new QLabel("标注列表:"));
    m_annotationList = new QListWidget;
    connect(m_annotationList, &QListWidget::currentRowChanged,
            this, &MainWindow::onLabelSelected);
    annLayout->addWidget(m_annotationList);

    // 操作按钮
    QPushButton* delAnnBtn = new QPushButton("删除选中标注");
    QPushButton* clrAnnBtn = new QPushButton("清除所有标注");
    QPushButton* saveBtn   = new QPushButton("保存标注");
    delAnnBtn->setStyleSheet("background:#a53a3a;color:white;padding:4px;border-radius:3px;");
    clrAnnBtn->setStyleSheet("background:#555;color:white;padding:4px;border-radius:3px;");
    saveBtn->setStyleSheet  ("background:#3a7a3a;color:white;padding:4px;border-radius:3px;");
    connect(delAnnBtn, &QPushButton::clicked, this, &MainWindow::deleteSelectedAnnotation);
    connect(clrAnnBtn, &QPushButton::clicked, this, &MainWindow::clearAnnotations);
    connect(saveBtn,   &QPushButton::clicked, this, &MainWindow::saveAnnotations);
    annLayout->addWidget(delAnnBtn);
    annLayout->addWidget(clrAnnBtn);
    annLayout->addWidget(saveBtn);

    // 设置保存文件夹按钮
    QPushButton* saveFolderBtn = new QPushButton("设置保存文件夹");
    saveFolderBtn->setStyleSheet("background:#555;color:white;padding:4px;border-radius:3px;");
    connect(saveFolderBtn, &QPushButton::clicked, this, [this]() {
        QString folder = QFileDialog::getExistingDirectory(
            this, "选择标注文件保存文件夹", m_currentFolder);
        if (!folder.isEmpty()) {
            m_saveFolder = folder;
            showStatusMsg("保存文件夹已设置为: " + folder);
        }
    });
    annLayout->addWidget(saveFolderBtn);

    m_annDock->setWidget(annWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_annDock);

    // 把面板开关加入 View 菜单
    QMenu* viewMenu = menuBar()->findChild<QMenu*>();
    // 直接找到View菜单并添加
    for (QAction* action : menuBar()->actions()) {
        if (action->text() == "&View") {
            action->menu()->addSeparator();
            action->menu()->addAction(m_fileDock->toggleViewAction());
            action->menu()->addAction(m_annDock->toggleViewAction());
            break;
        }
    }
}

void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel("就绪");
    m_coordLabel  = new QLabel("");
    m_countLabel  = new QLabel("标注: 0");

    statusBar()->addWidget(m_statusLabel, 1);
    statusBar()->addPermanentWidget(m_countLabel);
    statusBar()->addPermanentWidget(m_coordLabel);
}

void MainWindow::setupShortcuts()
{
    new QShortcut(QKeySequence("S"), this, SLOT(saveAnnotations()));
    new QShortcut(QKeySequence("W"), this, [this]() {
        int idx = (m_labelCombo->currentIndex() + 1) % m_labelCombo->count();
        m_labelCombo->setCurrentIndex(idx);
    });
}

// ======================== 文件操作 ========================

void MainWindow::openImage()
{
    QString path = QFileDialog::getOpenFileName(
        this, "打开图像", "",
        "Images (*.jpg *.jpeg *.png *.bmp *.gif *.tiff *.webp);;All Files (*)"
        );
    if (path.isEmpty()) return;

    QFileInfo fi(path);
    m_currentFolder     = fi.absolutePath();
    m_imageFiles        = QStringList{path};
    m_currentImageIndex = 0;

    updateImageList();
    loadImageFile(path);
}

void MainWindow::openFolder()
{
    QString folder = QFileDialog::getExistingDirectory(this, "打开文件夹");
    if (folder.isEmpty()) return;

    m_currentFolder = folder;
    QDir dir(folder);
    m_imageFiles.clear();
    for (const QString& filter : IMAGE_FILTERS) {
        for (const QString& f : dir.entryList({filter}, QDir::Files, QDir::Name))
            m_imageFiles.append(dir.absoluteFilePath(f));
    }

    if (m_imageFiles.isEmpty()) {
        QMessageBox::information(this, "提示", "该文件夹中没有图像文件");
        return;
    }

    updateImageList();
    m_currentImageIndex = 0;
    m_fileList->setCurrentRow(0);
    loadImageFile(m_imageFiles[0]);
    showStatusMsg(QString("加载文件夹: %1  共 %2 张图像")
                      .arg(folder).arg(m_imageFiles.size()));
}

void MainWindow::loadImageFile(const QString& path)
{
    // ★★★ 新增：切换前自动保存当前图片的标注 ★★★
    if (m_imageView->hasImage() && !m_imageView->getAnnotations().isEmpty()) {
        QString oldImagePath = m_imageView->currentImagePath();
        QFileInfo oldFi(oldImagePath);

        // 决定保存到哪个文件夹
        QString saveDir = m_saveFolder.isEmpty()
                              ? oldFi.absolutePath()   // 没设置就保存到图片所在文件夹
                              : m_saveFolder;          // 设置过就用指定文件夹

        QString autoSavePath = saveDir + "/" + oldFi.baseName() + ".txt";
        saveYOLO(autoSavePath);
        showStatusMsg("自动保存: " + autoSavePath);
    }
    // ★★★ 新增代码结束 ★★★
    m_imageView->loadImage(path);
    QFileInfo fi(path);
    QPixmap px(path);
    m_imageInfoLabel->setText(QString("%1\n%2 x %3")
                                  .arg(fi.fileName())
                                  .arg(px.isNull() ? 0 : px.width())
                                  .arg(px.isNull() ? 0 : px.height()));

    // ★★★ 修改：先在保存文件夹里找，再在图片文件夹里找 ★★★
    QString yoloPath;
    if (!m_saveFolder.isEmpty()) {
        QString p1 = m_saveFolder + "/" + fi.baseName() + ".txt";
        if (QFileInfo::exists(p1)) yoloPath = p1;
    }
    if (yoloPath.isEmpty()) {
        QString p2 = fi.absolutePath() + "/" + fi.baseName() + ".txt";
        if (QFileInfo::exists(p2)) yoloPath = p2;
    }

    if (!yoloPath.isEmpty()) {
        loadYOLO(yoloPath);
        showStatusMsg("自动加载标注: " + yoloPath);
    }
    // ★★★ 修改结束 ★★★
    updateWindowTitle();
}

void MainWindow::prevImage()
{
    if (m_imageFiles.size() < 2) return;
    m_currentImageIndex =
        (m_currentImageIndex - 1 + m_imageFiles.size()) % m_imageFiles.size();
    m_fileList->setCurrentRow(m_currentImageIndex);
    loadImageFile(m_imageFiles[m_currentImageIndex]);
}

void MainWindow::nextImage()
{
    if (m_imageFiles.size() < 2) return;
    m_currentImageIndex = (m_currentImageIndex + 1) % m_imageFiles.size();
    m_fileList->setCurrentRow(m_currentImageIndex);
    loadImageFile(m_imageFiles[m_currentImageIndex]);
}

void MainWindow::updateImageList()
{
    m_fileList->clear();
    for (const QString& path : m_imageFiles) {
        QFileInfo fi(path);
        m_fileList->addItem(fi.fileName());
    }
}

// ======================== 标注保存/加载 ========================

void MainWindow::saveAnnotations()
{
    if (!m_imageView->hasImage()) return;

    // 第一次保存时让用户选择文件夹
    if (m_saveFolder.isEmpty()) {
        m_saveFolder = QFileDialog::getExistingDirectory(
            this, "选择标注文件保存文件夹", m_currentFolder);
        if (m_saveFolder.isEmpty()) return;
    }

    int ret = QMessageBox::question(
        this, "保存格式", "选择标注文件格式：",
        "YOLO (.txt)", "VOC XML (.xml)", "取消"
        );

    QFileInfo fi(m_imageView->currentImagePath());
    if (ret == 0) {
        QString savePath = m_saveFolder + "/" + fi.baseName() + ".txt";
        saveYOLO(savePath);
        showStatusMsg("已保存 YOLO 格式: " + savePath);
    } else if (ret == 1) {
        QString savePath = m_saveFolder + "/" + fi.baseName() + ".xml";
        saveVOC(savePath);
        showStatusMsg("已保存 VOC XML 格式: " + savePath);
    }
}

void MainWindow::saveYOLO(const QString& path)
{
    if (!m_imageView->hasImage()) return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream out(&file);
    auto anns = m_imageView->getAnnotations();
    QStringList labels = m_labelManager.getLabels();

    QPixmap px(m_imageView->currentImagePath());
    double imgW = px.isNull() ? 640 : px.width();
    double imgH = px.isNull() ? 480 : px.height();

    for (const auto& ann : anns) {
        int classId = labels.indexOf(ann.label);
        if (classId < 0) classId = 0;

        double cx = (ann.rect.x() + ann.rect.width()  / 2.0) / imgW;
        double cy = (ann.rect.y() + ann.rect.height() / 2.0) / imgH;
        double w  = ann.rect.width()  / imgW;
        double h  = ann.rect.height() / imgH;

        out << classId << " "
            << QString::number(cx, 'f', 6) << " "
            << QString::number(cy, 'f', 6) << " "
            << QString::number(w,  'f', 6) << " "
            << QString::number(h,  'f', 6) << "\n";
    }
    file.close();
}

void MainWindow::saveVOC(const QString& path)
{
    if (!m_imageView->hasImage()) return;

    QFileInfo imgFi(m_imageView->currentImagePath());
    QPixmap   px(m_imageView->currentImagePath());
    int imgW = px.isNull() ? 0 : px.width();
    int imgH = px.isNull() ? 0 : px.height();

    QDomDocument doc;
    QDomElement root = doc.createElement("annotation");
    doc.appendChild(root);

    auto addText = [&](QDomElement& parent, const QString& tag, const QString& val) {
        QDomElement el = doc.createElement(tag);
        el.appendChild(doc.createTextNode(val));
        parent.appendChild(el);
    };

    addText(root, "folder",   imgFi.absolutePath());
    addText(root, "filename", imgFi.fileName());

    QDomElement size = doc.createElement("size");
    addText(size, "width",  QString::number(imgW));
    addText(size, "height", QString::number(imgH));
    addText(size, "depth",  "3");
    root.appendChild(size);

    for (const auto& ann : m_imageView->getAnnotations()) {
        QDomElement obj = doc.createElement("object");
        addText(obj, "name",      ann.label);
        addText(obj, "pose",      "Unspecified");
        addText(obj, "truncated", "0");
        addText(obj, "difficult", "0");

        QDomElement bndbox = doc.createElement("bndbox");
        addText(bndbox, "xmin", QString::number((int)ann.rect.left()));
        addText(bndbox, "ymin", QString::number((int)ann.rect.top()));
        addText(bndbox, "xmax", QString::number((int)ann.rect.right()));
        addText(bndbox, "ymax", QString::number((int)ann.rect.bottom()));
        obj.appendChild(bndbox);
        root.appendChild(obj);
    }

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << doc.toString(4);
        file.close();
    }
}

void MainWindow::loadAnnotations()
{
    if (!m_imageView->hasImage()) return;
    QString path = QFileDialog::getOpenFileName(
        this, "加载标注", m_currentFolder,
        "YOLO (*.txt);;VOC XML (*.xml);;All Files (*)"
        );
    if (path.isEmpty()) return;

    if (path.endsWith(".txt", Qt::CaseInsensitive))
        loadYOLO(path);
    else if (path.endsWith(".xml", Qt::CaseInsensitive))
        loadVOC(path);
}

void MainWindow::loadYOLO(const QString& path)
{
    if (!m_imageView->hasImage()) return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QPixmap px(m_imageView->currentImagePath());
    double imgW = px.isNull() ? 640 : px.width();
    double imgH = px.isNull() ? 480 : px.height();

    QStringList labels = m_labelManager.getLabels();
    QList<AnnotationItem> anns;
    int id = 1;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        QStringList parts = line.split(' ');
        if (parts.size() < 5) continue;

        int    classId = parts[0].toInt();
        double cx = parts[1].toDouble() * imgW;
        double cy = parts[2].toDouble() * imgH;
        double w  = parts[3].toDouble() * imgW;
        double h  = parts[4].toDouble() * imgH;

        QString label = (classId < labels.size()) ? labels[classId]
                                                  : QString("class%1").arg(classId);
        if (!m_labelManager.contains(label)) {
            m_labelManager.addLabel(label);
            m_labelCombo->addItem(label);
        }

        QRectF rect(cx - w / 2.0, cy - h / 2.0, w, h);
        anns.append(AnnotationItem(id++, label, rect, m_labelManager.getColor(label)));
    }
    file.close();
    m_imageView->setAnnotations(anns);
}

void MainWindow::loadVOC(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;

    QDomDocument doc;
    if (!doc.setContent(&file)) { file.close(); return; }
    file.close();

    QList<AnnotationItem> anns;
    int id = 1;
    QDomNodeList objects = doc.elementsByTagName("object");
    for (int i = 0; i < objects.size(); i++) {
        QDomElement obj = objects.at(i).toElement();
        QString label = obj.firstChildElement("name").text();

        if (!m_labelManager.contains(label)) {
            m_labelManager.addLabel(label);
            m_labelCombo->addItem(label);
        }

        QDomElement bb = obj.firstChildElement("bndbox");
        double xmin = bb.firstChildElement("xmin").text().toDouble();
        double ymin = bb.firstChildElement("ymin").text().toDouble();
        double xmax = bb.firstChildElement("xmax").text().toDouble();
        double ymax = bb.firstChildElement("ymax").text().toDouble();

        QRectF rect(xmin, ymin, xmax - xmin, ymax - ymin);
        anns.append(AnnotationItem(id++, label, rect, m_labelManager.getColor(label)));
    }
    m_imageView->setAnnotations(anns);
}

// ======================== 标注操作 ========================

void MainWindow::onAnnotationChanged()
{
    updateAnnotationList();
    m_countLabel->setText(QString("标注: %1")
                              .arg(m_imageView->getAnnotations().size()));
}

void MainWindow::onAnnotationSelected(int id)
{
    auto anns = m_imageView->getAnnotations();
    for (int i = 0; i < anns.size(); i++) {
        if (anns[i].id == id) {
            m_annotationList->setCurrentRow(i);
            break;
        }
    }
}

void MainWindow::onLabelSelected(int row)
{
    Q_UNUSED(row);
}

void MainWindow::deleteSelectedAnnotation()
{
    m_imageView->deleteSelectedAnnotation();
}

void MainWindow::clearAnnotations()
{
    int ret = QMessageBox::question(this, "确认", "确定要清除所有标注吗？",
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes)
        m_imageView->clearAnnotations();
}

void MainWindow::updateAnnotationList()
{
    m_annotationList->clear();
    auto anns = m_imageView->getAnnotations();
    for (const auto& ann : anns) {
        QString text = QString("[%1] %2  (%3,%4  %5x%6)")
        .arg(ann.id)
            .arg(ann.label)
            .arg((int)ann.rect.x())
            .arg((int)ann.rect.y())
            .arg((int)ann.rect.width())
            .arg((int)ann.rect.height());
        QListWidgetItem* item = new QListWidgetItem(text);
        item->setForeground(ann.color);
        m_annotationList->addItem(item);
    }
}

// ======================== 标签管理 ========================

void MainWindow::addLabel()
{
    QString label = QInputDialog::getText(this, "添加标签", "标签名称:");
    if (label.isEmpty()) return;
    if (m_labelManager.contains(label)) {
        QMessageBox::information(this, "提示", "标签已存在！");
        return;
    }
    m_labelManager.addLabel(label);
    m_labelCombo->addItem(label);
    m_labelCombo->setCurrentText(label);
    showStatusMsg("添加标签: " + label);
}

void MainWindow::removeLabel()
{
    QString current = m_labelCombo->currentText();
    if (current.isEmpty()) return;
    int ret = QMessageBox::question(this, "确认", "删除标签: " + current + "?",
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;
    m_labelManager.removeLabel(current);
    m_labelCombo->removeItem(m_labelCombo->currentIndex());
}

void MainWindow::onLabelComboChanged(int index)
{
    if (index < 0) return;
    m_imageView->setCurrentLabel(m_labelCombo->currentText());
}

// ======================== 辅助 ========================

void MainWindow::showStatusMsg(const QString& msg)
{
    m_statusLabel->setText(msg);
}

void MainWindow::updateWindowTitle()
{
    QString title = "目标检测标注工具";
    if (m_imageView->hasImage()) {
        QFileInfo fi(m_imageView->currentImagePath());
        title += " - " + fi.fileName();
        if (!m_imageFiles.isEmpty())
            title += QString(" [%1/%2]")
                         .arg(m_currentImageIndex + 1)
                         .arg(m_imageFiles.size());
    }
    setWindowTitle(title);
}