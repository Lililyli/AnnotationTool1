#ifndef ANNOTATIONITEM_H
#define ANNOTATIONITEM_H

#include <QString>
#include <QRectF>
#include <QColor>

// 单个标注框数据结构
struct AnnotationItem {
    int     id;          // 唯一ID
    QString label;       // 类别名称
    QRectF  rect;        // 图像坐标系中的矩形（像素）
    QColor  color;       // 显示颜色
    bool    selected;    // 是否被选中

    AnnotationItem()
        : id(-1), label("object"), color(Qt::red), selected(false) {}

    AnnotationItem(int id_, const QString& lbl, const QRectF& r, const QColor& c)
        : id(id_), label(lbl), rect(r), color(c), selected(false) {}
};

#endif // ANNOTATIONITEM_H