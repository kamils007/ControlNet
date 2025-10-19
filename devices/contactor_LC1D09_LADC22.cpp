#include "contactor_LC1D09_LADC22.h"
#include "contactor_view.h"   // kolory/pióra + SchematicButton

#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QBrush>
#include <QPen>
#include <cmath>

static inline qreal SCALE() { return 0.7; }
static inline int PX(qreal v){ return int(std::lround(v * SCALE())); }

static QGraphicsSimpleTextItem* addText(QGraphicsScene* sc, const QString& t, const QPointF& pos, qreal scale = 1.0) {
    auto* it = sc->addSimpleText(t);
    it->setBrush(ContactorView::colText());
    it->setScale(scale * SCALE());
    it->setPos(pos);
    it->setZValue(2);
    return it;
}

Contactor_LC1D09_LADC22::Contactor_LC1D09_LADC22(QGraphicsScene* scene,
                                                 const QString& prefix,
                                                 const QPointF& topLeft,
                                                 AddTerminalFn addTerminal,
                                                 AddLineFn addLine,
                                                 TrackFn track,
                                                 QObject* parent)
    : QObject(parent),
    m_scene(scene),
    m_prefix(prefix),
    m_addTerminal(std::move(addTerminal)),
    m_addLine(std::move(addLine)),
    m_track(std::move(track))
{
    build(topLeft);
}

void Contactor_LC1D09_LADC22::build(const QPointF& off) {
    // Wymiary i pozycje
    const int BODY_W        = PX(420);
    const int BODY_H        = PX(320);
    const int MARGIN_TOP_L  = PX(60);
    const int MARGIN_BOT_T  = PX(60);

    const int BODY_X    = int(off.x());
    const int BODY_Y    = int(off.y());

    const int col1X = BODY_X + PX(110);
    const int col2X = BODY_X + BODY_W/2;
    const int col3X = BODY_X + BODY_W - PX(110);

    // Korpus + tytuł
    auto* body = m_scene->addRect(BODY_X, BODY_Y, BODY_W, BODY_H, ContactorView::penWire(1.8), QBrush(Qt::NoBrush));
    m_track(body); m_items.push_back(body);

    auto* tK = addText(m_scene, m_prefix.left(m_prefix.size()-1), QPointF(BODY_X + PX(6), BODY_Y - PX(22)), 1.0);
    m_track(tK); m_items.push_back(tK);

    const int midY = BODY_Y + BODY_H/2;
    m_addLine(QPointF(BODY_X, midY), QPointF(BODY_X+BODY_W, midY), Qt::DashLine);

    const int topY = BODY_Y - MARGIN_TOP_L;
    const int botY = BODY_Y + BODY_H + MARGIN_BOT_T;

    // piony L i T
    m_addLine(QPointF(col1X, topY),           QPointF(col1X, BODY_Y), Qt::SolidLine);
    m_addLine(QPointF(col2X, topY),           QPointF(col2X, BODY_Y), Qt::SolidLine);
    m_addLine(QPointF(col3X, topY),           QPointF(col3X, BODY_Y), Qt::SolidLine);
    m_addLine(QPointF(col1X, BODY_Y+BODY_H),  QPointF(col1X, botY),   Qt::SolidLine);
    m_addLine(QPointF(col2X, BODY_Y+BODY_H),  QPointF(col2X, botY),   Qt::SolidLine);
    m_addLine(QPointF(col3X, BODY_Y+BODY_H),  QPointF(col3X, botY),   Qt::SolidLine);

    // Piny L1/L2/L3 u góry
    const QString pL1 = m_prefix + "L1";
    const QString pL2 = m_prefix + "L2";
    const QString pL3 = m_prefix + "L3";
    if (auto* t = m_addTerminal(pL1, QPointF(col1X, topY))) { m_pins.insert(pL1); m_items.push_back(t); }
    if (auto* t = m_addTerminal(pL2, QPointF(col2X, topY))) { m_pins.insert(pL2); m_items.push_back(t); }
    if (auto* t = m_addTerminal(pL3, QPointF(col3X, topY))) { m_pins.insert(pL3); m_items.push_back(t); }
    auto* l1t = addText(m_scene, "L1", QPointF(col1X-PX(8), topY-PX(26)), 1.0); m_track(l1t); m_items.push_back(l1t);
    auto* l2t = addText(m_scene, "L2", QPointF(col2X-PX(8), topY-PX(26)), 1.0); m_track(l2t); m_items.push_back(l2t);
    auto* l3t = addText(m_scene, "L3", QPointF(col3X-PX(8), topY-PX(26)), 1.0); m_track(l3t); m_items.push_back(l3t);

    // Piny T1/T2/T3 na dole
    const QString pT1 = m_prefix + "T1";
    const QString pT2 = m_prefix + "T2";
    const QString pT3 = m_prefix + "T3";
    if (auto* t = m_addTerminal(pT1, QPointF(col1X, botY))) { m_pins.insert(pT1); m_items.push_back(t); }
    if (auto* t = m_addTerminal(pT2, QPointF(col2X, botY))) { m_pins.insert(pT2); m_items.push_back(t); }
    if (auto* t = m_addTerminal(pT3, QPointF(col3X, botY))) { m_pins.insert(pT3); m_items.push_back(t); }
    auto* t1t = addText(m_scene, "T1", QPointF(col1X-PX(10), botY+PX(10)), 1.0); m_track(t1t); m_items.push_back(t1t);
    auto* t2t = addText(m_scene, "T2", QPointF(col2X-PX(10), botY+PX(10)), 1.0); m_track(t2t); m_items.push_back(t2t);
    auto* t3t = addText(m_scene, "T3", QPointF(col3X-PX(10), botY+PX(10)), 1.0); m_track(t3t); m_items.push_back(t3t);

    // Wejścia cewki A1/A2 z lewej + przyciski START/RET
    const int a1y    = BODY_Y + BODY_H/2 - PX(60);
    const int a2y    = BODY_Y + BODY_H/2 + PX(60);
    const int aLeftX = BODY_X - PX(90);
    m_addLine(QPointF(aLeftX, a1y), QPointF(BODY_X, a1y), Qt::SolidLine);
    m_addLine(QPointF(aLeftX, a2y), QPointF(BODY_X, a2y), Qt::SolidLine);

    const QString pA1 = m_prefix + "A1";
    const QString pA2 = m_prefix + "A2";
    if (auto* t = m_addTerminal(pA1, QPointF(aLeftX, a1y))) { m_pins.insert(pA1); m_items.push_back(t); }
    if (auto* t = m_addTerminal(pA2, QPointF(aLeftX, a2y))) { m_pins.insert(pA2); m_items.push_back(t); }
    auto* a1t = addText(m_scene, "A1", QPointF(aLeftX-PX(20), a1y-PX(26)), 1.0); m_track(a1t); m_items.push_back(a1t);
    auto* a2t = addText(m_scene, "A2", QPointF(aLeftX-PX(20), a2y-PX(26)), 1.0); m_track(a2t); m_items.push_back(a2t);

    const QRectF rBtn1(aLeftX-PX(70), a1y-PX(12), PX(50), PX(24));
    const QRectF rBtn2(aLeftX-PX(70), a2y-PX(12), PX(50), PX(24));
    m_btnA1  = new SchematicButton(rBtn1, "START", nullptr, ContactorView::colPhase());
    m_btnA2  = new SchematicButton(rBtn2, "RET",   nullptr, ContactorView::colNeutral());
    m_scene->addItem(m_btnA1); m_track(m_btnA1); m_items.push_back(m_btnA1);
    m_scene->addItem(m_btnA2); m_track(m_btnA2); m_items.push_back(m_btnA2);

    // sygnały przycisków (forward)
    connect(m_btnA1, &SchematicButton::toggled, this, [this, pA1](bool on){
        if (on) emit requestAddPhase(pA1);
        else    emit requestRemovePhase(pA1);
    });
    connect(m_btnA2, &SchematicButton::toggled, this, [this, pA2](bool on){
        if (on) emit requestAddNeutral(pA2);
        else    emit requestRemoveNeutral(pA2);
    });

    // Bloki opisowe i styki pomocnicze (LADC22)
    const QRectF rLC(BODY_X + PX(20),           BODY_Y + PX(35), BODY_W/2 - PX(85), BODY_H - PX(70));
    const QRectF rLA(BODY_X + BODY_W/2 + PX(5), BODY_Y + PX(35), BODY_W/2 - PX(20), BODY_H - PX(70));
    auto* r1 = m_scene->addRect(rLC, ContactorView::penBlock(1.6)); m_track(r1); m_items.push_back(r1);
    auto* r2 = m_scene->addRect(rLA, ContactorView::penBlock(1.6)); m_track(r2); m_items.push_back(r2);
    auto* n1 = addText(m_scene, "LC1D09", rLC.bottomLeft() + QPointF(PX(6), -PX(16)), 1.0); m_track(n1); m_items.push_back(n1);
    auto* n2 = addText(m_scene, "LADC22", rLA.bottomLeft() + QPointF(PX(6), -PX(16)), 1.0); m_track(n2); m_items.push_back(n2);

    const int auxGapV = PX(60);
    const int auxTopY = BODY_Y + PX(70);

    // 53-54 (NO)
    if (auto* t = m_addTerminal(m_prefix + "53", QPointF(int(rLA.left()) + PX(20),  auxTopY)))         { m_pins.insert(m_prefix + "53"); m_items.push_back(t); }
    if (auto* t = m_addTerminal(m_prefix + "54", QPointF(int(rLA.left()) + PX(20),  auxTopY+auxGapV))) { m_pins.insert(m_prefix + "54"); m_items.push_back(t); }
    auto* t53 = addText(m_scene, "53 NO", QPointF(int(rLA.left()) + PX(0), auxTopY-PX(24)), 0.9); m_track(t53); m_items.push_back(t53);

    // 61-62 (NC)
    if (auto* t = m_addTerminal(m_prefix + "61", QPointF(int(rLA.left()) + PX(70),  auxTopY)))         { m_pins.insert(m_prefix + "61"); m_items.push_back(t); }
    if (auto* t = m_addTerminal(m_prefix + "62", QPointF(int(rLA.left()) + PX(70),  auxTopY+auxGapV))) { m_pins.insert(m_prefix + "62"); m_items.push_back(t); }
    auto* t61 = addText(m_scene, "61 NC", QPointF(int(rLA.left()) + PX(50), auxTopY-PX(24)), 0.9); m_track(t61); m_items.push_back(t61);

    // 75-76 (NC)
    if (auto* t = m_addTerminal(m_prefix + "75", QPointF(int(rLA.left()) + PX(120), auxTopY)))         { m_pins.insert(m_prefix + "75"); m_items.push_back(t); }
    if (auto* t = m_addTerminal(m_prefix + "76", QPointF(int(rLA.left()) + PX(120), auxTopY+auxGapV))) { m_pins.insert(m_prefix + "76"); m_items.push_back(t); }
    auto* t75 = addText(m_scene, "75 NC", QPointF(int(rLA.left()) + PX(100), auxTopY-PX(24)), 0.9); m_track(t75); m_items.push_back(t75);

    // 87-88 (NO)
    if (auto* t = m_addTerminal(m_prefix + "87", QPointF(int(rLA.left()) + PX(170), auxTopY)))         { m_pins.insert(m_prefix + "87"); m_items.push_back(t); }
    if (auto* t = m_addTerminal(m_prefix + "88", QPointF(int(rLA.left()) + PX(170), auxTopY+auxGapV))) { m_pins.insert(m_prefix + "88"); m_items.push_back(t); }
    auto* t87 = addText(m_scene, "87 NO", QPointF(int(rLA.left()) + PX(150), auxTopY-PX(24)), 0.9); m_track(t87); m_items.push_back(t87);
}
