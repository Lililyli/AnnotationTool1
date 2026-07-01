#include "dbviewer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSqlQuery>
#include <QHeaderView>
#include <QLabel>

DbViewer::DbViewer(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("📊 数据库记录查看");
    resize(900, 500);

    QVBoxLayout* layout = new QVBoxLayout(this);

    m_tabWidget = new QTabWidget(this);

    // ===== 训练记录表 =====
    m_trainTable = new QTableWidget;
    m_trainTable->setColumnCount(9);
    m_trainTable->setHorizontalHeaderLabels(
        {"ID", "模型路径", "数据集", "Epochs", "Batch", "学习率", "mAP50", "状态", "时间"}
        );
    m_trainTable->horizontalHeader()->setStretchLastSection(true);
    m_trainTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tabWidget->addTab(m_trainTable, "训练记录");

    // ===== 推理记录表 =====
    m_inferTable = new QTableWidget;
    m_inferTable->setColumnCount(6);
    m_inferTable->setHorizontalHeaderLabels(
        {"ID", "模型路径", "图片路径", "结果", "结果图", "时间"}
        );
    m_inferTable->horizontalHeader()->setStretchLastSection(true);
    m_inferTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tabWidget->addTab(m_inferTable, "推理记录");

    layout->addWidget(m_tabWidget);

    // ===== 按钮 =====
    QHBoxLayout* btnLayout = new QHBoxLayout;
    QPushButton* refreshBtn = new QPushButton("🔄 刷新");
    QPushButton* closeBtn = new QPushButton("关闭");
    refreshBtn->setStyleSheet("background:#3a6ea5;color:white;padding:6px 16px;border-radius:3px;");
    closeBtn->setStyleSheet("background:#666;color:white;padding:6px 16px;border-radius:3px;");

    btnLayout->addStretch();
    btnLayout->addWidget(refreshBtn);
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    connect(refreshBtn, &QPushButton::clicked, this, [this]() {
        refreshTrainTable();
        refreshInferTable();
    });
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    // 启动时加载数据
    refreshTrainTable();
    refreshInferTable();
}

void DbViewer::refreshTrainTable()
{
    m_trainTable->setRowCount(0);
    QSqlQuery query("SELECT id, model_path, dataset, epochs, batch_size, "
                    "learning_rate, map50, status, created_at "
                    "FROM train_records ORDER BY id DESC");

    int row = 0;
    while (query.next()) {
        m_trainTable->insertRow(row);
        for (int col = 0; col < 9; col++) {
            m_trainTable->setItem(row, col,
                                  new QTableWidgetItem(query.value(col).toString()));
        }
        row++;
    }
    m_trainTable->resizeColumnsToContents();
}

void DbViewer::refreshInferTable()
{
    m_inferTable->setRowCount(0);
    QSqlQuery query("SELECT id, model_path, image_path, result_text, "
                    "result_image_path, created_at "
                    "FROM infer_records ORDER BY id DESC");

    int row = 0;
    while (query.next()) {
        m_inferTable->insertRow(row);
        for (int col = 0; col < 6; col++) {
            m_inferTable->setItem(row, col,
                                  new QTableWidgetItem(query.value(col).toString()));
        }
        row++;
    }
    m_inferTable->resizeColumnsToContents();
}