#include "quantizewidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QGroupBox>
#include <QTimer>
#include <QFileInfo>
#include <QProcess>
QuantizeWidget::QuantizeWidget(QWidget *parent)
    : QWidget(parent), m_progressValue(0), m_process(nullptr)
{
    setupUI();
}

void QuantizeWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // --- 设置区域 ---
    QGroupBox* settingGroup = new QGroupBox("量化设置", this);
    QVBoxLayout* settingLayout = new QVBoxLayout(settingGroup);

    // 输入模型
    QHBoxLayout* inputLayout = new QHBoxLayout();
    m_inputModelEdit = new QLineEdit();
    m_inputModelEdit->setPlaceholderText("请选择要量化的模型 (如 .onnx, .pt)");
    QPushButton* inputBtn = new QPushButton("浏览...");
    inputLayout->addWidget(new QLabel("输入模型:"));
    inputLayout->addWidget(m_inputModelEdit);
    inputLayout->addWidget(inputBtn);

    // 输出目录
    QHBoxLayout* outputLayout = new QHBoxLayout();
    m_outputDirEdit = new QLineEdit();
    m_outputDirEdit->setPlaceholderText("请选择量化后模型的保存目录");
    QPushButton* outputBtn = new QPushButton("浏览...");
    outputLayout->addWidget(new QLabel("输出目录:"));
    outputLayout->addWidget(m_outputDirEdit);
    outputLayout->addWidget(outputBtn);

    // 量化方式
    QHBoxLayout* methodLayout = new QHBoxLayout();
    m_methodCombo = new QComboBox();
    m_methodCombo->addItems({"INT8 (推荐)", "FP16", "INT4"});
    methodLayout->addWidget(new QLabel("量化精度:"));
    methodLayout->addWidget(m_methodCombo);
    methodLayout->addStretch();

    settingLayout->addLayout(inputLayout);
    settingLayout->addLayout(outputLayout);
    settingLayout->addLayout(methodLayout);
    mainLayout->addWidget(settingGroup);

    // --- 操作与状态区域 ---
    m_startBtn = new QPushButton("开始量化", this);
    m_startBtn->setMinimumHeight(40);
    m_startBtn->setStyleSheet("background-color: #3a7a3a; color: white; font-weight: bold;");

    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);

    m_logText = new QTextEdit();
    m_logText->setReadOnly(true);

    mainLayout->addWidget(m_startBtn);
    mainLayout->addWidget(m_progressBar);
    mainLayout->addWidget(new QLabel("日志输出:"));
    mainLayout->addWidget(m_logText);

    // --- 绑定信号与槽 ---
    connect(inputBtn, &QPushButton::clicked, this, &QuantizeWidget::onSelectInputModel);
    connect(outputBtn, &QPushButton::clicked, this, &QuantizeWidget::onSelectOutputDir);
    connect(m_startBtn, &QPushButton::clicked, this, &QuantizeWidget::onStartQuantize);
}

void QuantizeWidget::onSelectInputModel()
{
    QString path = QFileDialog::getOpenFileName(this, "选择模型文件");
    if (!path.isEmpty()) {
        m_inputModelEdit->setText(path);
    }
}

void QuantizeWidget::onSelectOutputDir()
{
    QString path = QFileDialog::getExistingDirectory(this, "选择保存目录");
    if (!path.isEmpty()) {
        m_outputDirEdit->setText(path);
    }
}

void QuantizeWidget::onStartQuantize()
{
    if (m_inputModelEdit->text().isEmpty()) {
        m_logText->append("<font color='red'>错误：请先选择输入模型！</font>");
        return;
    }

    m_startBtn->setEnabled(false);
    m_progressBar->setValue(0);
    m_logText->append("<b>开始模型量化任务...</b>");
    m_logText->append("加载模型: " + m_inputModelEdit->text());

    // 获取输出目录（如果未设置，使用输入模型所在目录）
    QString outputDir = m_outputDirEdit->text();
    if (outputDir.isEmpty()) {
        QFileInfo fi(m_inputModelEdit->text());
        outputDir = fi.absolutePath();
    }

    // 获取量化精度
    QString method = m_methodCombo->currentText();  // 例如 "INT8 (推荐)", "FP16", "INT4"

    // 准备调用 Python 脚本
    // 建议使用 python 的绝对路径，例如 "C:/Python39/python.exe"，如果环境变量有 python 也可以直接写 "python"
    QString pythonExe = "D:/software/anaconda/envs/work2_env/python.exe";  // 如果报错找不到，改成绝对路径，如 "C:/Users/你的用户名/AppData/Local/Programs/Python/Python39/python.exe"
    QString scriptPath = "D:/software/projectQT/AnnotationTool/quantize.py";
    QStringList args;
    args << scriptPath << m_inputModelEdit->text() << outputDir << method;

    // 创建 QProcess 并启动
    m_process = new QProcess(this);
    connect(m_process, &QProcess::finished, this, &QuantizeWidget::onQuantizeProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &QuantizeWidget::onQuantizeProcessError);

    // 可选：实时读取输出并显示到日志
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this](){
        QString output = m_process->readAllStandardOutput();
        m_logText->append(output);
    });
    connect(m_process, &QProcess::readyReadStandardError, this, [this](){
        QString err = m_process->readAllStandardError();
        m_logText->append("<font color='red'>" + err + "</font>");
    });

    m_process->setProcessChannelMode(QProcess::MergedChannels);
    m_process->start(pythonExe, args);

    m_logText->append("正在调用 Python 后台进行量化...");
}


void QuantizeWidget::onQuantizeProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    if (status == QProcess::NormalExit && exitCode == 0) {
        // Python 脚本最后会打印一行 "Success: 完整路径"
        // 因为我们已经连接了 readyReadStandardOutput，日志中已经显示了脚本的所有输出
        // 这里需要从脚本输出中提取模型路径。最简单的方法：再读一次所有输出（注意可能会重复，但无大碍）
        QString output = m_process->readAllStandardOutput();
        // 也可以不重复读，而是在 readyReadStandardOutput 中累积到一个成员变量里。

        // 解析 "Success:" 行
        QStringList lines = output.split('\n');
        QString modelPath;
        for (const QString& line : lines) {
            if (line.startsWith("Success:")) {
                modelPath = line.mid(8).trimmed();
                break;
            }
        }

        // 如果上述解析没找到，尝试从日志控件中获取最后一行（备用）
        if (modelPath.isEmpty()) {
            QString logText = m_logText->toPlainText();
            QStringList logLines = logText.split('\n');
            for (int i = logLines.size()-1; i >= 0; --i) {
                if (logLines[i].startsWith("Success:")) {
                    modelPath = logLines[i].mid(8).trimmed();
                    break;
                }
            }
        }

        if (!modelPath.isEmpty() && QFile::exists(modelPath)) {
            m_logText->append("<font color='green'>量化完成！模型已保存至: " + modelPath + "</font>");
            m_progressBar->setValue(100);
            emit quantizeFinished(modelPath);
        } else {
            m_logText->append("<font color='red'>错误：未能找到生成的模型文件！</font>");
        }
    } else {
        m_logText->append("<font color='red'>量化进程异常退出，请检查 Python 环境或脚本。</font>");
        QString err = m_process->readAllStandardError();
        if (!err.isEmpty())
            m_logText->append(err);
    }

    m_startBtn->setEnabled(true);
    m_process->deleteLater();
    m_process = nullptr;
}

void QuantizeWidget::onQuantizeProcessError()
{
    m_logText->append("<font color='red'>启动 Python 进程失败: " + m_process->errorString() + "</font>");
    m_startBtn->setEnabled(true);
    m_process->deleteLater();
    m_process = nullptr;
}