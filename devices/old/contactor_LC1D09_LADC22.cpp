#include "contactor_LC1D09_LADC22.h"

#include <QGraphicsScene>
#include <QGraphicsItemGroup>
#include <QGraphicsRectItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QPainter>
#include <QVariantAnimation>
#include <QMenu>
#include <QGraphicsSceneMouseEvent>
#include <cmath>

// ======= LOKALNE STYLE (jak w starym) =======================================
static inline qreal SCALE() { return 0.7; }
static inline int   PX(qreal v){ return int(std::lround(v * SCALE())); }

static QPen penWire(qreal w = 1.2, Qt::PenStyle style = Qt::SolidLine) {
    QPen p(Qt::black);
    p.setWidthF(w);
    p.setStyle(style);
    p.setCapStyle(Qt::RoundCap);
    p.setJoinStyle(Qt::RoundJoin);
    return p;
}
static QPen penBlock(qreal w = 1.6) {
    QPen p(QColor(40,40,40));
    p.setWidthF(w);
    return p;
}
static QColor colText()    { return QColor(20,20,20); }
static QColor colPhase()   { return QColor(220,40,40); }
static QColor colNeutral() { return QColor(40,100,220); }

static QGraphicsSimpleTextItem* addText(QGraphicsScene* sc, const QString& t, const QPointF& pos, qreal scale = 1.0) {
    auto* it = sc->addSimpleText(t);
    it->setBrush(colText());
    it->setScale(scale * SCALE());
    it->setPos(pos);
    it->setZValue(2);
    return it;
}

// ======= LEKKI WŁASNY „SCHEMATIC BUTTON” ====================================
// Prostokątny przycisk na scenie z napisem; emituje toggled(bool).
class SchematicButton : public QGraphicsObject {
    Q_OBJECT
public:
    SchematicButton(const QRectF& r, const QString& text, const QColor& col, QGraphicsItem* parent=nullptr)
        : QGraphicsObject(parent), m_rect(r), m_text(text), m_col(col)
    {
        setFlag(ItemIsSelectable, false);
        setAcceptedMouseButtons(Qt::LeftButton);
        m_anim = new QVariantAnimation(this);
        m_anim->setDuration(120);
        m_anim->setStartValue(0.0);
        m_anim->setEndValue(1.0);
        connect(m_anim, &QVariantAnimation::valueChanged, this, [this]{ update(); });
    }

    QRectF boundingRect() const override { return m_rect.adjusted(-1,-1,1,1); }

    void paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) override {
        p->setRenderHint(QPainter::Antialiasing, true);
        const qreal t = (m_on ? m_anim->currentValue().toReal() : (1.0 - m_anim->currentValue().toReal()));
        QColor fill = m_col.lighter(100 + int(30*t));
        fill.setAlpha(200);
        p->setPen(QPen(Qt::black, 1.0));
        p->setBrush(fill);
        p->drawRoundedRect(m_rect, 4, 4);

        p->setPen(Qt::white);
        QFont f = p->font(); f.setBold(true); f.setPointSizeF(8*SCALE()); p->setFont(f);
        p->drawText(m_rect, Qt::AlignCenter, m_text + (m_on ? " ON" : ""));
    }

signals:
    void toggled(bool on);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* e) override {
        if (e->button() != Qt::LeftButton) return;
        m_on = !m_on;
        m_anim->stop(); m_anim->start();
        update();
        emit toggled(m_on);
    }

private:
    QRectF m_rect;
    QString m_text;
    QColor m_col;
    bool m_on = false;
    QVariantAnimation* m_anim = nullptr;
};

// ======= IMPLEMENTACJA KONTAKTORA ============================================
Contactor_LC1D09_LADC22::Contactor_LC1D09_LADC22(QGraphicsScene* scene,
                                                 const QString& prefix,
                                                 const QPointF& topLeft,
                                                 QObject* parent)
    : QObject(parent), m_scene(scene), m_prefix(prefix)
{
    m_group = new QGraphicsItemGroup();
    m_group->setFlag(QGraphicsItem::ItemIsMovable, true);
    m_group->setFlag(QGraphicsItem::ItemIsSelectable, true);
    m_group->setZValue(1.0);
    m_scene->addItem(m_group);

    build(topLeft);
}

Contactor_LC1D09_LADC22::~Contactor_LC1D09_LADC22() {
    destroy();
}

QGraphicsItem* Contactor_LC1D09_LADC22::rootItem() const { return reinterpret_cast<QGraphicsItem*>(m_group); }
const QSet<QString>& Contactor_LC1D09_LADC22::pins() const { return m_pins; }
const QString& Contactor_LC1D09_LADC22::prefix() const { return m_prefix; }

// Rysuje elementy jak w starym projekcie (korpus, piny, opisy, przyciski)
void Contactor_LC1D09_LADC22::build(const QPointF& off)
{
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
    auto* body = m_scene->addRect(BODY_X, BODY_Y, BODY_W, BODY_H, penWire(1.8), QBrush(Qt::NoBrush));
    body->setParentItem(m_group); m_items.push_back(body);

    auto* tK = addText(m_scene, m_prefix.left(m_prefix.size()-1), QPointF(BODY_X + PX(6), BODY_Y - PX(22)), 1.0);
    tK->setParentItem(m_group); m_items.push_back(tK);

    const int midY = BODY_Y + BODY_H/2;
    {
        auto* mid = m_scene->addLine(BODY_X, midY, BODY_X+BODY_W, midY, penWire(1.0, Qt::DashLine));
        mid->setParentItem(m_group); m_items.push_back(mid);
    }

    const int topY = BODY_Y - MARGIN_TOP_L;
    const int botY = BODY_Y + BODY_H + MARGIN_BOT_T;

    // piony L/T
    auto addVLine = [&](int x1, int y1, int x2, int y2){
        auto* ln = m_scene->addLine(x1, y1, x2, y2, penWire(1.2));
        ln->setParentItem(m_group); m_items.push_back(ln);
    };
    addVLine(col1X, topY, col1X, BODY_Y);
    addVLine(col2X, topY, col2X, BODY_Y);
    addVLine(col3X, topY, col3X, BODY_Y);
    addVLine(col1X, BODY_Y+BODY_H, col1X, botY);
    addVLine(col2X, BODY_Y+BODY_H, col2X, botY);
    addVLine(col3X, BODY_Y+BODY_H, col3X, botY);

    // Piny L1/L2/L3 (góra)
    auto addPinCircle = [&](const QString& pinName, const QPointF& pos){
        auto* el = m_scene->addEllipse(pos.x()-PX(5), pos.y()-PX(5), PX(10), PX(10),
                                       penWire(1.2), QBrush(Qt::white));
        el->setParentItem(m_group);
        m_items.push_back(el);
        m_pins.insert(pinName);
    };
    const QString pL1 = m_prefix + "L1";
    const QString pL2 = m_prefix + "L2";
    const QString pL3 = m_prefix + "L3";
    addPinCircle(pL1, QPointF(col1X, topY));
    addPinCircle(pL2, QPointF(col2X, topY));
    addPinCircle(pL3, QPointF(col3X, topY));
    auto* l1t = addText(m_scene, "L1", QPointF(col1X-PX(8), topY-PX(26)), 1.0); l1t->setParentItem(m_group); m_items.push_back(l1t);
    auto* l2t = addText(m_scene, "L2", QPointF(col2X-PX(8), topY-PX(26)), 1.0); l2t->setParentItem(m_group); m_items.push_back(l2t);
    auto* l3t = addText(m_scene, "L3", QPointF(col3X-PX(8), topY-PX(26)), 1.0); l3t->setParentItem(m_group); m_items.push_back(l3t);

    // Piny T1/T2/T3 (dół)
    const QString pT1 = m_prefix + "T1";
    const QString pT2 = m_prefix + "T2";
    const QString pT3 = m_prefix + "T3";
    addPinCircle(pT1, QPointF(col1X, botY));
    addPinCircle(pT2, QPointF(col2X, botY));
    addPinCircle(pT3, QPointF(col3X, botY));
    auto* t1t = addText(m_scene, "T1", QPointF(col1X-PX(10), botY+PX(10)), 1.0); t1t->setParentItem(m_group); m_items.push_back(t1t);
    auto* t2t = addText(m_scene, "T2", QPointF(col2X-PX(10), botY+PX(10)), 1.0); t2t->setParentItem(m_group); m_items.push_back(t2t);
    auto* t3t = addText(m_scene, "T3", QPointF(col3X-PX(10), botY+PX(10)), 1.0); t3t->setParentItem(m_group); m_items.push_back(t3t);

    // Wejścia cewki A1/A2 + przyciski START/RET
    const int a1y    = BODY_Y + BODY_H/2 - PX(60);
    const int a2y    = BODY_Y + BODY_H/2 + PX(60);
    const int aLeftX = BODY_X - PX(90);
    auto* a1ln = m_scene->addLine(aLeftX, a1y, BODY_X, a1y, penWire(1.2));
    a1ln->setParentItem(m_group); m_items.push_back(a1ln);
    auto* a2ln = m_scene->addLine(aLeftX, a2y, BODY_X, a2y, penWire(1.2));
    a2ln->setParentItem(m_group); m_items.push_back(a2ln);

    const QString pA1 = m_prefix + "A1";
    const QString pA2 = m_prefix + "A2";
    addPinCircle(pA1, QPointF(aLeftX, a1y));
    addPinCircle(pA2, QPointF(aLeftX, a2y));
    auto* a1t = addText(m_scene, "A1", QPointF(aLeftX-PX(20), a1y-PX(26)), 1.0); a1t->setParentItem(m_group); m_items.push_back(a1t);
    auto* a2t = addText(m_scene, "A2", QPointF(aLeftX-PX(20), a2y-PX(26)), 1.0); a2t->setParentItem(m_group); m_items.push_back(a2t);

    // Własne przyciski
    const QRectF rBtn1(aLeftX-PX(70), a1y-PX(12), PX(50), PX(24));
    const QRectF rBtn2(aLeftX-PX(70), a2y-PX(12), PX(50), PX(24));
    auto* btnA1  = new SchematicButton(rBtn1, "START", colPhase());
    auto* btnA2  = new SchematicButton(rBtn2, "RET",   colNeutral());
    btnA1->setParentItem(m_group);
    btnA2->setParentItem(m_group);
    m_scene->addItem(btnA1);
    m_scene->addItem(btnA2);
    m_items.push_back(btnA1);
    m_items.push_back(btnA2);

    // sygnały przycisków → żądania do „mózgu”
    QObject::connect(btnA1, &SchematicButton::toggled, this, [this, pA1](bool on){
        if (on) emit requestAddPhase(pA1);
        else    emit requestRemovePhase(pA1);
    });
    QObject::connect(btnA2, &SchematicButton::toggled, this, [this, pA2](bool on){
        if (on) emit requestAddNeutral(pA2);
        else    emit requestRemoveNeutral(pA2);
    });

    // Boczne bloki i styki pomocnicze (LADC22)
    const QRectF rLC(BODY_X + PX(20),           BODY_Y + PX(35), BODY_W/2 - PX(85), BODY_H - PX(70));
    const QRectF rLA(BODY_X + BODY_W/2 + PX(5), BODY_Y + PX(35), BODY_W/2 - PX(20), BODY_H - PX(70));
    auto* r1 = m_scene->addRect(rLC, penBlock(1.6)); r1->setParentItem(m_group); m_items.push_back(r1);
    auto* r2 = m_scene->addRect(rLA, penBlock(1.6)); r2->setParentItem(m_group); m_items.push_back(r2);
    auto* n1 = addText(m_scene, "LC1D09", rLC.bottomLeft() + QPointF(PX(6), -PX(16)), 1.0); n1->setParentItem(m_group); m_items.push_back(n1);
    auto* n2 = addText(m_scene, "LADC22", rLA.bottomLeft() + QPointF(PX(6), -PX(16)), 1.0); n2->setParentItem(m_group); m_items.push_back(n2);

    const int auxGapV = PX(60);
    const int auxTopY = BODY_Y + PX(70);

    auto addPin = [&](const QString& name, const QPointF& pos){
        addPinCircle(name, pos);
    };

    // 53-54 (NO)
    addPin(m_prefix + "53", QPointF(int(rLA.left()) + PX(20),  auxTopY));
    addPin(m_prefix + "54", QPointF(int(rLA.left()) + PX(20),  auxTopY+auxGapV));
    auto* t53 = addText(m_scene, "53 NO", QPointF(int(rLA.left()) + PX(0), auxTopY-PX(24)), 0.9); t53->setParentItem(m_group); m_items.push_back(t53);

    // 61-62 (NC)
    addPin(m_prefix + "61", QPointF(int(rLA.left()) + PX(70),  auxTopY));
    addPin(m_prefix + "62", QPointF(int(rLA.left()) + PX(70),  auxTopY+auxGapV));
    auto* t61 = addText(m_scene, "61 NC", QPointF(int(rLA.left()) + PX(50), auxTopY-PX(24)), 0.9); t61->setParentItem(m_group); m_items.push_back(t61);

    // 75-76 (NC)
    addPin(m_prefix + "75", QPointF(int(rLA.left()) + PX(120), auxTopY));
    addPin(m_prefix + "76", QPointF(int(rLA.left()) + PX(120), auxTopY+auxGapV));
    auto* t75 = addText(m_scene, "75 NC", QPointF(int(rLA.left()) + PX(100), auxTopY-PX(24)), 0.9); t75->setParentItem(m_group); m_items.push_back(t75);

    // 87-88 (NO)
    addPin(m_prefix + "87", QPointF(int(rLA.left()) + PX(170), auxTopY));
    addPin(m_prefix + "88", QPointF(int(rLA.left()) + PX(170), auxTopY+auxGapV));
    auto* t87 = addText(m_scene, "87 NO", QPointF(int(rLA.left()) + PX(150), auxTopY-PX(24)), 0.9); t87->setParentItem(m_group); m_items.push_back(t87);
}

void Contactor_LC1D09_LADC22::destroy()
{
    if (!m_scene) return;

    for (QGraphicsItem* it : std::as_const(m_items)) {
        if (!it) continue;
        if (it->scene()) it->scene()->removeItem(it);
        delete it;
    }
    m_items.clear();

    if (m_group) {
        if (m_group->scene()) m_group->scene()->removeItem(m_group);
        delete m_group;
        m_group = nullptr;
    }
}

QGraphicsItem* createContactor_LC1D09_LADC22(QGraphicsScene* scene,
                                             const QString& prefix,
                                             const QPointF& topLeft)
{
    auto* c = new Contactor_LC1D09_LADC22(scene, prefix, topLeft);
    return c->rootItem();
}

#include "contactor_LC1D09_LADC22.moc"  // dla SchematicButton (Q_OBJECT) — jeśli używasz AUTOMOC, możesz usunąć
