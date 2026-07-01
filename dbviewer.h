#ifndef DBVIEWER_H
#define DBVIEWER_H

#include <QDialog>
#include <QTableWidget>
#include <QTabWidget>
#include <QPushButton>

class DbViewer : public QDialog
{
    Q_OBJECT
public:
    explicit DbViewer(QWidget* parent = nullptr);

private slots:
    void refreshTrainTable();
    void refreshInferTable();

private:
    QTabWidget*   m_tabWidget;
    QTableWidget* m_trainTable;
    QTableWidget* m_inferTable;
};

#endif // DBVIEWER_H