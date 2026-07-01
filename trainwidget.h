#ifndef TRAINWIDGET_H
#define TRAINWIDGET_H

#include <QWidget>
#include <QProcess>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QTextEdit>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>

class TrainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TrainWidget(QWidget* parent = nullptr);
    ~TrainWidget();

private slots:
    void selectPythonPath();
    void selectYamlFile();
    void selectWeightFile();
    void selectOutputFolder();
    void startTraining();
    void stopTraining();
    void onProcessOutput();
    void onProcessError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    void setupUI();
    void appendLog(const QString& text, const QString& color = "#e0e0e0", bool isHtml = false);
    void setTrainingState(bool running);
    QStringList buildArgs();

    // 进程
    QProcess* m_process;

    // 路径
    QLineEdit* m_pythonEdit;     // Python解释器路径
    QLineEdit* m_yamlEdit;       // data.yaml路径
    QLineEdit* m_weightEdit;     // 权重文件路径
    QLineEdit* m_outputEdit;     // 输出路径

    // 训练参数
    QSpinBox*       m_epochsSpin;
    QSpinBox*       m_batchSpin;
    QSpinBox*       m_imgSizeSpin;
    QDoubleSpinBox* m_lrSpin;
    QSpinBox*       m_workersSpin;
    QCheckBox*      m_gpuCheck;
    QLineEdit*      m_extraArgsEdit;

    // 控制按钮
    QPushButton* m_startBtn;
    QPushButton* m_stopBtn;

    // 状态显示
    QLabel*       m_statusLabel;
    QProgressBar* m_progressBar;
    QTextEdit*    m_logEdit;

    // 内部状态
    int m_totalEpochs;
    int m_currentEpoch;
};

#endif // TRAINWIDGET_H