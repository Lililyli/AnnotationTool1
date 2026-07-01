#include "trainwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QScrollBar>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QDebug>
#include <QStringList>
#include <QScrollArea>
#include "database.h"
// 将带有 ANSI 颜色代码的文本转换为 HTML 富文本
QString ansiToHtml(const QString& ansiString) {
    QString html;
    int i = 0;
    QString currentColor = "";
    bool isBold = false;
    bool inSpan = false;

    // 闭合当前 HTML 标签的 lambda
    auto closeSpan = [&]() {
        if (inSpan) {
            html += "</span>";
            inSpan = false;
        }
    };

    // 开启新的 HTML 标签的 lambda
    auto openSpan = [&]() {
        if (currentColor.isEmpty() && !isBold) return;
        QString style;
        if (!currentColor.isEmpty()) style += "color:" + currentColor + ";";
        if (isBold) style += "font-weight:bold;";
        html += "<span style=\"" + style + "\">";
        inSpan = true;
    };

    while (i < ansiString.length()) {
        // 检测到 ANSI 转义序列的开头：ESC (即 \x1B) 和 [
        if (ansiString[i] == '\x1B' && i + 1 < ansiString.length() && ansiString[i+1] == '[') {
            int j = i + 2;
            QString codeStr;
            // 提取控制数字，直到遇到结束符 'm' (颜色) 或 'K' (清空行)
            while (j < ansiString.length() && ansiString[j] != 'm' && ansiString[j] != 'K' && ansiString[j].isPrint()) {
                codeStr += ansiString[j];
                j++;
            }

            if (j < ansiString.length() && ansiString[j] == 'm') {
                // 遇到 'm'，说明是颜色/样式代码
                closeSpan();
                QStringList codes = codeStr.split(';');
                for (const QString& c : codes) {
                    int code = c.toInt();
                    if (code == 0) { currentColor = ""; isBold = false; } // 重置
                    else if (code == 1) { isBold = true; } // 加粗
                    // 标准颜色 30-37
                    else if (code >= 30 && code <= 37) {
                        const char* colors[] = {"black", "red", "green", "olive", "blue", "magenta", "teal", "white"};
                        currentColor = colors[code - 30];
                    }
                    // 亮色 90-97 (YOLO经常用亮蓝色或亮绿色)
                    else if (code >= 90 && code <= 97) {
                        const char* brightColors[] = {"gray", "lightcoral", "lightgreen", "lightyellow", "lightblue", "lightpink", "lightcyan", "white"};
                        currentColor = brightColors[code - 90];
                    }
                }
                openSpan();
                i = j + 1; // 跳过这个转义序列
            } else if (j < ansiString.length() && ansiString[j] == 'K') {
                // 遇到 'K'，这是进度条用来清除当前行的指令，我们这里直接忽略它
                i = j + 1;
            } else {
                // 无法识别的转义字符，直接跳过头部
                i += 2;
            }
        } else {
            // 处理普通文本，防止与 HTML 标签冲突
            if (ansiString[i] == '<') html += "&lt;";
            else if (ansiString[i] == '>') html += "&gt;";
            else if (ansiString[i] == '&') html += "&amp;";
            else if (ansiString[i] == '\n') html += "<br>"; // 将换行转为 HTML 换行
            // 我们过滤掉 '\r' (回车)，因为在纯文本框追加 HTML 时，无法完美实现原地覆盖，干脆删掉避免排版混乱
            else if (ansiString[i] != '\r') html += ansiString[i];

            i++;
        }
    }
    closeSpan();
    return html;
}
TrainWidget::TrainWidget(QWidget* parent)
    : QWidget(parent)
    , m_process(nullptr)
    , m_totalEpochs(100)
    , m_currentEpoch(0)
{
    setupUI();
}

TrainWidget::~TrainWidget()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }
}

void TrainWidget::setupUI()
{
    // 1. 创建最外层的布局和滚动区域
    QVBoxLayout* wrapperLayout = new QVBoxLayout(this);
    wrapperLayout->setContentsMargins(0, 0, 0, 0); // 让滚动条贴边

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true); // 必须：让内部widget自适应大小
    scrollArea->setFrameShape(QFrame::NoFrame); // 去除自带边框，更好看
    wrapperLayout->addWidget(scrollArea);

    // 2. 创建真正的内部容器，原本属于 this 的 mainLayout 现在给 container
    QWidget* container = new QWidget(scrollArea);
    QVBoxLayout* mainLayout = new QVBoxLayout(container);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // ==================== 路径设置组 ====================
    QGroupBox* pathGroup = new QGroupBox("路径设置");
    QGridLayout* pathLayout = new QGridLayout(pathGroup);
    pathLayout->setColumnStretch(1, 1);

    pathLayout->addWidget(new QLabel("Python路径:"), 0, 0);
    m_pythonEdit = new QLineEdit("D:/software/anaconda/python.exe");
    pathLayout->addWidget(m_pythonEdit, 0, 1);
    QPushButton* pythonBtn = new QPushButton("浏览");
    pythonBtn->setFixedWidth(60);
    connect(pythonBtn, &QPushButton::clicked, this, &TrainWidget::selectPythonPath);
    pathLayout->addWidget(pythonBtn, 0, 2);

    pathLayout->addWidget(new QLabel("数据集yaml:"), 1, 0);
    m_yamlEdit = new QLineEdit("D:/software/projectQT/maskdata/data.yaml");
    pathLayout->addWidget(m_yamlEdit, 1, 1);
    QPushButton* yamlBtn = new QPushButton("浏览");
    yamlBtn->setFixedWidth(60);
    connect(yamlBtn, &QPushButton::clicked, this, &TrainWidget::selectYamlFile);
    pathLayout->addWidget(yamlBtn, 1, 2);

    pathLayout->addWidget(new QLabel("权重文件:"), 2, 0);
    m_weightEdit = new QLineEdit("yolo11n.pt");
    pathLayout->addWidget(m_weightEdit, 2, 1);
    QPushButton* weightBtn = new QPushButton("浏览");
    weightBtn->setFixedWidth(60);
    connect(weightBtn, &QPushButton::clicked, this, &TrainWidget::selectWeightFile);
    pathLayout->addWidget(weightBtn, 2, 2);

    pathLayout->addWidget(new QLabel("输出路径:"), 3, 0);
    m_outputEdit = new QLineEdit("D:/software/projectQT/maskdata/runs");
    pathLayout->addWidget(m_outputEdit, 3, 1);
    QPushButton* outputBtn = new QPushButton("浏览");
    outputBtn->setFixedWidth(60);
    connect(outputBtn, &QPushButton::clicked, this, &TrainWidget::selectOutputFolder);
    pathLayout->addWidget(outputBtn, 3, 2);

    mainLayout->addWidget(pathGroup);

    // ==================== 训练参数组 ====================
    QGroupBox* paramGroup = new QGroupBox("训练参数");
    QGridLayout* paramLayout = new QGridLayout(paramGroup);

    paramLayout->addWidget(new QLabel("Epochs:"), 0, 0);
    m_epochsSpin = new QSpinBox; m_epochsSpin->setRange(1, 1000); m_epochsSpin->setValue(50);
    paramLayout->addWidget(m_epochsSpin, 0, 1);

    paramLayout->addWidget(new QLabel("Batch Size:"), 0, 2);
    m_batchSpin = new QSpinBox; m_batchSpin->setRange(1, 128); m_batchSpin->setValue(16);
    paramLayout->addWidget(m_batchSpin, 0, 3);

    paramLayout->addWidget(new QLabel("图像尺寸:"), 1, 0);
    m_imgSizeSpin = new QSpinBox; m_imgSizeSpin->setRange(32, 1280); m_imgSizeSpin->setValue(640); m_imgSizeSpin->setSingleStep(32);
    paramLayout->addWidget(m_imgSizeSpin, 1, 1);

    paramLayout->addWidget(new QLabel("学习率:"), 1, 2);
    m_lrSpin = new QDoubleSpinBox; m_lrSpin->setRange(0.00001, 1.0); m_lrSpin->setValue(0.01); m_lrSpin->setDecimals(5); m_lrSpin->setSingleStep(0.001);
    paramLayout->addWidget(m_lrSpin, 1, 3);

    paramLayout->addWidget(new QLabel("Workers:"), 2, 0);
    m_workersSpin = new QSpinBox; m_workersSpin->setRange(0, 16); m_workersSpin->setValue(4);
    paramLayout->addWidget(m_workersSpin, 2, 1);

    paramLayout->addWidget(new QLabel("使用GPU:"), 2, 2);
    m_gpuCheck = new QCheckBox("启用 (需要CUDA)"); m_gpuCheck->setChecked(false);
    paramLayout->addWidget(m_gpuCheck, 2, 3);

    paramLayout->addWidget(new QLabel("额外参数:"), 3, 0);
    m_extraArgsEdit = new QLineEdit; m_extraArgsEdit->setPlaceholderText("如: patience=50 save_period=10");
    paramLayout->addWidget(m_extraArgsEdit, 3, 1, 1, 3);

    mainLayout->addWidget(paramGroup);

    // ==================== 控制按钮 ====================
    QHBoxLayout* ctrlLayout = new QHBoxLayout;

    m_startBtn = new QPushButton("▶  开始训练");
    m_startBtn->setFixedHeight(42);
    m_startBtn->setStyleSheet("QPushButton{background:#3a7a3a;color:white;font-size:14px;font-weight:bold;border-radius:5px;}QPushButton:hover{background:#4a9a4a;}QPushButton:disabled{background:#2a4a2a;color:#666;}");
    connect(m_startBtn, &QPushButton::clicked, this, &TrainWidget::startTraining);

    m_stopBtn = new QPushButton("■  停止训练");
    m_stopBtn->setFixedHeight(42);
    m_stopBtn->setEnabled(false);
    m_stopBtn->setStyleSheet("QPushButton{background:#a53a3a;color:white;font-size:14px;font-weight:bold;border-radius:5px;}QPushButton:hover{background:#c54a4a;}QPushButton:disabled{background:#4a2a2a;color:#666;}");
    connect(m_stopBtn, &QPushButton::clicked, this, &TrainWidget::stopTraining);

    ctrlLayout->addWidget(m_startBtn);
    ctrlLayout->addWidget(m_stopBtn);
    mainLayout->addLayout(ctrlLayout);

    // ==================== 进度显示 ====================
    m_statusLabel = new QLabel("就绪 — 请检查路径后点击开始训练");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("color:#aaa;padding:3px;font-size:12px;");
    mainLayout->addWidget(m_statusLabel);

    m_progressBar = new QProgressBar;
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFormat("等待开始...");
    m_progressBar->setFixedHeight(22);
    mainLayout->addWidget(m_progressBar);

    // ==================== 日志输出 ====================
    QGroupBox* logGroup = new QGroupBox("训练日志");
    QVBoxLayout* logLayout = new QVBoxLayout(logGroup);

    m_logEdit = new QTextEdit;
    m_logEdit->setReadOnly(true);
    m_logEdit->setFont(QFont("Courier New", 9));
    m_logEdit->setStyleSheet("background:#1a1a1a;color:#e0e0e0;border:1px solid #333;");

    // 注意：既然外层有了 QScrollArea，日志框本身就不需要太高了
    // 把它改成一个合理的值，比如 300，避免内容过长撑爆滚动区域
    m_logEdit->setMinimumHeight(300);

    logLayout->addWidget(m_logEdit);

    QPushButton* clearLogBtn = new QPushButton("清空日志");
    clearLogBtn->setFixedWidth(80);
    clearLogBtn->setStyleSheet("background:#444;color:white;border-radius:3px;padding:2px;");
    connect(clearLogBtn, &QPushButton::clicked, m_logEdit, &QTextEdit::clear);
    logLayout->addWidget(clearLogBtn, 0, Qt::AlignRight);

    mainLayout->addWidget(logGroup);

    // 3. 最后一步：把装满控件的 container 塞进 scrollArea 里
    scrollArea->setWidget(container);
}
// ======================== 路径选择 ========================

void TrainWidget::selectPythonPath()
{
    QString path = QFileDialog::getOpenFileName(
        this, "选择Python解释器", "",
        "Python (*.exe);;所有文件 (*)"
        );
    if (!path.isEmpty())
        m_pythonEdit->setText(path);
}

void TrainWidget::selectYamlFile()
{
    QString path = QFileDialog::getOpenFileName(
        this, "选择数据集yaml文件",
        "D:/software/projectQT/maskdata",
        "YAML文件 (*.yaml *.yml);;所有文件 (*)"
        );
    if (!path.isEmpty())
        m_yamlEdit->setText(path);
}

void TrainWidget::selectWeightFile()
{
    QString path = QFileDialog::getOpenFileName(
        this, "选择权重文件", "",
        "权重文件 (*.pt *.pth);;所有文件 (*)"
        );
    if (!path.isEmpty())
        m_weightEdit->setText(path);
}

void TrainWidget::selectOutputFolder()
{
    QString folder = QFileDialog::getExistingDirectory(
        this, "选择输出文件夹",
        "D:/software/projectQT/maskdata"
        );
    if (!folder.isEmpty())
        m_outputEdit->setText(folder);
}

// ======================== 训练控制 ========================

void TrainWidget::startTraining()
{
    // 检查必要路径
    if (!QFileInfo::exists(m_pythonEdit->text())) {
        QMessageBox::warning(this, "提示",
                             "Python路径不存在：\n" + m_pythonEdit->text() +
                                 "\n\n请点击浏览选择正确的python.exe");
        return;
    }
    if (m_yamlEdit->text().isEmpty() ||
        !QFileInfo::exists(m_yamlEdit->text())) {
        QMessageBox::warning(this, "提示",
                             "data.yaml 文件不存在：\n" + m_yamlEdit->text());
        return;
    }
    if (m_outputEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择输出路径！");
        return;
    }

    // 确保输出目录存在
    QDir().mkpath(m_outputEdit->text());

    // 重置进度
    m_totalEpochs  = m_epochsSpin->value();
    m_currentEpoch = 0;
    m_progressBar->setRange(0, m_totalEpochs);
    m_progressBar->setValue(0);
    m_progressBar->setFormat("Epoch 0 / " + QString::number(m_totalEpochs));

    // 销毁旧进程
    if (m_process) {
        m_process->kill();
        m_process->deleteLater();
        m_process = nullptr;
    }

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &TrainWidget::onProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &TrainWidget::onProcessError);
    connect(m_process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TrainWidget::onProcessFinished);

    QString      program = m_pythonEdit->text();
    QStringList  args    = buildArgs();

    appendLog("═══════════════════════════════════════════", "#444444");
    appendLog(QString("▶ 开始时间: %1")
                  .arg(QDateTime::currentDateTime()
                           .toString("yyyy-MM-dd hh:mm:ss")), "#888888");
    appendLog("▶ 命令: " + program + " " + args.join(" "), "#aaaaaa");
    appendLog("═══════════════════════════════════════════", "#444444");

    m_process->start(program, args);

    if (!m_process->waitForStarted(5000)) {
        appendLog("❌ 启动失败！请确认：", "#ff4444");
        appendLog("   1. Python路径正确", "#ff6666");
        appendLog("   2. ultralytics已安装: pip install ultralytics", "#ff6666");
        m_statusLabel->setText("启动失败");
        return;
    }

    setTrainingState(true);
    appendLog("✅ 训练进程启动成功", "#44ff44");
}

void TrainWidget::stopTraining()
{
    if (!m_process) return;
    int ret = QMessageBox::question(
        this, "确认停止",
        "确定要停止当前训练吗？",
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    m_process->kill();
    appendLog("⚠  用户手动停止训练", "#ffaa00");
    setTrainingState(false);
}

QStringList TrainWidget::buildArgs()
{
    // 路径统一用正斜杠，避免Python字符串转义问题
    QString yamlPath   = m_yamlEdit->text().replace("\\", "/");
    QString outputPath = m_outputEdit->text().replace("\\", "/");
    QString weightPath = m_weightEdit->text().replace("\\", "/");
    QString device     = m_gpuCheck->isChecked() ? "0" : "cpu";

    // 拼成一行Python代码传给 python -c 执行
    QString pyCode = QString(
                         "from ultralytics import YOLO; "
                         "model = YOLO('%1'); "
                         "model.train("
                         "data='%2', "
                         "epochs=%3, "
                         "imgsz=%4, "
                         "batch=%5, "
                         "lr0=%6, "
                         "workers=%7, "
                         "project='%8', "
                         "name='mask_train', "
                         "device='%9')"
                         )
                         .arg(weightPath)   // %1 权重
                         .arg(yamlPath)     // %2 yaml
                         .arg(m_epochsSpin->value())    // %3 epochs
                         .arg(m_imgSizeSpin->value())   // %4 imgsz
                         .arg(m_batchSpin->value())     // %5 batch
                         .arg(m_lrSpin->value())        // %6 lr0
                         .arg(m_workersSpin->value())   // %7 workers
                         .arg(outputPath)               // %8 project
                         .arg(device);                  // %9 device

    // 额外参数追加（格式: key=value 用分号分隔）
    QString extra = m_extraArgsEdit->text().trimmed();
    if (!extra.isEmpty())
        pyCode += "; " + extra;

    QStringList args;
    args << "-c" << pyCode;
    return args;
}

// ======================== 进程输出处理 ========================

void TrainWidget::onProcessOutput()
{
    QByteArray data = m_process->readAllStandardOutput();
    // 1. 改为 fromUtf8 强制 UTF-8 解码，解决乱码
    QString text = QString::fromUtf8(data);

    for (QString line : text.split('\n')) {
        // 去除进度条常用的回车符 '\r'，防止排版混乱
        line.remove('\r');
        line = line.trimmed();
        if (line.isEmpty()) continue;

        // 2. 剥离 ANSI 控制符提取纯文本，用于正则匹配进度 (避免控制符干扰正则)
        QString cleanLine = line;
        cleanLine.replace(QRegularExpression("\\x1B\\[[0-9;]*[a-zA-Z]"), "");

        // 解析 epoch 进度 (使用纯文本匹配)
        QRegularExpression rxEpoch("^\\s*(\\d+)/(\\d+)");
        QRegularExpressionMatch m = rxEpoch.match(cleanLine);
        if (m.hasMatch()) {
            int cur   = m.captured(1).toInt();
            int total = m.captured(2).toInt();
            if (total > 0 && cur >= 1 && cur <= total) {
                m_currentEpoch = cur;
                m_totalEpochs  = total;
                m_progressBar->setRange(0, total);
                m_progressBar->setValue(cur);
                m_progressBar->setFormat(
                    QString("Epoch %1 / %2  (%3%)")
                        .arg(cur).arg(total)
                        .arg(int(100.0 * cur / total)));
                m_statusLabel->setText(
                    QString("训练中... Epoch %1 / %2")
                        .arg(cur).arg(total));
            }
        }

        // 3. 将带有终端颜色的原始文本转换为 HTML 代码
        QString htmlLine = ansiToHtml(line);

        // 我们不再使用你之前写的硬编码颜色(关键词匹配)，而是直接依赖 YOLO 自带的颜色
        // 传入 true 告诉 appendLog 这是处理好的 HTML
        appendLog(htmlLine, "#e0e0e0", true);
    }
}



void TrainWidget::onProcessError()
{
    QByteArray data = m_process->readAllStandardError();
    // 同样使用 fromUtf8 解码
    QString text = QString::fromUtf8(data);

    for (QString line : text.split('\n')) {
        line.remove('\r');
        line = line.trimmed();
        if (line.isEmpty()) continue;

        QString cleanLine = line;
        cleanLine.replace(QRegularExpression("\\x1B\\[[0-9;]*[a-zA-Z]"), "");
        QString lower = cleanLine.toLower();

        // 错误流中的警告和报错我们依然可以强行覆盖颜色
        QString color = "#e0e0e0";
        if (lower.contains("warning") || lower.contains("warn"))
            color = "#ffaa44";
        else if (lower.contains("error") || lower.contains("exception")
                 || lower.contains("traceback"))
            color = "#ff5555";

        // 转换 HTML
        QString htmlLine = ansiToHtml(line);
        appendLog(htmlLine, color, true);
    }
}

void TrainWidget::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    setTrainingState(false);
    appendLog("═══════════════════════════════════════════", "#444444");

    if (status == QProcess::CrashExit) {
        appendLog("❌ 训练进程崩溃退出！", "#ff4444");
        m_statusLabel->setText("❌ 训练异常退出");
        m_statusLabel->setStyleSheet(
            "color:#ff4444;font-weight:bold;font-size:12px;");

    } else if (exitCode == 0) {
        // ★★★ 把模型路径提到最前面，整个if块都能用 ★★★
        QString bestModelPath = m_outputEdit->text() + "/mask_train/weights/best.pt";

        appendLog("✅ 训练完成！", "#44ff44");
        appendLog(QString("■ 结束时间: %1")
                      .arg(QDateTime::currentDateTime()
                               .toString("yyyy-MM-dd hh:mm:ss")), "#888888");
        appendLog("■ 最优权重: " + bestModelPath, "#ffd700");

        // ★★★ 新增：保存到数据库 ★★★
        Database::instance().saveTrainRecord(
            bestModelPath,
            m_yamlEdit->text(),
            m_epochsSpin->value(),
            m_batchSpin->value(),
            m_lrSpin->value(),
            0.0,
            "完成"
            );
        appendLog("📊 训练记录已保存到数据库", "#44aaff");
        // ★★★ 新增结束 ★★★

        m_statusLabel->setText("✅ 训练完成！");
        m_statusLabel->setStyleSheet(
            "color:#44ff44;font-weight:bold;font-size:12px;");
        m_progressBar->setValue(m_progressBar->maximum());
        m_progressBar->setFormat("训练完成 100%");

        QMessageBox::information(this, "训练完成",
                                 QString("✅ 训练已完成！\n\n最优权重保存在:\n%1")
                                     .arg(bestModelPath));

    } else {
        appendLog(QString("❌ 训练失败，退出码: %1").arg(exitCode), "#ff4444");
        appendLog("请检查上方日志中的错误信息", "#ff6666");
        m_statusLabel->setText(
            QString("❌ 训练失败 (code:%1)").arg(exitCode));
        m_statusLabel->setStyleSheet(
            "color:#ff4444;font-weight:bold;font-size:12px;");
    }

    appendLog("═══════════════════════════════════════════", "#444444");
}

// ======================== 辅助函数 ========================

void TrainWidget::appendLog(const QString& text, const QString& color, bool isHtml)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");

    // 如果是普通文本，进行 HTML 转义防止特殊字符破坏排版；如果是处理好的 HTML，则直接使用
    QString content = isHtml ? text : text.toHtmlEscaped();

    QString html = QString(
                       "<span style='color:#555;'>[%1]</span>&nbsp;"
                       "<span style='color:%2;font-family:Courier New;'>%3</span>")
                       .arg(timestamp)
                       .arg(color)
                       .arg(content);

    m_logEdit->append(html);

    QScrollBar* sb = m_logEdit->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void TrainWidget::setTrainingState(bool running)
{
    m_startBtn->setEnabled(!running);
    m_stopBtn->setEnabled(running);

    m_pythonEdit->setEnabled(!running);
    m_yamlEdit->setEnabled(!running);
    m_weightEdit->setEnabled(!running);
    m_outputEdit->setEnabled(!running);
    m_epochsSpin->setEnabled(!running);
    m_batchSpin->setEnabled(!running);
    m_imgSizeSpin->setEnabled(!running);
    m_lrSpin->setEnabled(!running);
    m_workersSpin->setEnabled(!running);
    m_gpuCheck->setEnabled(!running);

    if (running) {
        m_statusLabel->setStyleSheet(
            "color:#44ff44;font-weight:bold;font-size:12px;");
        m_statusLabel->setText("训练中...");
    } else {
        m_statusLabel->setStyleSheet("color:#aaa;font-size:12px;");
    }
}