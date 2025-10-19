#include "contactor_view.h"
#include "power_block.h"
#include "contactor_LC1D09_LADC22.h"
#include "motor_3phase_block.h"    // **NOWE**

#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsSimpleTextItem>
#include <QPen>
#include <QBrush>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QContextMenuEvent>
#include <QAction>
#include <QMouseEvent>
#include <QCursor>
#include <cmath>

constexpr qreal S = 0.7;
static inline int   PX(qreal v) { return int(std::lround(v * S)); }
static inline qreal PQ(qreal v) { return            (v * S); }
static inline qreal   snap1(qreal v, qreal g) { return std::round(v/g)*g; }
static inline QPointF snapPt(const QPointF& p, qreal g) { return QPointF(snap1(p.x(), g), snap1(p.y(), g)); }

QColor ContactorView::colBg()       { return QColor(18, 30, 46); }
QColor ContactorView::colWire()     { return QColor(220, 230, 245); }
QColor ContactorView::colBlock()    { return QColor(200, 40, 40); }
QColor ContactorView::colDash()     { return QColor(200, 210, 225); }
QColor ContactorView::colText()     { return QColor(190, 170, 255); }
QColor ContactorView::colOn()       { return QColor(53, 199, 89); }
QColor ContactorView::colPhase()    { return QColor(220, 60, 60); }
QColor ContactorView::colNeutral()  { return QColor(66, 133, 244); }
QColor ContactorView::colFault()    { return QColor(255, 190, 0); }

QPen ContactorView::penWire(qreal w)  { QPen p(colWire());  p.setWidthF(w); return p; }
QPen ContactorView::penBlock(qreal w) { QPen p(colBlock()); p.setWidthF(w); return p; }
QPen ContactorView::penDash(qreal w)  { QPen p(colDash());  p.setWidthF(w); p.setStyle(Qt::DashLine); return p; }

// ===== SchematicButton =====
SchematicButton::SchematicButton(const QRectF& r, const QString& text, QGraphicsItem* parent, const QColor& onColor)
    : QGraphicsObject(parent), m_rect(r), m_text(text), m_onColor(onColor) {
    setAcceptedMouseButtons(Qt::LeftButton);
    setCursor(Qt::PointingHandCursor);
}
void SchematicButton::setOn(bool on) { if (m_on != on) { m_on = on; update(); } }
void SchematicButton::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) {
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(QPen(ContactorView::colWire(), 1.6));
    p->setBrush(m_on ? QBrush(m_onColor) : QBrush(Qt::NoBrush));
    p->drawRoundedRect(m_rect, PX(5), PX(5));
    p->setPen(QPen(ContactorView::colWire()));
    p->drawText(m_rect, Qt::AlignCenter, m_text);
}
void SchematicButton::mousePressEvent(QGraphicsSceneMouseEvent* e) {
    if (e->button() == Qt::LeftButton) { m_on = !m_on; update(); emit toggled(m_on); }
    QGraphicsObject::mousePressEvent(e);
}

// ===== ContactorView =====
ContactorView::ContactorView(QWidget* parent, bool buildDefault)
    : QGraphicsView(parent), m_scene(new QGraphicsScene(this)) {
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing, true);
    setBackgroundBrush(colBg());
    setMouseTracking(true);
    if (viewport()) viewport()->setMouseTracking(true);
    m_scene->setSceneRect(0, 0, 5000, 3000);
    if (buildDefault) buildScene();
}

static QGraphicsSimpleTextItem* addText(QGraphicsScene* sc, const QString& t, const QPointF& pos, qreal scale = 1.0) {
    auto* it = sc->addSimpleText(t);
    it->setBrush(ContactorView::colText());
    it->setScale(scale * S);
    it->setPos(pos);
    it->setZValue(2);
    return it;
}

QGraphicsEllipseItem* ContactorView::addTerminal(const QString& name, const QPointF& c) {
    const int r = PX(10);
    auto* e = m_scene->addEllipse(c.x() - r, c.y() - r, 2*r, 2*r,
                                  penWire(1.6), QBrush(Qt::NoBrush));
    e->setToolTip(name);
    e->setZValue(1.1);
    m_terms.insert(name, e);
    // podczas budowy — przypisz pin do aktualnej grupy
    if (!m_buildingK.isEmpty()) { m_contactors[m_buildingK].pins.insert(name); trackItem(e); }
    if (!m_buildingP.isEmpty()) { m_powers[m_buildingP].pins.insert(name);    trackItem(e); }
    return e;
}
QGraphicsLineItem* ContactorView::addLine(const QString&, const QPointF& p1, const QPointF& p2, Qt::PenStyle style) {
    QPen pen = (style == Qt::DashLine) ? penDash(1.2) : penWire(1.6);
    auto* l = m_scene->addLine(QLineF(p1, p2), pen);
    l->setZValue(0.5);
    trackItem(l);
    return l;
}
void ContactorView::setTerminalOn(const QString& name, bool on) {
    if (auto* e = m_terms.value(name, nullptr)) {
        e->setBrush(on ? QBrush(colOn()) : QBrush(Qt::NoBrush));
        e->setPen(penWire(1.6));
    }
}
void ContactorView::setTerminalPhase(const QString& name, bool on) {
    if (auto* e = m_terms.value(name, nullptr)) {
        e->setBrush(on ? QBrush(colPhase()) : QBrush(Qt::NoBrush));
        e->setPen(penWire(1.6));
    }
    // **NOWE**: informuj słuchaczy (Motor3PhaseBlock) o zmianie „fazowości”
    emit terminalPhaseChanged(name, on);
}
void ContactorView::setTerminalNeutral(const QString& name, bool on) {
    if (auto* e = m_terms.value(name, nullptr)) {
        e->setBrush(on ? QBrush(colNeutral()) : QBrush(Qt::NoBrush));
        e->setPen(penWire(1.6));
    }
}
void ContactorView::setTerminalHot(const QString& name, bool on) { setTerminalPhase(name, on); }
void ContactorView::registerAuxPair(const QString& id, const QString& upper, const QString& lower, bool isNO) {
    Q_UNUSED(id); Q_UNUSED(upper); Q_UNUSED(lower); Q_UNUSED(isNO);
}

// ---------- RYSOWANIE: stycznik (Contactor_LC1D09_LADC22) ----------
void ContactorView::drawSingleContactorAt(const QPointF& off, uint8_t idx) {
    const QString K = QStringLiteral("K%1_").arg(idx);

    // Na czas budowy — pozwól addTerminal/trackItem wpisać piny/itemy do grupy K
    m_buildingK = K; m_contactors[K];

    auto addTerm = [this](const QString& n, const QPointF& c){ return this->addTerminal(n, c); };
    auto addLine = [this](const QPointF& a, const QPointF& b, Qt::PenStyle st){ return this->addLine("", a, b, st); };
    auto track   = [this](QGraphicsItem* it){ this->trackItem(it); };

    auto* kb = new Contactor_LC1D09_LADC22(m_scene, K, off, addTerm, addLine, track, this);

    // Po zbudowaniu — koniec trybu
    m_buildingK.clear();

    // Forward sygnałów START/RET
    m_contBlocks.insert(K, kb);
    connect(kb, &Contactor_LC1D09_LADC22::requestAddPhase,      this, &ContactorView::addPhaseSourceRequested);
    connect(kb, &Contactor_LC1D09_LADC22::requestRemovePhase,   this, &ContactorView::removePhaseSourceRequested);
    connect(kb, &Contactor_LC1D09_LADC22::requestAddNeutral,    this, &ContactorView::addNeutralSourceRequested);
    connect(kb, &Contactor_LC1D09_LADC22::requestRemoveNeutral, this, &ContactorView::removeNeutralSourceRequested);

    emit contactorPlaced(K);
}

// ---------- RYSOWANIE: blok zasilania 3F (PowerBlock) ----------
void ContactorView::drawPower3At(const QPointF& off, uint8_t idx) {
    const QString P = QStringLiteral("P%1_").arg(idx);

    // Na czas budowy — pozwól addTerminal/trackItem wpisać piny/itemy do grupy P
    m_buildingP = P; m_powers[P];

    auto addTerm = [this](const QString& n, const QPointF& c){ return this->addTerminal(n, c); };
    auto addLine = [this](const QPointF& a, const QPointF& b){ return this->addLine("", a, b, Qt::SolidLine); };
    auto track   = [this](QGraphicsItem* it){ this->trackItem(it); };

    auto* pb = new PowerBlock(m_scene, P, off, addTerm, addLine, track, this);

    // Po zbudowaniu — koniec trybu
    m_buildingP.clear();

    // Forward przycisku POWER
    m_powerBlocks.insert(P, pb);
    connect(pb, &PowerBlock::requestAddPhase,    this, &ContactorView::addPhaseSourceRequested);
    connect(pb, &PowerBlock::requestRemovePhase, this, &ContactorView::removePhaseSourceRequested);

    emit powerPlaced(P);
}

// ---------- RYSOWANIE: silnik 3F (Motor3PhaseBlock) ----------
void ContactorView::drawMotorAt(const QPointF& off, uint8_t idx) {
    const QString M = QStringLiteral("M%1_").arg(idx);

    // Dedykowane trackowanie do map „M”
    m_motors[M];

    auto addTerm = [this,M](const QString& n, const QPointF& c){
        auto* e = this->addTerminal(n, c);
        m_motors[M].pins.insert(n);
        m_itemToM.insert(e, M);
        m_motors[M].items.push_back(e);
        return e;
    };
    auto addLine = [this,M](const QPointF& a, const QPointF& b){
        auto* l = this->addLine("", a, b, Qt::SolidLine);
        m_itemToM.insert(l, M);
        m_motors[M].items.push_back(l);
        return l;
    };
    auto track   = [this,M](QGraphicsItem* it){
        if (!it) return;
        m_itemToM.insert(it, M);
        m_motors[M].items.push_back(it);
    };

    // proste zapytanie o bezpośredni mostek
    auto bridgeQuery = [this](const QString& pin)->QString{
        for (auto it = m_bridgeToPins.constBegin(); it != m_bridgeToPins.constEnd(); ++it) {
            const auto& pr = it.value();
            if (pr.first == pin)  return pr.second;
            if (pr.second == pin) return pr.first;
        }
        return {};
    };

    auto* mb = new Motor3PhaseBlock(m_scene, this, M, off, addTerm, addLine, track, bridgeQuery, this);
    m_motorBlocks.insert(M, mb);
}

void ContactorView::buildScene() {}

// legacy slots
void ContactorView::setMainClosed(bool closed) { for (const QString& n : {"L1","L2","L3"}) setTerminalOn(n, closed); }
void ContactorView::setAuxNOClosed(bool closed){ for (const QString& n : {"14","13","53","54","87","88"}) setTerminalOn(n, closed); }
void ContactorView::setAuxNCClosed(bool closed){ for (const QString& n : {"22","21","61","62","75","76"}) setTerminalOn(n, closed); }
void ContactorView::setA1On(bool on){ setTerminalPhase("A1", on); if (m_btnA1) m_btnA1->setOn(on); }
void ContactorView::setA2On(bool on){ setTerminalNeutral("A2", on); if (m_btnA2) m_btnA2->setOn(on); }
void ContactorView::setSupplyOn(bool on){ for (const QString& n : {"T1","T2","T3"}) setTerminalOn(n, on); }
void ContactorView::setMainBClosed(bool closed){ for (const QString& n : {"L1b","L2b","L3b"}) setTerminalOn(n, closed); }
void ContactorView::setAuxNOBClosed(bool closed){ for (const QString& n : {"14b","13b","53b","54b","87b","88b"}) setTerminalOn(n, closed); }
void ContactorView::setAuxNCBClosed(bool closed){ for (const QString& n : {"22b","21b","61b","62b","75b","76"}) setTerminalOn(n, closed); }
void ContactorView::setA1BOn(bool on){ setTerminalPhase("A1b", on); if (m_btnA1b) m_btnA1b->setOn(on); }
void ContactorView::setA2BOn(bool on){ setTerminalNeutral("A2b", on); if (m_btnA2b) m_btnA2b->setOn(on); }
void ContactorView::setUpperSupplyOn(bool on){ for (const QString& n : {"T1b","T2b","T3b"}) setTerminalOn(n, on); }

// Zwarcie marker (krzyżyk nad pinem)
void ContactorView::setTerminalFault(const QString& name, bool on) {
    auto* e = m_terms.value(name, nullptr); if (!e) return;
    auto& vec = m_faultMarks[name];
    if (vec.isEmpty()) {
        const QRectF r = e->rect().translated(e->pos());
        const QPointF c = r.center();
        const qreal d = 18.0 * S;
        const QPen pen(colFault(), 2.4);
        auto* l1 = m_scene->addLine(QLineF(c.x()-d, c.y()-d, c.x()+d, c.y()+d), pen);
        auto* l2 = m_scene->addLine(QLineF(c.x()-d, c.y()+d, c.x()+d, c.y()-d), pen);
        l1->setZValue(3); l2->setZValue(3);
        l1->setVisible(false); l2->setVisible(false);
        vec = { l1, l2 };
        trackItem(l1);
        trackItem(l2);
    }
    for (auto* l : vec) if (l) l->setVisible(on);
}

// API mostków
QString ContactorView::terminalAt(const QPointF& scenePos, qreal radius) const {
    const qreal r2 = radius * radius;
    for (auto it = m_terms.constBegin(); it != m_terms.constEnd(); ++it) {
        auto* e = it.value(); if (!e) continue;
        const QPointF c = e->sceneBoundingRect().center();
        const qreal dx = c.x() - scenePos.x();
        const qreal dy = c.y() - scenePos.y();
        if ((dx*dx + dy*dy) <= r2) return it.key();
    }
    return {};
}
QPointF ContactorView::terminalPos(const QString& name) const {
    if (auto* e = m_terms.value(name, nullptr)) return e->sceneBoundingRect().center();
    return {};
}
QGraphicsPathItem* ContactorView::addBridgePolyline(const QVector<QPointF>& pts) {
    if (pts.size() < 2 || !m_scene) return nullptr;
    QPainterPath ph(pts.front()); for (int i = 1; i < pts.size(); ++i) ph.lineTo(pts[i]);
    auto* item = m_scene->addPath(ph, penWire(2.2));
    item->setZValue(1.2);
    item->setAcceptedMouseButtons(Qt::RightButton);
    m_bridges.push_back(item);
    return item;
}
void ContactorView::registerBridge(QGraphicsPathItem* item, const QString& aPin, const QString& bPin) {
    if (!item) return; m_bridgeToPins.insert(item, qMakePair(aPin, bPin));
}
void ContactorView::removeBridgeItem(QGraphicsPathItem* item) {
    if (!item) return; m_bridgeToPins.remove(item); m_bridges.removeOne(item);
    if (m_scene) m_scene->removeItem(item); delete item;
}

// Tryby wstawiania
void ContactorView::beginPlaceContactor() {
    if (!m_scene) return;
    m_placeContactor = true; m_placePower3 = false; m_placeMotor3 = false;

    const int BODY_W        = PX(420);
    const int BODY_H        = PX(320);
    const int MARGIN_TOP_L  = PX(60);
    const int MARGIN_BOT_T  = PX(60);
    const int W = BODY_W + PX(140);
    const int H = BODY_H + MARGIN_TOP_L + MARGIN_BOT_T + PX(40);

    if (!m_contGhost) {
        m_contGhost = m_scene->addRect(0, 0, W, H, penDash(1.6), QBrush(Qt::NoBrush));
        m_contGhost->setZValue(3.0);
    } else {
        m_contGhost->setRect(0, 0, W, H);
        m_contGhost->setVisible(true);
    }

    const qreal grid = 10.0;
    QPointF sp = mapToScene(mapFromGlobal(QCursor::pos()));
    sp = snapPt(sp, grid);
    m_contGhost->setPos(sp - QPointF(W/2.0, H/2.0));

    setCursor(Qt::CrossCursor);
    setMouseTracking(true);
    if (viewport()) viewport()->setMouseTracking(true);
}

void ContactorView::beginPlacePower3() {
    if (!m_scene) return;
    m_placePower3 = true; m_placeContactor = false; m_placeMotor3 = false;

    const int W = PX(260);
    const int H = PX(170);
    if (!m_contGhost) {
        m_contGhost = m_scene->addRect(0, 0, W, H, penDash(1.6), QBrush(Qt::NoBrush));
        m_contGhost->setZValue(3.0);
    } else {
        m_contGhost->setRect(0, 0, W, H);
        m_contGhost->setVisible(true);
    }

    const qreal grid = 10.0;
    QPointF sp = mapToScene(mapFromGlobal(QCursor::pos()));
    sp = snapPt(sp, grid);
    m_contGhost->setPos(sp - QPointF(W/2.0, H/2.0));

    setCursor(Qt::CrossCursor);
    setMouseTracking(true);
    if (viewport()) viewport()->setMouseTracking(true);
}

void ContactorView::beginPlaceMotor3() {
    if (!m_scene) return;
    m_placeMotor3 = true; m_placeContactor = false; m_placePower3 = false;

    const int W = PX(220);
    const int H = PX(180);
    if (!m_contGhost) {
        m_contGhost = m_scene->addRect(0, 0, W, H, penDash(1.6), QBrush(Qt::NoBrush));
        m_contGhost->setZValue(3.0);
    } else {
        m_contGhost->setRect(0, 0, W, H);
        m_contGhost->setVisible(true);
    }

    const qreal grid = 10.0;
    QPointF sp = mapToScene(mapFromGlobal(QCursor::pos()));
    sp = snapPt(sp, grid);
    m_contGhost->setPos(sp - QPointF(W/2.0, H/2.0));

    setCursor(Qt::CrossCursor);
    setMouseTracking(true);
    if (viewport()) viewport()->setMouseTracking(true);
}

void ContactorView::mouseMoveEvent(QMouseEvent* e) {
    if ((m_placeContactor || m_placePower3 || m_placeMotor3) && m_contGhost) {
        const qreal grid = 10.0;
        QPointF sp = mapToScene(e->pos());
        sp = snapPt(sp, grid);
        QRectF r = m_contGhost->rect();
        m_contGhost->setPos(sp - QPointF(r.width()/2, r.height()/2));
    }
    QGraphicsView::mouseMoveEvent(e);
}
void ContactorView::mousePressEvent(QMouseEvent* e) {
    if ((m_placeContactor || m_placePower3 || m_placeMotor3) && e->button() == Qt::LeftButton) {
        const qreal grid = 10.0;
        QPointF sp = mapToScene(e->pos());
        sp = snapPt(sp, grid);
        const QSizeF  gs = m_contGhost ? m_contGhost->rect().size() : QSizeF(0,0);
        const QPointF pos = sp - QPointF(gs.width()/2.0, gs.height()/2.0);

        if (m_placeContactor) {
            const uint8_t idx = m_nextK; drawSingleContactorAt(pos, idx); if (m_nextK < 255) m_nextK++;
        } else if (m_placePower3) {
            const uint8_t idx = m_nextP; drawPower3At(pos, idx); if (m_nextP < 255) m_nextP++;
        } else if (m_placeMotor3) {
            const uint8_t idx = m_nextM; drawMotorAt(pos, idx);   if (m_nextM < 255) m_nextM++;
        }

        if (m_contGhost) m_contGhost->setVisible(false);
        m_placeContactor = m_placePower3 = m_placeMotor3 = false;
        unsetCursor();
        e->accept();
        return;
    }
    QGraphicsView::mousePressEvent(e);
}

// PPM
void ContactorView::contextMenuEvent(QContextMenuEvent* e) {
    QGraphicsItem* item = itemAt(e->pos());
    if (!item) return;

    // Jeśli klik na mostek — menu mostka
    if (auto* path = qgraphicsitem_cast<QGraphicsPathItem*>(item)) {
        if (m_bridgeToPins.contains(path)) {
            QMenu menu;
            QAction* del = menu.addAction("Usuń mostek");
            QAction* chosen = menu.exec(e->globalPos());
            if (chosen == del) {
                const auto pins = m_bridgeToPins.value(path);
                emit bridgeDeleteRequested(pins.first, pins.second, path);
            }
            return;
        }
    }

    // Rozpoznaj pin (jeśli trafiony)
    QString pinName;
    for (auto it = m_terms.constBegin(); it != m_terms.constEnd(); ++it) {
        if (it.value() == item) { pinName = it.key(); break; }
    }

    // Rozpoznaj grupy
    const QString kPrefix = kOfItem(item);
    const QString pPrefix = pOfItem(item);
    const QString mPrefix = mOfItem(item);   // **NOWE**

    QMenu menu;
    QAction* addF = nullptr;
    QAction* addN = nullptr;
    QAction* remF = nullptr;
    QAction* remN = nullptr;
    if (!pinName.isEmpty()) {
        addF = menu.addAction("Dodaj źródło: FAZA");
        addN = menu.addAction("Dodaj źródło: ZERO");
        remF = menu.addAction("Usuń źródło: FAZA");
        remN = menu.addAction("Usuń źródło: ZERO");
        menu.addSeparator();
    }
    QAction* delK = nullptr;
    QAction* delP = nullptr;
    QAction* delM = nullptr;  // **NOWE**
    if (!kPrefix.isEmpty()) {
        delK = menu.addAction(QStringLiteral("Usuń stycznik %1").arg(kPrefix.left(kPrefix.size()-1)));
    }
    if (!pPrefix.isEmpty()) {
        delP = menu.addAction(QStringLiteral("Usuń zasilanie %1").arg(pPrefix.left(pPrefix.size()-1)));
    }
    if (!mPrefix.isEmpty()) {
        delM = menu.addAction(QStringLiteral("Usuń silnik %1").arg(mPrefix.left(mPrefix.size()-1)));
    }

    QAction* chosen = menu.exec(e->globalPos());
    if (!chosen) return;

    if      (chosen == addF) emit addPhaseSourceRequested(pinName);
    else if (chosen == addN) emit addNeutralSourceRequested(pinName);
    else if (chosen == remF) emit removePhaseSourceRequested(pinName);
    else if (chosen == remN) emit removeNeutralSourceRequested(pinName);
    else if (chosen == delK) emit contactorDeleteRequested(kPrefix);
    else if (chosen == delP) emit powerDeleteRequested(pPrefix);
    else if (chosen == delM) removeMotor(mPrefix);  // **NOWE**
}

// --- usuwanie z widoku ---
void ContactorView::removeContactor(const QString& kPrefix) {
    if (!m_contactors.contains(kPrefix)) return;

    // Usuń mostki związane z tym K
    QList<QGraphicsPathItem*> toRemove;
    for (auto it = m_bridgeToPins.begin(); it != m_bridgeToPins.end(); ++it) {
        const auto& pr = it.value();
        if (pr.first.startsWith(kPrefix) || pr.second.startsWith(kPrefix)) {
            toRemove.push_back(it.key());
        }
    }
    for (auto* p : toRemove) removeBridgeItem(p);

    // Usuń itemy graficzne
    auto grp = m_contactors.take(kPrefix);
    for (auto* it : std::as_const(grp.items)) {
        m_itemToK.remove(it);
        if (m_scene) m_scene->removeItem(it);
        delete it;
    }
    // Usuń piny
    for (const QString& pin : std::as_const(grp.pins)) {
        m_terms.remove(pin);
        m_faultMarks.remove(pin);
    }

    // porządek: usuń obiekt stycznika, jeśli był śledzony
    if (auto* cb = m_contBlocks.take(kPrefix)) delete cb;
}

void ContactorView::removePowerBlock(const QString& pPrefix) {
    auto* pb = m_powerBlocks.value(pPrefix, nullptr);
    if (!pb) return;

    // 1) Usuń mostki związane z pinami tego bloku
    QList<QGraphicsPathItem*> toRemove;
    for (auto it = m_bridgeToPins.begin(); it != m_bridgeToPins.end(); ++it) {
        const auto& pr = it.value();
        if (pr.first.startsWith(pPrefix) || pr.second.startsWith(pPrefix)) {
            toRemove.push_back(it.key());
        }
    }
    for (auto* p : toRemove) removeBridgeItem(p);

    // 2) Wyczyść mapy pinów (NIE dotykamy już sceny dla elips — będą skasowane w kroku 3)
    for (const QString& pin : m_powers[pPrefix].pins) {
        m_terms.remove(pin);
        m_faultMarks.remove(pin);
    }

    // 3) Usuń elementy graficzne JEDEN raz (w tym elipsy pinów, bo są w items())
    for (auto* it : m_powers[pPrefix].items) {
        m_itemToP.remove(it);
        if (it && it->scene() == m_scene) {
            m_scene->removeItem(it);
        }
        delete it;
    }

    m_powers.remove(pPrefix);
    if (auto* pbObj = m_powerBlocks.take(pPrefix)) delete pbObj;
}

void ContactorView::removeMotor(const QString& mPrefix) {
    if (!m_motors.contains(mPrefix)) return;

    // Usuń mostki zaczepione do pinów tego silnika
    QList<QGraphicsPathItem*> toRemove;
    for (auto it = m_bridgeToPins.begin(); it != m_bridgeToPins.end(); ++it) {
        const auto& pr = it.value();
        if (pr.first.startsWith(mPrefix) || pr.second.startsWith(mPrefix)) {
            toRemove.push_back(it.key());
        }
    }
    for (auto* p : toRemove) removeBridgeItem(p);

    // Wyczyść mapy pinów
    for (const QString& pin : std::as_const(m_motors[mPrefix].pins)) {
        m_terms.remove(pin);
        m_faultMarks.remove(pin);
    }

    // Usuń itemy graficzne
    for (auto* it : std::as_const(m_motors[mPrefix].items)) {
        m_itemToM.remove(it);
        if (it && it->scene() == m_scene) m_scene->removeItem(it);
        delete it;
    }

    // Usuń obiekt i wpisy
    if (auto* mb = m_motorBlocks.take(mPrefix)) delete mb;
    m_motors.remove(mPrefix);
}

// --- pomocnicze: śledzenie itemów i odwzorowania item->grupa ---
void ContactorView::trackItem(QGraphicsItem* it) {
    if (!it) return;
    if (!m_buildingK.isEmpty()) {
        m_contactors[m_buildingK].items.push_back(it);
        m_itemToK.insert(it, m_buildingK);
    }
    if (!m_buildingP.isEmpty()) {
        m_powers[m_buildingP].items.push_back(it);
        m_itemToP.insert(it, m_buildingP);
    }
}

void ContactorView::setMotorPhaseMasks(const QString& motorPrefix, int mU, int mV, int mW)
{
    auto it = m_motorBlocks.find(motorPrefix);
    if (it != m_motorBlocks.end() && it.value()) {
        it.value()->syncPhaseMasks(mU, mV, mW);
    }
}


QString ContactorView::kOfItem(QGraphicsItem* it) const {
    if (!it) return {};
    auto itK = m_itemToK.constFind(it);
    if (itK != m_itemToK.constEnd()) return itK.value();
    return {};
}
QString ContactorView::pOfItem(QGraphicsItem* it) const {
    if (!it) return {};
    auto itP = m_itemToP.constFind(it);
    if (itP != m_itemToP.constEnd()) return itP.value();
    return {};
}
QString ContactorView::mOfItem(QGraphicsItem* it) const {
    if (!it) return {};
    auto itM = m_itemToM.constFind(it);
    if (itM != m_itemToM.constEnd()) return itM.value();
    return {};
}
