#include "power_block.h"
#include "contactor_LC1D09_LADC22.h"
#include "contactor_view.h"
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QBrush>
#include <QPen>
#include <cmath>

static inline int PX(qreal v){ return int(std::lround(v * 0.7)); }

PowerBlock::PowerBlock(QGraphicsScene* scene,
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

void PowerBlock::build(const QPointF& off) {
    const int BW = PX(260);
    const int BH = PX(170);
    const int X  = int(off.x());
    const int Y  = int(off.y());

    // Rama + tytuł
    auto* r = m_scene->addRect(X, Y, BW, BH, ContactorView::penWire(1.8), QBrush(Qt::NoBrush));
    m_track(r); m_items.push_back(r);

    auto* title = m_scene->addSimpleText(QStringLiteral("ZASILANIE 3F (%1)").arg(m_prefix.left(m_prefix.size()-1)));
    title->setBrush(ContactorView::colText());
    title->setScale(0.7);
    title->setPos(QPointF(X + PX(8), Y - PX(22)));
    m_track(title); m_items.push_back(title);

    // Piny L1/L2/L3 po lewej
    const int pinX = X - PX(30);
    const int gap  = PX(40);
    const int L1y  = Y + PX(40);
    const int L2y  = L1y + gap;
    const int L3y  = L2y + gap;

    // <<< KLUCZOWE: zapisz wskaźniki linii i dodaj do m_items >>>
    if (auto* l = m_addLine(QPointF(pinX, L1y), QPointF(X, L1y))) m_items.push_back(l);
    if (auto* l = m_addLine(QPointF(pinX, L2y), QPointF(X, L2y))) m_items.push_back(l);
    if (auto* l = m_addLine(QPointF(pinX, L3y), QPointF(X, L3y))) m_items.push_back(l);

    const QString pL1 = m_prefix + "L1";
    const QString pL2 = m_prefix + "L2";
    const QString pL3 = m_prefix + "L3";

    if (auto* t = m_addTerminal(pL1, QPointF(pinX, L1y))) { m_pins.insert(pL1); m_items.push_back(t); }
    if (auto* t = m_addTerminal(pL2, QPointF(pinX, L2y))) { m_pins.insert(pL2); m_items.push_back(t); }
    if (auto* t = m_addTerminal(pL3, QPointF(pinX, L3y))) { m_pins.insert(pL3); m_items.push_back(t); }

    auto* l1 = m_scene->addSimpleText("L1");
    l1->setBrush(ContactorView::colText()); l1->setScale(0.7); l1->setPos(QPointF(pinX - PX(20), L1y - PX(26)));
    m_track(l1); m_items.push_back(l1);
    auto* l2 = m_scene->addSimpleText("L2");
    l2->setBrush(ContactorView::colText()); l2->setScale(0.7); l2->setPos(QPointF(pinX - PX(20), L2y - PX(26)));
    m_track(l2); m_items.push_back(l2);
    auto* l3 = m_scene->addSimpleText("L3");
    l3->setBrush(ContactorView::colText()); l3->setScale(0.7); l3->setPos(QPointF(pinX - PX(20), L3y - PX(26)));
    m_track(l3); m_items.push_back(l3);

    // Przycisk POWER (FAZA na L1/L2/L3)
    const QRectF rBtn(X + BW - PX(90), Y + BH/2 - PX(12), PX(70), PX(24));
    m_btnPower = new SchematicButton(rBtn, "POWER", nullptr, ContactorView::colPhase());
    m_scene->addItem(m_btnPower);
    m_track(m_btnPower); m_items.push_back(m_btnPower);

    connect(m_btnPower, &SchematicButton::toggled, this, [this, pL1, pL2, pL3](bool on){
        if (on) { emit requestAddPhase(pL1); emit requestAddPhase(pL2); emit requestAddPhase(pL3); }
        else    { emit requestRemovePhase(pL1); emit requestRemovePhase(pL2); emit requestRemovePhase(pL3); }
    });
}
