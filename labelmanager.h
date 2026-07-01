#ifndef LABELMANAGER_H
#define LABELMANAGER_H

#include <QStringList>
#include <QMap>
#include <QColor>

class LabelManager
{
public:
    LabelManager();

    void addLabel(const QString& label);
    void removeLabel(const QString& label);
    QStringList getLabels() const;
    QColor getColor(const QString& label) const;
    bool contains(const QString& label) const;

private:
    QStringList         m_labels;
    QMap<QString,QColor> m_colorMap;

    QColor generateColor(int index) const;
};

#endif // LABELMANAGER_H