#include "imageview.h"
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QFontMetrics>
#include <QtMath>
#include <QDebug>

ImageView::ImageView(QWidget* parent)
    : QWidget(parent)
    , m_nextId(1)
    , m_currentLabel("person")
    , m_labelManager(nullptr)
    , m_scale(1.0)
    , m_offset(0, 0)
    // 新增旋转
    , m_rotation(0.0)          // ← 新增这行
    , m_drawState(DrawState::None)
    , m_selectedId(-1)
    , m_resizeHandle(-1)
    , m_panning(false)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(400, 300);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

// ======================== 图像操作 ========================

void ImageView::loadImage(const QString& path)
{
    QPixmap px(path);
    if (px.isNull()) {
        emit statusMessage("无法加载图像: " + path);
        return;
    }
    m_pixmap    = px;
    m_imagePath = path;
    m_annotations.clear();
    m_selectedId = -1;
    m_nextId     = 1;
    fitToWindow();
    emit annotationChanged();
    emit statusMessage(QString("已加载: %1  [%2 x %3]")
                           .arg(path).arg(px.width()).arg(px.height()));
}

void ImageView::clearImage()
{
    m_pixmap = QPixmap();
    m_imagePath.clear();
    m_annotations.clear();
    m_selectedId = -1;
    update();
}

// ======================== 标注操作 ========================

void ImageView::setCurrentLabel(const QString& label)
{
    m_currentLabel = label;
}

void ImageView::deleteSelectedAnnotation()
{
    if (m_selectedId < 0) return;
    for (int i = 0; i < m_annotations.size(); i++) {
        if (m_annotations[i].id == m_selectedId) {
            m_annotations.removeAt(i);
            break;
        }
    }
    m_selectedId = -1;
    update();
    emit annotationChanged();
}

void ImageView::clearAnnotations()
{
    m_annotations.clear();
    m_selectedId = -1;
    m_nextId     = 1;
    update();
    emit annotationChanged();
}

void ImageView::setAnnotations(const QList<AnnotationItem>& anns)
{
    m_annotations = anns;
    m_selectedId  = -1;
    // 更新 nextId
    int maxId = 0;
    for (const auto& a : anns)
        if (a.id > maxId) maxId = a.id;
    m_nextId = maxId + 1;
    update();
    emit annotationChanged();
}

// ======================== 视图操作 ========================

void ImageView::zoomIn()
{
    if (m_pixmap.isNull()) return;
    m_scale *= 1.2;
    // 以窗口中心为缩放中心
    QPointF center(width() / 2.0, height() / 2.0);
    QPointF imgCenter = widgetToImage(center.toPoint());
    m_scale = qMin(m_scale, 20.0);
    m_offset = center - QPointF(imgCenter.x() * m_scale, imgCenter.y() * m_scale);
    update();
}

void ImageView::zoomOut()
{
    if (m_pixmap.isNull()) return;
    m_scale /= 1.2;
    QPointF center(width() / 2.0, height() / 2.0);
    QPointF imgCenter = widgetToImage(center.toPoint());
    m_scale = qMax(m_scale, 0.05);
    m_offset = center - QPointF(imgCenter.x() * m_scale, imgCenter.y() * m_scale);
    update();
}

void ImageView::fitToWindow()
{
    if (m_pixmap.isNull()) return;
    double scaleX = (double)width()  / m_pixmap.width();
    double scaleY = (double)height() / m_pixmap.height();
    m_scale  = qMin(scaleX, scaleY) * 0.95;
    m_offset = QPointF(
        (width()  - m_pixmap.width()  * m_scale) / 2.0,
        (height() - m_pixmap.height() * m_scale) / 2.0
        );
    update();
}

void ImageView::resetZoom()
{
    if (m_pixmap.isNull()) return;
    m_scale  = 1.0;
    m_offset = QPointF(
        (width()  - m_pixmap.width())  / 2.0,
        (height() - m_pixmap.height()) / 2.0
        );
    update();
}

// ======================== 坐标转换 ========================

QPointF ImageView::widgetToImage(const QPoint& pt) const
{
    return QPointF(
        (pt.x() - m_offset.x()) / m_scale,
        (pt.y() - m_offset.y()) / m_scale
        );
}

QPointF ImageView::imageToWidget(const QPointF& pt) const
{
    return QPointF(
        pt.x() * m_scale + m_offset.x(),
        pt.y() * m_scale + m_offset.y()
        );
}

QRectF ImageView::imageRectToWidget(const QRectF& r) const
{
    QPointF tl = imageToWidget(r.topLeft());
    QPointF br = imageToWidget(r.bottomRight());
    return QRectF(tl, br);
}

// ======================== 鼠标事件 ========================

void ImageView::mousePressEvent(QMouseEvent* event)
{
    if (m_pixmap.isNull()) return;
    setFocus();

    // 中键：平移
    if (event->button() == Qt::MiddleButton) {
        m_panning        = true;
        m_panStart       = event->pos();
        m_panStartOffset = m_offset;
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    // 右键：取消选中
    if (event->button() == Qt::RightButton) {
        m_selectedId = -1;
        update();
        return;
    }

    if (event->button() != Qt::LeftButton) return;

    m_pressPos   = event->pos();
    m_currentPos = event->pos();

    // 检查是否点击在已有标注上（移动 or 缩放）
    int clickedId = findAnnotationAt(event->pos());

    if (clickedId >= 0) {
        // 先检查缩放手柄
        if (m_selectedId == clickedId) {
            int handle = findHandleAt(event->pos(), m_selectedId);
            if (handle >= 0) {
                m_resizeHandle = handle;
                m_drawState    = DrawState::Resizing;
                for (const auto& a : m_annotations)
                    if (a.id == m_selectedId) {
                        m_moveStartRect = a.rect;
                        m_moveStartPos  = widgetToImage(event->pos());
                        break;
                    }
                return;
            }
        }

        // 选中 & 准备移动
        m_selectedId   = clickedId;
        m_drawState    = DrawState::Moving;
        m_moveStartPos = widgetToImage(event->pos());
        for (const auto& a : m_annotations)
            if (a.id == m_selectedId) {
                m_moveStartRect = a.rect;
                break;
            }
        setCursor(Qt::SizeAllCursor);
        emit annotationSelected(clickedId);
        update();
    } else {
        // 开始绘制新框
        m_selectedId = -1;
        m_drawState  = DrawState::Drawing;
        m_drawingRect = QRectF();
        setCursor(Qt::CrossCursor);
    }
}

void ImageView::mouseMoveEvent(QMouseEvent* event)
{
    m_currentPos = event->pos();

    if (m_panning) {
        QPointF delta = event->pos() - m_panStart;
        m_offset = m_panStartOffset + delta;
        update();
        return;
    }

    if (m_pixmap.isNull()) {
        update();
        return;
    }

    if (m_drawState == DrawState::Drawing) {
        QPointF imgStart = widgetToImage(m_pressPos);
        QPointF imgCur   = widgetToImage(event->pos());
        m_drawingRect = QRectF(imgStart, imgCur).normalized();

        // 限制在图像范围内
        m_drawingRect &= QRectF(0, 0, m_pixmap.width(), m_pixmap.height());
        update();

        emit statusMessage(QString("绘制中: [%1, %2, %3, %4]")
                               .arg((int)m_drawingRect.x())
                               .arg((int)m_drawingRect.y())
                               .arg((int)m_drawingRect.width())
                               .arg((int)m_drawingRect.height()));

    } else if (m_drawState == DrawState::Moving) {
        QPointF imgCur   = widgetToImage(event->pos());
        QPointF delta    = imgCur - m_moveStartPos;

        for (auto& a : m_annotations) {
            if (a.id == m_selectedId) {
                QRectF newRect = m_moveStartRect.translated(delta);
                // 限制在图像范围内
                double iw = m_pixmap.width(), ih = m_pixmap.height();
                newRect.moveLeft  (qMax(0.0, qMin(newRect.left(),   iw - newRect.width())));
                newRect.moveTop   (qMax(0.0, qMin(newRect.top(),    ih - newRect.height())));
                a.rect = newRect;
                break;
            }
        }
        update();

    } else if (m_drawState == DrawState::Resizing) {
        QPointF imgCur = widgetToImage(event->pos());
        QPointF delta  = imgCur - m_moveStartPos;

        for (auto& a : m_annotations) {
            if (a.id == m_selectedId) {
                QRectF r = m_moveStartRect;
                double iw = m_pixmap.width(), ih = m_pixmap.height();

                // handle 0-7: TL,T,TR,R,BR,B,BL,L
                switch (m_resizeHandle) {
                case 0: r.setTopLeft    (r.topLeft()     + delta); break;
                case 1: r.setTop        (r.top()         + delta.y()); break;
                case 2: r.setTopRight   (r.topRight()    + delta); break;
                case 3: r.setRight      (r.right()       + delta.x()); break;
                case 4: r.setBottomRight(r.bottomRight() + delta); break;
                case 5: r.setBottom     (r.bottom()      + delta.y()); break;
                case 6: r.setBottomLeft (r.bottomLeft()  + delta); break;
                case 7: r.setLeft       (r.left()        + delta.x()); break;
                }
                r = r.normalized();
                // 限制范围
                r.setLeft  (qMax(0.0, r.left()));
                r.setTop   (qMax(0.0, r.top()));
                r.setRight (qMin(iw,  r.right()));
                r.setBottom(qMin(ih,  r.bottom()));
                if (r.width() > MIN_RECT_SIZE && r.height() > MIN_RECT_SIZE)
                    a.rect = r;
                break;
            }
        }
        update();

    } else {
        // 光标提示
        int hoverAnn = findAnnotationAt(event->pos());
        if (hoverAnn >= 0) {
            if (m_selectedId == hoverAnn && findHandleAt(event->pos(), hoverAnn) >= 0)
                setCursor(Qt::SizeFDiagCursor);
            else
                setCursor(Qt::SizeAllCursor);
        } else {
            setCursor(Qt::CrossCursor);
        }
        update(); // 绘制十字线
    }
}

void ImageView::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_panning && event->button() == Qt::MiddleButton) {
        m_panning = false;
        setCursor(Qt::CrossCursor);
        return;
    }

    if (event->button() != Qt::LeftButton) return;

    if (m_drawState == DrawState::Drawing) {
        if (!m_drawingRect.isNull() &&
            m_drawingRect.width()  > MIN_RECT_SIZE &&
            m_drawingRect.height() > MIN_RECT_SIZE)
        {
            QColor color = m_labelManager
                               ? m_labelManager->getColor(m_currentLabel)
                               : Qt::red;
            AnnotationItem item(m_nextId++, m_currentLabel, m_drawingRect, color);
            m_annotations.append(item);
            m_selectedId = item.id;
            emit annotationChanged();
            emit statusMessage(QString("新增标注 [%1]: %2")
                                   .arg(item.label)
                                   .arg(item.id));
        }
        m_drawingRect = QRectF();

    } else if (m_drawState == DrawState::Moving ||
               m_drawState == DrawState::Resizing) {
        emit annotationChanged();
    }

    m_drawState    = DrawState::None;
    m_resizeHandle = -1;
    setCursor(Qt::CrossCursor);
    update();
}

void ImageView::wheelEvent(QWheelEvent* event)
{
    if (m_pixmap.isNull()) return;

    // 以鼠标为中心缩放
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QPoint mousePos = event->position().toPoint();
#else
    QPoint mousePos = event->pos();
#endif
    QPointF imgPos = widgetToImage(mousePos);

    double factor = (event->angleDelta().y() > 0) ? 1.15 : (1.0 / 1.15);
    m_scale = qBound(0.05, m_scale * factor, 20.0);

    m_offset = QPointF(
        mousePos.x() - imgPos.x() * m_scale,
        mousePos.y() - imgPos.y() * m_scale
        );
    update();
    event->accept();
}

void ImageView::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        deleteSelectedAnnotation();
    } else if (event->key() == Qt::Key_Escape) {
        m_selectedId  = -1;
        m_drawState   = DrawState::None;
        m_drawingRect = QRectF();
        update();
    }
}

void ImageView::resizeEvent(QResizeEvent*)
{
    if (!m_pixmap.isNull())
        fitToWindow();
}

// ======================== 绘制 ========================

void ImageView::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 背景
    painter.fillRect(rect(), QColor(40, 40, 40));

    if (m_pixmap.isNull()) {
        painter.setPen(Qt::gray);
        painter.setFont(QFont("Arial", 16));
        painter.drawText(rect(), Qt::AlignCenter, "请打开图像文件\n(File → Open Image)");
        return;
    }

    // 绘制图像（支持旋转）
    painter.save();  // 保存当前画笔状态

    // 以图片显示区域中心为旋转中心
    QPointF center(
        m_offset.x() + m_pixmap.width()  * m_scale / 2.0,
        m_offset.y() + m_pixmap.height() * m_scale / 2.0
        );
    painter.translate(center);   // 移动到旋转中心
    painter.rotate(m_rotation);  // 旋转
    painter.translate(-center);  // 移回来

    QRectF dstRect(m_offset.x(), m_offset.y(),
                   m_pixmap.width()  * m_scale,
                   m_pixmap.height() * m_scale);
    painter.drawPixmap(dstRect.toRect(), m_pixmap);

    painter.restore();  // 恢复画笔状态（标注框不受旋转影响）

    // 绘制标注
    drawAnnotations(painter);

    // 绘制正在拖拽的矩形
    drawCurrentRect(painter);

    // 绘制十字准线（非绘制状态下）
    if (m_drawState == DrawState::None) {
        drawCrosshair(painter, m_currentPos);
    }
}

void ImageView::drawAnnotations(QPainter& painter)
{
    for (const auto& ann : m_annotations) {
        QRectF wr = imageRectToWidget(ann.rect);
        bool selected = (ann.id == m_selectedId);

        // 半透明填充
        QColor fillColor = ann.color;
        fillColor.setAlpha(selected ? 60 : 30);
        painter.fillRect(wr, fillColor);

        // 边框
        QPen pen(ann.color, selected ? 2.5 : 1.5);
        if (selected) pen.setStyle(Qt::SolidLine);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(wr);

        // 标签背景
        QString labelText = QString("#%1 %2").arg(ann.id).arg(ann.label);
        QFont   font("Arial", 9, QFont::Bold);
        painter.setFont(font);
        QFontMetrics fm(font);
        QRect textRect = fm.boundingRect(labelText);
        textRect.adjust(-3, -2, 3, 2);

        // 标签位置（矩形上方）
        QPoint textPos(wr.left(), wr.top() - 2);
        if (textPos.y() < textRect.height())
            textPos.setY(wr.top() + textRect.height() + 2);

        QRect bgRect(textPos.x(), textPos.y() - textRect.height(),
                     textRect.width(), textRect.height());

        painter.fillRect(bgRect, ann.color);
        painter.setPen(Qt::white);
        painter.drawText(bgRect, Qt::AlignCenter, labelText);

        // 绘制手柄（仅选中时）
        if (selected) {
            QList<QPointF> handles = {
                wr.topLeft(),
                QPointF(wr.center().x(), wr.top()),
                wr.topRight(),
                QPointF(wr.right(), wr.center().y()),
                wr.bottomRight(),
                QPointF(wr.center().x(), wr.bottom()),
                wr.bottomLeft(),
                QPointF(wr.left(), wr.center().y())
            };
            painter.setPen(QPen(Qt::white, 1));
            painter.setBrush(ann.color);
            for (const QPointF& h : handles) {
                painter.drawRect(QRectF(h.x() - HANDLE_SIZE/2, h.y() - HANDLE_SIZE/2,
                                        HANDLE_SIZE, HANDLE_SIZE));
            }
        }
    }
}

void ImageView::drawCurrentRect(QPainter& painter)
{
    if (m_drawState != DrawState::Drawing || m_drawingRect.isNull()) return;

    QRectF wr = imageRectToWidget(m_drawingRect);
    QColor color = m_labelManager
                       ? m_labelManager->getColor(m_currentLabel)
                       : Qt::yellow;

    QColor fill = color;
    fill.setAlpha(40);
    painter.fillRect(wr, fill);

    QPen pen(color, 2, Qt::DashLine);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(wr);

    // 尺寸提示
    QString sizeStr = QString("%1 x %2")
                          .arg((int)m_drawingRect.width())
                          .arg((int)m_drawingRect.height());
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 8));
    painter.drawText(wr.bottomRight() + QPointF(4, 0), sizeStr);
}

void ImageView::drawCrosshair(QPainter& painter, const QPoint& pos)
{
    if (!rect().contains(pos)) return;

    // 检查是否在图像范围内
    QPointF imgPos = widgetToImage(pos);
    if (imgPos.x() < 0 || imgPos.x() > m_pixmap.width() ||
        imgPos.y() < 0 || imgPos.y() > m_pixmap.height())
        return;

    QPen pen(QColor(200, 200, 200, 80), 1, Qt::DotLine);
    painter.setPen(pen);
    painter.drawLine(0, pos.y(), width(), pos.y());
    painter.drawLine(pos.x(), 0, pos.x(), height());

    // 坐标显示
    painter.setPen(QColor(255, 255, 100));
    painter.setFont(QFont("Arial", 8));
    painter.drawText(pos + QPoint(5, -5),
                     QString("(%1, %2)")
                         .arg((int)imgPos.x())
                         .arg((int)imgPos.y()));
}

// ======================== 辅助 ========================

int ImageView::findAnnotationAt(const QPoint& widgetPos) const
{
    // 逆序查找（后绘制的优先）
    for (int i = m_annotations.size() - 1; i >= 0; i--) {
        QRectF wr = imageRectToWidget(m_annotations[i].rect);
        if (wr.contains(widgetPos))
            return m_annotations[i].id;
    }
    return -1;
}

int ImageView::findHandleAt(const QPoint& widgetPos, int annId) const
{
    for (const auto& ann : m_annotations) {
        if (ann.id != annId) continue;
        QRectF wr = imageRectToWidget(ann.rect);
        QList<QPointF> handles = {
            wr.topLeft(),
            QPointF(wr.center().x(), wr.top()),
            wr.topRight(),
            QPointF(wr.right(), wr.center().y()),
            wr.bottomRight(),
            QPointF(wr.center().x(), wr.bottom()),
            wr.bottomLeft(),
            QPointF(wr.left(), wr.center().y())
        };
        for (int i = 0; i < handles.size(); i++) {
            QRectF hr(handles[i].x() - HANDLE_SIZE, handles[i].y() - HANDLE_SIZE,
                      HANDLE_SIZE * 2, HANDLE_SIZE * 2);
            if (hr.contains(widgetPos)) return i;
        }
        break;
    }
    return -1;
}

void ImageView::rotateClockwise()
{
    if (m_pixmap.isNull()) return;
    m_rotation += 90.0;
    if (m_rotation >= 360.0) m_rotation = 0.0;
    fitToWindow();  // 旋转后重新适应窗口
    update();
}

void ImageView::rotateCounterClockwise()
{
    if (m_pixmap.isNull()) return;
    m_rotation -= 90.0;
    if (m_rotation < 0.0) m_rotation += 360.0;
    fitToWindow();
    update();
}