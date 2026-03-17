#pragma once
#include <QWidget>
#include <QList>
#include <QRect>

class SnapManager {
public:
    static SnapManager* instance() {
        static SnapManager s;
        return &s;
    }

    void add(QWidget* w) {
        if (!m_windows.contains(w)) m_windows.append(w);
    }

    void snap(QWidget* moving, int threshold = 20) {
        if (!moving) return;
        QRect r = moving->geometry();
        for (QWidget* other : m_windows) {
            if (other == moving || !other->isVisible()) continue;
            QRect o = other->geometry();
            // Snap verticale: bottom di moving → top di other
            if (qAbs(r.bottom() - o.top()) <= threshold)
                r.moveBottom(o.top() - 1);
            // Snap verticale: top di moving → bottom di other
            else if (qAbs(r.top() - o.bottom()) <= threshold)
                r.moveTop(o.bottom() + 1);
            // Allineamento orizzontale: left edges
            if (qAbs(r.left() - o.left()) <= threshold)
                r.moveLeft(o.left());
        }
        moving->move(r.topLeft());
    }

private:
    QList<QWidget*> m_windows;
};
