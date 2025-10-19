#include "contactor_LC1D09_LADC22.h"

#include <QBrush>
#include <QCursor>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSimpleTextItem>
#include <QPainter>
#include <QPen>
#include <QLineF>

#include <cmath>

namespace {

constexpr qreal SCALE_FACTOR = 0.7;

int PX(qreal v) { return int(std::lround(v * SCALE_FACTOR)); }

QColor colWire()    { return QColor(220, 230, 245); }
QColor colBlock()   { return QColor(200, 40, 40); }
QColor colText()    { return QColor(190, 170, 255); }
QColor colPhase()   { return QColor(220, 60, 60); }
QColor colNeutral() { return QColor(66, 133, 244); }

QPen penWire(qreal width = 1.6, Qt::PenStyle style = Qt::SolidLine)
{
    QPen pen(colWire());
    pen.setWidthF(width);
    pen.setStyle(style);
    return pen;
}

QPen penBlock(qreal width = 1.6)
{
    QPen pen(colBlock());
    pen.setWidthF(width);
    return pen;
}

QGraphicsSimpleTextItem* addText(QGraphicsScene* scene,
                                 const QString&    text,
                                 const QPointF&    pos,
                                 qreal             scale = 1.0)
{
    auto* item = scene->addSimpleText(text);
    item->setBrush(colText());
    item->setScale(scale * SCALE_FACTOR);
    item->setPos(pos);
    item->setZValue(2.0);
    return item;
}

} // namespace

// ======== SchematicButton ====================================================

SchematicButton::SchematicButton(const QRectF& r,
                                 const QString& text,
                                 QGraphicsItem* parent,
                                 const QColor& onColor)
    : QGraphicsObject(parent)
    , m_rect(r)
    , m_text(text)
    , m_onColor(onColor)
{
    setAcceptedMouseButtons(Qt::LeftButton);
    setCursor(Qt::PointingHandCursor);
}

void SchematicButton::setOn(bool on)
{
    if (m_on == on)
        return;
    m_on = on;
    update();
}

void SchematicButton::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*)
{
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(penWire(1.6));
    p->setBrush(m_on ? QBrush(m_onColor) : QBrush(Qt::NoBrush));
    p->drawRoundedRect(m_rect, PX(5), PX(5));

    p->setPen(penWire());
    p->drawText(m_rect, Qt::AlignCenter, m_text);
}

void SchematicButton::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        m_on = !m_on;
        update();
        emit toggled(m_on);
    }
    QGraphicsObject::mousePressEvent(e);
}

// ======== Contactor_LC1D09_LADC22 ===========================================

Contactor_LC1D09_LADC22::Contactor_LC1D09_LADC22(QGraphicsScene* scene,
                                                 const QString&  prefix,
                                                 const QPointF&  topLeft,
                                                 QObject*        parent)
    : QObject(parent)
    , m_scene(scene)
    , m_prefix(prefix)
{
    if (!m_scene)
        return;
    build(topLeft);
}

void Contactor_LC1D09_LADC22::build(const QPointF& off)
{
    const int BODY_W       = PX(420);
    const int BODY_H       = PX(320);
    const int MARGIN_TOP_L = PX(60);
    const int MARGIN_BOT_T = PX(60);

    const int BODY_X = int(off.x());
    const int BODY_Y = int(off.y());

    const int col1X = BODY_X + PX(110);
    const int col2X = BODY_X + BODY_W / 2;
    const int col3X = BODY_X + BODY_W - PX(110);

    // Korpus + tytuł
    auto* body = m_scene->addRect(BODY_X, BODY_Y, BODY_W, BODY_H, penWire(1.8), QBrush(Qt::NoBrush));
    m_items.push_back(body);

    auto* title = addText(m_scene, m_prefix.left(m_prefix.size() - 1), QPointF(BODY_X + PX(6), BODY_Y - PX(22)), 1.0);
    m_items.push_back(title);

    const int midY = BODY_Y + BODY_H / 2;
    m_items.push_back(createLine(QPointF(BODY_X, midY), QPointF(BODY_X + BODY_W, midY), Qt::DashLine));

    const int topY = BODY_Y - MARGIN_TOP_L;
    const int botY = BODY_Y + BODY_H + MARGIN_BOT_T;

    // piony L i T
    m_items.push_back(createLine(QPointF(col1X, topY), QPointF(col1X, BODY_Y)));
    m_items.push_back(createLine(QPointF(col2X, topY), QPointF(col2X, BODY_Y)));
    m_items.push_back(createLine(QPointF(col3X, topY), QPointF(col3X, BODY_Y)));
    m_items.push_back(createLine(QPointF(col1X, BODY_Y + BODY_H), QPointF(col1X, botY)));
    m_items.push_back(createLine(QPointF(col2X, BODY_Y + BODY_H), QPointF(col2X, botY)));
    m_items.push_back(createLine(QPointF(col3X, BODY_Y + BODY_H), QPointF(col3X, botY)));

    // Piny L1/L2/L3 u góry
    const QString pL1 = m_prefix + "L1";
    const QString pL2 = m_prefix + "L2";
    const QString pL3 = m_prefix + "L3";
    createTerminal(pL1, QPointF(col1X, topY));
    createTerminal(pL2, QPointF(col2X, topY));
    createTerminal(pL3, QPointF(col3X, topY));
    m_items.push_back(addText(m_scene, "L1", QPointF(col1X - PX(8), topY - PX(26)), 1.0));
    m_items.push_back(addText(m_scene, "L2", QPointF(col2X - PX(8), topY - PX(26)), 1.0));
    m_items.push_back(addText(m_scene, "L3", QPointF(col3X - PX(8), topY - PX(26)), 1.0));

    // Piny T1/T2/T3 na dole
    const QString pT1 = m_prefix + "T1";
    const QString pT2 = m_prefix + "T2";
    const QString pT3 = m_prefix + "T3";
    createTerminal(pT1, QPointF(col1X, botY));
    createTerminal(pT2, QPointF(col2X, botY));
    createTerminal(pT3, QPointF(col3X, botY));
    m_items.push_back(addText(m_scene, "T1", QPointF(col1X - PX(10), botY + PX(10)), 1.0));
    m_items.push_back(addText(m_scene, "T2", QPointF(col2X - PX(10), botY + PX(10)), 1.0));
    m_items.push_back(addText(m_scene, "T3", QPointF(col3X - PX(10), botY + PX(10)), 1.0));
    addContactEdge("L1", "T1", ContactKind::NormallyOpen);
    addContactEdge("L2", "T2", ContactKind::NormallyOpen);
    addContactEdge("L3", "T3", ContactKind::NormallyOpen);

    // Wejścia cewki A1/A2 z lewej + przyciski START/RET
    const int a1y    = BODY_Y + BODY_H / 2 - PX(60);
    const int a2y    = BODY_Y + BODY_H / 2 + PX(60);
    const int aLeftX = BODY_X - PX(90);
    m_items.push_back(createLine(QPointF(aLeftX, a1y), QPointF(BODY_X, a1y)));
    m_items.push_back(createLine(QPointF(aLeftX, a2y), QPointF(BODY_X, a2y)));

    const QString pA1 = m_prefix + "A1";
    const QString pA2 = m_prefix + "A2";
    createTerminal(pA1, QPointF(aLeftX, a1y));
    createTerminal(pA2, QPointF(aLeftX, a2y));
    m_items.push_back(addText(m_scene, "A1", QPointF(aLeftX - PX(20), a1y - PX(26)), 1.0));
    m_items.push_back(addText(m_scene, "A2", QPointF(aLeftX - PX(20), a2y - PX(26)), 1.0));

    const QRectF rBtn1(aLeftX - PX(70), a1y - PX(12), PX(50), PX(24));
    const QRectF rBtn2(aLeftX - PX(70), a2y - PX(12), PX(50), PX(24));
    m_btnA1 = new SchematicButton(rBtn1, QStringLiteral("START"), nullptr, colPhase());
    m_btnA2 = new SchematicButton(rBtn2, QStringLiteral("RET"),   nullptr, colNeutral());
    m_scene->addItem(m_btnA1);
    m_scene->addItem(m_btnA2);
    m_items.push_back(m_btnA1);
    m_items.push_back(m_btnA2);

    connect(m_btnA1, &SchematicButton::toggled, this, [this, pA1](bool on) {
        if (on)
            emit requestAddPhase(pA1);
        else
            emit requestRemovePhase(pA1);
    });
    connect(m_btnA2, &SchematicButton::toggled, this, [this, pA2](bool on) {
        if (on)
            emit requestAddNeutral(pA2);
        else
            emit requestRemoveNeutral(pA2);
    });

    // Bloki opisowe i styki pomocnicze (LADC22)
    const QRectF rLC(BODY_X + PX(20),           BODY_Y + PX(35), BODY_W / 2 - PX(85), BODY_H - PX(70));
    const QRectF rLA(BODY_X + BODY_W / 2 + PX(5), BODY_Y + PX(35), BODY_W / 2 - PX(20), BODY_H - PX(70));
    m_items.push_back(m_scene->addRect(rLC, penBlock(1.6)));
    m_items.push_back(m_scene->addRect(rLA, penBlock(1.6)));
    m_items.push_back(addText(m_scene, "LC1D09", rLC.bottomLeft() + QPointF(PX(6), -PX(16)), 1.0));
    m_items.push_back(addText(m_scene, "LADC22", rLA.bottomLeft() + QPointF(PX(6), -PX(16)), 1.0));

    const int auxGapV = PX(60);
    const int auxTopY = BODY_Y + PX(70);

    auto addAux = [&](const QString& upper, const QString& lower, const QString& label, int pinOffset, int textOffset) {
        createTerminal(upper, QPointF(int(rLA.left()) + PX(pinOffset), auxTopY));
        createTerminal(lower, QPointF(int(rLA.left()) + PX(pinOffset), auxTopY + auxGapV));
        m_items.push_back(addText(m_scene, label, QPointF(int(rLA.left()) + PX(textOffset), auxTopY - PX(24)), 0.9));
    };

    addAux(m_prefix + "53", m_prefix + "54", QStringLiteral("53 NO"), 20, 0);
    addAux(m_prefix + "61", m_prefix + "62", QStringLiteral("61 NC"), 70, 50);
    addAux(m_prefix + "75", m_prefix + "76", QStringLiteral("75 NC"), 120, 100);
    addAux(m_prefix + "87", m_prefix + "88", QStringLiteral("87 NO"), 170, 150);
    addContactEdge("53", "54", ContactKind::NormallyOpen);
    addContactEdge("61", "62", ContactKind::NormallyClosed);
    addContactEdge("75", "76", ContactKind::NormallyClosed);
    addContactEdge("87", "88", ContactKind::NormallyOpen);
}

QGraphicsEllipseItem* Contactor_LC1D09_LADC22::createTerminal(const QString& name, const QPointF& center)
{
    const int r = PX(10);
    auto* ellipse = m_scene->addEllipse(center.x() - r, center.y() - r, 2 * r, 2 * r,
                                        penWire(1.6), QBrush(Qt::NoBrush));
    ellipse->setToolTip(name);
    ellipse->setZValue(1.1);

    m_items.push_back(ellipse);
    m_pins.insert(name);
    m_terminals.insert(name, ellipse);
    return ellipse;
}

QGraphicsLineItem* Contactor_LC1D09_LADC22::createLine(const QPointF& a, const QPointF& b, Qt::PenStyle style)
{
    QPen pen = (style == Qt::DashLine) ? penWire(1.2, Qt::DashLine) : penWire(1.6, style);
    auto* line = m_scene->addLine(QLineF(a, b), pen);
    line->setZValue(0.5);
    return line;
}

void Contactor_LC1D09_LADC22::addContactEdge(const QString& pinA, const QString& pinB, ContactKind kind)
{
    m_contactEdges.push_back({m_prefix + pinA, m_prefix + pinB, kind});
}

