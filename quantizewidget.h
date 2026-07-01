#ifndef QUANTIZEWIDGET_H
#define QUANTIZEWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QProgressBar>
#include <QTextEdit>
#include <QProcess>

class QuantizeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QuantizeWidget(QWidget *parent = nullptr);

signals:
    // 核心信号：量化完成后，发出这个信号，并携带量化后的模型路径
    void quantizeFinished(const QString& modelPath);

private slots:
    void onSelectInputModel();
    void onSelectOutputDir();
    void onStartQuantize();
    void onQuantizeProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onQuantizeProcessError();

private:
    void setupUI();

    QLineEdit* m_inputModelEdit;
    QLineEdit* m_outputDirEdit;
    QComboBox* m_methodCombo;
    QPushButton* m_startBtn;
    QProgressBar* m_progressBar;
    QTextEdit* m_logText;
    QProcess* m_process;

    // 用于模拟进度的变量
    int m_progressValue;
};

#endif // QUANTIZEWIDGET_H