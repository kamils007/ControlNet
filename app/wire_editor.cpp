#include "wire_editor.h"
#include "contactor_view.h"

#include <QGraphicsPathItem>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPen>
#include <QtMath>

WireEditor::WireEditor(ContactorView* view, QObject* parent)
    : QObject(parent), m_view(view)
{
    if (m_view && m_view->viewport())
        m_view->viewport()->installEventFilter(this);
}

bool WireEditor::eventFilter(QObject* obj, QEvent* ev) {
    if (!m_view || obj != m_view->viewport()) return QObject::eventFilter(obj, ev);

    switch (ev->type()) {
    case QEvent::MouseButtonDblClick: {
        auto* me = static_cast<QMouseEvent*>(ev);
        const QPointF sp = m_view->mapToScene(me->pos());
        const QString pin = m_view->terminalAt(sp, 16.0);
        if (!m_active) {
            if (!pin.isEmpty() && isAuxPin(pin)) startAt(pin, m_view->terminalPos(pin));
        } else {
            if (!pin.isEmpty() && isAuxPin(pin)) finishAt(pin, m_view->terminalPos(pin));
            else cancel();
        }
        return true;
    }
    case QEvent::MouseMove: {
        auto* me = static_cast<QMouseEvent*>(ev);
        if (m_active) {
            onMouseMove(m_view->mapToScene(me->pos()));
            return true;
        } else {
            const QString pin = m_view->terminalAt(m_view->mapToScene(me->pos()), 16.0);
            m_lastHoverPin = pin;
        }
        break;
    }
    case QEvent::MouseButtonPress: {
        auto* me = static_cast<QMouseEvent*>(ev);
        if (!m_active) break;
        if (me->button() == Qt::LeftButton) {
            onClick(m_view->mapToScene(me->pos()));
            return true;
        }
        break;
    }
    default: break;
    }
    return QObject::eventFilter(obj, ev);
}

void WireEditor::startAt(const QString& pin, const QPointF& pos) {
    m_active = true;
    m_startPin = pin;
    m_pts.clear();
    m_pts.push_back(pos);
    m_dir = Dir::None;

    if (!m_rubber) {
        m_rubber = new QGraphicsPathItem();
        m_rubber->setZValue(1.5);
        m_view->scene()->addItem(m_rubber);
    }
    m_rubber->setPen(ContactorView::penWire(2.0));
    m_rubber->setVisible(true);
}

void WireEditor::cancel() {
    m_active = false;
    m_startPin.clear();
    m_pts.clear();
    m_dir = Dir::None;
    if (m_rubber) m_rubber->setVisible(false);
}

void WireEditor::finishAt(const QString& pin, const QPointF& pos) {
    const QPointF aligned = orthoTo(pos);
    if (m_pts.isEmpty() || (m_pts.back() != aligned)) m_pts.push_back(aligned);

    // narysuj mostek i zarejestruj piny
    auto* item = m_view->addBridgePolyline(m_pts);
    if (item) m_view->registerBridge(item, m_startPin, pin);

    // zgłoś do logiki (MainWindow)
    emit wireCommitted(m_startPin, pin, m_pts);

    cancel();
}

void WireEditor::onMouseMove(const QPointF& scenePos) {
    if (!m_rubber || m_pts.isEmpty()) return;

    const QPointF aligned = orthoTo(scenePos);

    QPainterPath ph(m_pts.front());
    for (int i = 1; i < m_pts.size(); ++i) ph.lineTo(m_pts[i]);
    ph.lineTo(aligned);

    m_rubber->setPath(ph);
}

void WireEditor::onClick(const QPointF& scenePos) {
    const QPointF aligned = orthoTo(scenePos);
    if (m_pts.isEmpty() || m_pts.back() != aligned)
        m_pts.push_back(aligned);

    // Ustal kierunek następnego odcinka jako PROSTOPADŁY do ostatniego
    if (m_pts.size() >= 2) {
        const QPointF& a = m_pts[m_pts.size() - 2];
        const QPointF& b = m_pts[m_pts.size() - 1];
        const bool lastIsH = qFuzzyCompare(a.y(), b.y());   // ten odcinek jest poziomy?
        m_dir = lastIsH ? Dir::V : Dir::H;                  // następny ma być prostopadły
    } else {
        // Teoretycznie nie wejdziemy tu, ale na wszelki wypadek:
        const QPointF& last = m_pts.back();
        const bool lastIsH = qFuzzyCompare(last.y(), aligned.y());
        m_dir = lastIsH ? Dir::V : Dir::H;
    }
}


QPointF WireEditor::orthoTo(const QPointF& scenePos) const {
    if (m_pts.isEmpty()) return scenePos;
    const QPointF last = m_pts.back();
    if (m_dir == Dir::None) {
        const qreal dx = qAbs(scenePos.x() - last.x());
        const qreal dy = qAbs(scenePos.y() - last.y());
        if (dx >= dy) return QPointF(scenePos.x(), last.y()); // H
        else          return QPointF(last.x(), scenePos.y()); // V
    } else if (m_dir == Dir::H) {
        return QPointF(scenePos.x(), last.y());
    } else {
        return QPointF(last.x(), scenePos.y());
    }
}
