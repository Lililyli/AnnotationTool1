#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QComboBox>
#include <QLabel>
#include <QToolBar>
#include <QDockWidget>
#include <QStringList>
#include "imageview.h"
#include "labelmanager.h"
#include "trainwidget.h"
#include "quantizewidget.h"
#include "inferwidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void openImage();
    void openFolder();
    void prevImage();
    void nextImage();
    void saveAnnotations();
    void loadAnnotations();

    void onAnnotationChanged();
    void onAnnotationSelected(int id);
    void onLabelSelected(int row);
    void deleteSelectedAnnotation();
    void clearAnnotations();

    void addLabel();
    void removeLabel();
    void onLabelComboChanged(int index);

    void zoomIn()      { m_imageView->zoomIn(); }
    void zoomOut()     { m_imageView->zoomOut(); }
    void fitToWindow() { m_imageView->fitToWindow(); }
    void resetZoom()   { m_imageView->resetZoom(); }
    void rotateClockwise()        { m_imageView->rotateClockwise(); }        // ← 新增
    void rotateCounterClockwise() { m_imageView->rotateCounterClockwise(); } // ← 新增
    void showStatusMsg(const QString& msg);

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupDockWidgets();
    void setupStatusBar();
    void setupShortcuts();

    void loadImageFile(const QString& path);
    void updateAnnotationList();
    void updateImageList();
    void updateWindowTitle();

    void saveYOLO(const QString& path);
    void saveVOC(const QString& path);
    void loadYOLO(const QString& path);
    void loadVOC(const QString& path);

    QuantizeWidget *m_quantizeWidget;
    InferWidget    *m_inferWidget;

    // 核心组件
    ImageView*   m_imageView;
    LabelManager m_labelManager;

    // 面板
    QDockWidget* m_fileDock;       // ← 新增
    QDockWidget* m_annDock;        // ← 新增

    // 左侧文件列表
    QListWidget* m_fileList;

    // 右侧标注面板
    QListWidget* m_annotationList;
    QComboBox*   m_labelCombo;
    QLabel*      m_imageInfoLabel;

    // 图像文件列表
    QStringList  m_imageFiles;
    int          m_currentImageIndex;

    // 状态栏
    QLabel*      m_statusLabel;
    QLabel*      m_coordLabel;
    QLabel*      m_countLabel;

    // 路径
    QString      m_currentFolder;
    QString      m_saveFolder;     // ← 新增
};

#endif // MAINWINDOW_H