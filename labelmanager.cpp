#include "labelmanager.h"
#include <QDebug>

// 预设颜色列表
static const QColor PRESET_COLORS[] = {
    QColor(255,  85,  85),  // 红
    QColor( 85, 170, 255),  // 蓝
    QColor( 85, 255,  85),  // 绿
    QColor(255, 170,  85),  // 橙
    QColor(170,  85, 255),  // 紫
    QColor( 85, 255, 255),  // 青
    QColor(255, 255,  85),  // 黄
    QColor(255,  85, 170),  // 粉
};
static const int COLOR_COUNT = 8;

LabelManager::LabelManager()
{
    // 默认标签
    addLabel("mask");
    addLabel("withoutmask");
}

void LabelManager::addLabel(const QString& label)
{
    if (!m_labels.contains(label)) {
        int idx = m_labels.size();
        m_labels.append(label);
        m_colorMap[label] = generateColor(idx);
    }
}

void LabelManager::removeLabel(const QString& label)
{
    m_labels.removeAll(label);
    m_colorMap.remove(label);
}

QStringList LabelManager::getLabels() const
{
    return m_labels;
}

QColor LabelManager::getColor(const QString& label) const
{
    return m_colorMap.value(label, Qt::red);
}

bool LabelManager::contains(const QString& label) const
{
    return m_labels.contains(label);
}

QColor LabelManager::generateColor(int index) const
{
    return PRESET_COLORS[index % COLOR_COUNT];
}