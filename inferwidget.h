#ifndef INFERWIDGET_H
#define INFERWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>

class InferWidget : public QWidget
{
    Q_OBJECT
public:
    explicit InferWidget(QWidget *parent = nullptr);

public slots:
    // 核心槽函数：接收从其他页面（量化页面）传来的模型路径
    void setModelPath(const QString& path);

private slots:
    void onSelectModel();
    void onSelectImage();
    void onRunInference();

private:
    void setupUI();

    QLineEdit* m_modelPathEdit;
    QLineEdit* m_imagePathEdit;
    QPushButton* m_runBtn;
    QLabel* m_resultImageLabel;
    QTextEdit* m_resultLog;
};

#endif // INFERWIDGET_H