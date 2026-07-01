#include "inferwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QGroupBox>
#include <QPixmap>
#include <QPainter>
#include <QPen>
#include "database.h"

InferWidget::InferWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void InferWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // --- 输入设置区域 ---
    QGroupBox* inputGroup = new QGroupBox("推理设置", this);
    QVBoxLayout* inputLayout = new QVBoxLayout(inputGroup);

    // 模型选择
    QHBoxLayout* modelLayout = new QHBoxLayout();
    m_modelPathEdit = new QLineEdit();
    m_modelPathEdit->setPlaceholderText("等待输入模型路径...");
    QPushButton* modelBtn = new QPushButton("浏览...");
    modelLayout->addWidget(new QLabel("加载模型:"));
    modelLayout->addWidget(m_modelPathEdit);
    modelLayout->addWidget(modelBtn);

    // 图像选择
    QHBoxLayout* imgLayout = new QHBoxLayout();
    m_imagePathEdit = new QLineEdit();
    m_imagePathEdit->setPlaceholderText("请选择要推理的测试图像...");
    QPushButton* imgBtn = new QPushButton("浏览...");
    imgLayout->addWidget(new QLabel("测试图像:"));
    imgLayout->addWidget(m_imagePathEdit);
    imgLayout->addWidget(imgBtn);

    inputLayout->addLayout(modelLayout);
    inputLayout->addLayout(imgLayout);
    mainLayout->addWidget(inputGroup);

    // --- 执行按钮 ---
    m_runBtn = new QPushButton("运行推理", this);
    m_runBtn->setMinimumHeight(40);
    m_runBtn->setStyleSheet("background-color: #3a6ea5; color: white; font-weight: bold;");
    mainLayout->addWidget(m_runBtn);

    // --- 结果显示区域 ---
    m_resultImageLabel = new QLabel("推理结果图像区", this);
    m_resultImageLabel->setAlignment(Qt::AlignCenter);
    m_resultImageLabel->setStyleSheet("background-color: #222; color: #888;");
    m_resultImageLabel->setMinimumHeight(200);

    m_resultLog = new QTextEdit(this);
    m_resultLog->setReadOnly(true);
    m_resultLog->setMaximumHeight(150);

    mainLayout->addWidget(m_resultImageLabel, 1); // 图像占较大比例
    mainLayout->addWidget(new QLabel("推理结果数据:"));
    mainLayout->addWidget(m_resultLog);

    // --- 绑定信号槽 ---
    connect(modelBtn, &QPushButton::clicked, this, &InferWidget::onSelectModel);
    connect(imgBtn, &QPushButton::clicked, this, &InferWidget::onSelectImage);
    connect(m_runBtn, &QPushButton::clicked, this, &InferWidget::onRunInference);
}

// 供外部调用的槽函数
void InferWidget::setModelPath(const QString& path)
{
    m_modelPathEdit->setText(path);
    m_resultLog->append("<font color='orange'>已自动接收量化后的模型：</font><br>" + path);
}

void InferWidget::onSelectModel()
{
    QString path = QFileDialog::getOpenFileName(this, "选择模型");
    if (!path.isEmpty()) m_modelPathEdit->setText(path);
}

void InferWidget::onSelectImage()
{
    QString path = QFileDialog::getOpenFileName(this, "选择测试图像", "", "Images (*.png *.jpg *.jpeg *.bmp)");
    if (!path.isEmpty()) m_imagePathEdit->setText(path);
}

#include <QProcess>
#include <QDir>
#include <QPixmap>

void InferWidget::onRunInference()
{
    QString imgPath = m_imagePathEdit->text();
    QString modelPath = m_modelPathEdit->text();

    if (imgPath.isEmpty() || modelPath.isEmpty()) return;

    m_resultLog->append("正在呼叫 Python 后台进行推理...");

    // 1. 准备参数
    QString pyScript = QDir::currentPath() + "/infer.py"; // 假设 python 脚本在当前目录
    QString outputPath = QDir::currentPath() + "/result_drawn.jpg"; // 设定输出图片路径

    QStringList arguments;
    arguments << pyScript << modelPath << imgPath << outputPath;

    // 2. 启动 Python 进程
    QProcess process;
    // 注意：这里的 "python" 取决于你电脑上的环境变量，如果是 conda 环境可能是 "python3" 或者环境的具体路径
    process.start("python", arguments);

    // 3. 等待 Python 脚本执行完毕 (设置超时时间，比如 30秒)
    if (!process.waitForFinished(30000)) {
        m_resultLog->append("<font color='red'>推理超时或 Python 脚本运行失败！</font>");
        return;
    }

    // 4. 读取 Python 脚本 print 出来的文字信息
    QString pythonOutput = process.readAllStandardOutput();
    QString pythonError = process.readAllStandardError();

    if (!pythonError.isEmpty()) {
        m_resultLog->append("<font color='red'>Python 报错:</font><br>" + pythonError);
    }

    // 把 Python 检测到的结果追加到界面的日志框里
    m_resultLog->append("<font color='green'>Python 结果:</font><br>" + pythonOutput);

    // 5. 显示 Python 已经画好框并保存的新图片！
    QPixmap resultImg(outputPath);
    if (!resultImg.isNull()) {
        QPixmap scaledPixmap = resultImg.scaled(m_resultImageLabel->size(),
                                                Qt::KeepAspectRatio,
                                                Qt::SmoothTransformation);
        m_resultImageLabel->setPixmap(scaledPixmap);
    } else {
        m_resultLog->append("<font color='red'>无法加载 Python 生成的结果图！</font>");
    }
    // ★★★ 新增：保存推理记录到数据库 ★★★
    Database::instance().saveInferRecord(
        modelPath,           // 模型路径
        imgPath,             // 输入图片
        pythonOutput,        // Python输出的结果文字
        outputPath           // 结果图片路径
        );
    m_resultLog->append("<font color='#44aaff'>📊 推理记录已保存到数据库</font>");
    // ★★★ 新增结束 ★★★

}