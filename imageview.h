#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H

#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QList>
#include "annotationitem.h"
#include "labelmanager.h"

class ImageView : public QWidget
{
    Q_OBJECT

public:
    explicit ImageView(QWidget* parent = nullptr);

    // 图像操作
    void loadImage(const QString& path);
    void clearImage();
    bool hasImage() const { return !m_pixmap.isNull(); }

    // 标注操作
    void setCurrentLabel(const QString& label);
    void deleteSelectedAnnotation();
    void clearAnnotations();
    QList<AnnotationItem> getAnnotations() const { return m_annotations; }
    void setAnnotations(const QList<AnnotationItem>& anns);

    // 视图操作
    void zoomIn();
    void zoomOut();
    void fitToWindow();
    void resetZoom();
    // 新增旋转功能
    void rotateClockwise();         // ← 新增这行
    void rotateCounterClockwise();  // ← 新增这行

    // 标签管理器
    void setLabelManager(LabelManager* mgr) { m_labelManager = mgr; }

    QString currentImagePath() const { return m_imagePath; }

signals:
    void annotationChanged();           // 标注发生变化
    void annotationSelected(int id);    // 选中某个标注
    void statusMessage(const QString& msg);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    // 坐标转换
    QPointF widgetToImage(const QPoint& pt) const;
    QPointF imageToWidget(const QPointF& pt) const;
    QRectF  imageRectToWidget(const QRectF& r) const;

    // 绘制
    void drawAnnotations(QPainter& painter);
    void drawCurrentRect(QPainter& painter);
    void drawCrosshair(QPainter& painter, const QPoint& pos);

    // 查找标注
    int findAnnotationAt(const QPoint& widgetPos) const;
    int findHandleAt(const QPoint& widgetPos, int annId) const;

    // 内部数据
    QPixmap              m_pixmap;
    QString              m_imagePath;
    QList<AnnotationItem> m_annotations;
    int                  m_nextId;
    QString              m_currentLabel;
    LabelManager*        m_labelManager;

    // 视图变换
    double  m_scale;
    QPointF m_offset;       // 图像左上角在widget中的坐标
    // 新增旋转功能
    double  m_rotation;     // ← 新增这行，旋转角度（0/90/180/270）

    // 拖拽绘制
    enum class DrawState { None, Drawing, Moving, Resizing };
    DrawState m_drawState;
    QPoint    m_pressPos;       // 按下时widget坐标
    QPoint    m_currentPos;     // 当前widget坐标
    QRectF    m_drawingRect;    // 正在绘制的矩形（图像坐标）

    // 选中 & 移动
    int     m_selectedId;
    QRectF  m_moveStartRect;    // 移动前的矩形
    QPointF m_moveStartPos;     // 移动开始的图像坐标
    int     m_resizeHandle;     // 0-7: 8个缩放手柄

    // 平移图像（中键）
    bool    m_panning;
    QPoint  m_panStart;
    QPointF m_panStartOffset;

    static const int HANDLE_SIZE = 8;
    static const int MIN_RECT_SIZE = 5;
};

#endif // IMAGEVIEW_H