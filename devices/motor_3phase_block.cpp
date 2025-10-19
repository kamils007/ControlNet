#include "motor_3phase_block.h"

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QLineF>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <QPolygonF>
#include <QStringList>

namespace {
constexpr qreal S = 1.0;               // prosty współczynnik skalowania
constexpr qreal TERMINAL_RADIUS = 8.0; // promień zacisków
constexpr qreal BODY_RADIUS     = 42.0;
constexpr qreal LINE_WIDTH      = 2.0;

QColor colFrame()   { return QColor(70, 70, 70); }
QColor colFill()    { return QColor(235, 235, 235); }
QColor colPhase()   { return QColor(220, 60, 60); }
QColor colReverse() { return QColor(60, 100, 220); }
QColor colInactive(){ return QColor(255, 255, 255); }
QColor colText()    { return QColor(40, 40, 40); }
}

Motor3PhaseBlock::Motor3PhaseBlock(QGraphicsScene* scene,
                                   const QString&  prefix,
                                   const QPointF&  topLeft,
                                   QObject* parent)
    : QObject(parent)
    , m_scene(scene)
    , m_prefix(prefix)
{
    build(topLeft);
}

void Motor3PhaseBlock::build(const QPointF& topLeft)
{
    if (!m_scene)
        return;

    const QPointF center = topLeft + QPointF(120.0 * S, 70.0 * S);
    const QPointF terminalBase = topLeft + QPointF(20.0 * S, 20.0 * S);
    const qreal spacing = 52.0 * S;

    // Korpus silnika
    m_body = m_scene->addEllipse(center.x() - BODY_RADIUS, center.y() - BODY_RADIUS,
                                 BODY_RADIUS * 2.0, BODY_RADIUS * 2.0,
                                 QPen(colFrame(), LINE_WIDTH), QBrush(colFill()));
    m_body->setZValue(1);
    m_items.push_back(m_body);

    // Etykieta "M"
    m_labelM = addLabel(center + QPointF(-12.0, -18.0), QStringLiteral("M"), 20, QFont::Bold);

    // Zaciski U/V/W
    const QStringList pinLabels = {QStringLiteral("U"), QStringLiteral("V"), QStringLiteral("W")};
    for (int i = 0; i < pinLabels.size(); ++i) {
        const QPointF pos = terminalBase + QPointF(0.0, spacing * i);
        auto* term = addTerminal(m_prefix + pinLabels[i], pos);
        m_terminals.push_back({m_prefix + pinLabels[i], term});
        m_pins.insert(m_prefix + pinLabels[i]);

        // krótka linia łącząca zacisk z korpusem
        const QPointF lineEnd = pos + QPointF(42.0 * S, 0.0);
        m_phaseTicks.push_back(addLine(pos + QPointF(TERMINAL_RADIUS, 0.0), lineEnd));

        addLabel(pos + QPointF(-22.0, -10.0), pinLabels[i]);
    }

    // Dekoracyjne połączenia i rama
    addLine(center + QPointF(-BODY_RADIUS, -BODY_RADIUS),
            center + QPointF(-BODY_RADIUS, BODY_RADIUS), Qt::DashLine);
    addLine(center + QPointF(BODY_RADIUS, -BODY_RADIUS),
            center + QPointF(BODY_RADIUS, BODY_RADIUS), Qt::DashLine);

    ensureDirectionLabel();
    updateRotation();
}

QGraphicsEllipseItem* Motor3PhaseBlock::addTerminal(const QString& name, const QPointF& center)
{
    auto* ellipse = m_scene->addEllipse(center.x() - TERMINAL_RADIUS,
                                        center.y() - TERMINAL_RADIUS,
                                        TERMINAL_RADIUS * 2.0,
                                        TERMINAL_RADIUS * 2.0,
                                        QPen(colFrame(), LINE_WIDTH), QBrush(colInactive()));
    ellipse->setZValue(2);
    ellipse->setData(0, name);
    m_items.push_back(ellipse);
    return ellipse;
}

QGraphicsLineItem* Motor3PhaseBlock::addLine(const QPointF& p1, const QPointF& p2, Qt::PenStyle style)
{
    auto* line = m_scene->addLine(QLineF(p1, p2), QPen(colFrame(), LINE_WIDTH, style));
    line->setZValue(0);
    m_items.push_back(line);
    return line;
}

QGraphicsSimpleTextItem* Motor3PhaseBlock::addLabel(const QPointF& pos,
                                                   const QString& text,
                                                   int pointSize,
                                                   QFont::Weight weight)
{
    auto* label = m_scene->addSimpleText(text);
    QFont f = label->font();
    f.setPointSize(pointSize);
    f.setWeight(weight);
    label->setFont(f);
    label->setBrush(QBrush(colText()));
    label->setPos(pos);
    label->setZValue(3);
    m_items.push_back(label);
    return label;
}

void Motor3PhaseBlock::ensureDirectionLabel()
{
    if (m_dirLabel)
        return;

    if (!m_scene)
        return;

    const QPointF pos = m_body ? (m_body->rect().center() + QPointF(-10.0, 6.0)) : QPointF();
    m_dirLabel = addLabel(pos, QStringLiteral("–"), 18, QFont::DemiBold);

    // strzałka dookólna (kierunek obrotów)
    if (m_body) {
        QPainterPath path;
        const QRectF r = m_body->rect().adjusted(10.0, 10.0, -10.0, -10.0);
        path.arcMoveTo(r, 20.0);
        path.arcTo(r, 20.0, -320.0);
        const QPointF arrowBase = path.pointAtPercent(0.85);
        QPolygonF head;
        head << arrowBase
             << arrowBase + QPointF(-6.0, -3.0)
             << arrowBase + QPointF(2.0, -8.0);
        path.addPolygon(head);
        m_arrow = m_scene->addPath(path, QPen(colFrame(), LINE_WIDTH), QBrush(colFrame()));
        m_arrow->setZValue(0.5);
        m_items.push_back(m_arrow);
    }
}

void Motor3PhaseBlock::setPhaseMasks(int maskU, int maskV, int maskW)
{
    m_phaseMask[0] = maskU;
    m_phaseMask[1] = maskV;
    m_phaseMask[2] = maskW;

    for (int i = 0; i < 3; ++i) {
        updateTerminalAppearance(i);
    }

    updateRotation();
}

void Motor3PhaseBlock::updateTerminalAppearance(int index)
{
    if (index < 0 || index >= m_terminals.size())
        return;

    auto* term = m_terminals[index].item;
    if (!term)
        return;

    const int mask = m_phaseMask[index];
    const bool active = (mask != 0);

    QBrush brush(active ? QBrush(colPhase()) : QBrush(colInactive()));
    term->setBrush(brush);

    QStringList phases;
    if (mask & 0x1) phases << QStringLiteral("L1");
    if (mask & 0x2) phases << QStringLiteral("L2");
    if (mask & 0x4) phases << QStringLiteral("L3");
    term->setToolTip(phases.isEmpty() ? QStringLiteral("brak fazy") : phases.join(QStringLiteral(", ")));

    if (index < m_phaseTicks.size() && m_phaseTicks[index]) {
        auto pen = m_phaseTicks[index]->pen();
        pen.setColor(active ? colPhase() : colFrame());
        m_phaseTicks[index]->setPen(pen);
    }
}

namespace {
int maskToIdx(int mask)
{
    if (mask == 0x1) return 0;
    if (mask == 0x2) return 1;
    if (mask == 0x4) return 2;
    return -1;
}
}

void Motor3PhaseBlock::updateRotation()
{
    const int a = maskToIdx(m_phaseMask[0]);
    const int b = maskToIdx(m_phaseMask[1]);
    const int c = maskToIdx(m_phaseMask[2]);

    int dir = 0; // 0 = brak informacji, +1 = zgodnie z ruchem wskazówek, -1 = przeciwnie
    if (a >= 0 && b >= 0 && c >= 0) {
        const int inversions = (a > b) + (a > c) + (b > c);
        dir = (inversions % 2 == 0) ? +1 : -1;
    }

    ensureDirectionLabel();
    if (m_dirLabel) {
        QString text;
        if (dir > 0)      text = QStringLiteral("↻");
        else if (dir < 0) text = QStringLiteral("↺");
        else              text = QStringLiteral("–");
        m_dirLabel->setText(text);
        if (m_body) {
            const QPointF c = m_body->rect().center();
            m_dirLabel->setPos(c + QPointF(-10.0, 4.0));
        }
    }

    if (m_arrow) {
        m_arrow->setVisible(dir != 0);
        QPen pen = m_arrow->pen();
        pen.setColor(dir >= 0 ? colPhase() : colReverse());
        m_arrow->setPen(pen);
        m_arrow->setBrush(dir >= 0 ? QBrush(colPhase()) : QBrush(colReverse()));
        m_arrow->setTransformOriginPoint(m_arrow->boundingRect().center());
        m_arrow->setScale(dir >= 0 ? 1.0 : -1.0);
    }
}
