#include "mainwindow.h"
#include "wire_editor.h"
#include "propagation.h"

#include <QStatusBar>
#include <QGridLayout>
#include <QWidget>
#include <QQueue>
#include <QMenuBar>
#include <QMenu>
#include <utility>



// ===================== Konstruktor =====================
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    buildUi();

    // Edytor mostków
    auto* wireEdit = new WireEditor(m_view, this);
    connect(wireEdit, &WireEditor::wireCommitted, this,
            [this](const QString& aPin, const QString& bPin, const QVector<QPointF>&){
                addWire(aPin, bPin, []{ return true; });
                recomputeSignals();
            });

    // Usuwanie mostków (PPM na ścieżce)
    connect(m_view, &ContactorView::bridgeDeleteRequested, this,
            [this](const QString& aNode, const QString& bNode, QGraphicsPathItem* item){
                removeWire(aNode, bNode);
                m_view->removeBridgeItem(item);
                recomputeSignals();
            });

    // Źródła z PPM i z przycisków
    connect(m_view, &ContactorView::addPhaseSourceRequested, this, [this](const QString& pin){
        setPhaseSource(pin, true);
    });
    connect(m_view, &ContactorView::addNeutralSourceRequested, this, [this](const QString& pin){
        setNeutralSource(pin, true);
    });
    connect(m_view, &ContactorView::removePhaseSourceRequested, this, [this](const QString& pin){
        setPhaseSource(pin, false);
    });
    connect(m_view, &ContactorView::removeNeutralSourceRequested, this, [this](const QString& pin){
        setNeutralSource(pin, false);
    });

    // Wstawienie i usuwanie: styczniki
    connect(m_view, &ContactorView::contactorPlaced,           this, &MainWindow::onContactorPlaced);
    connect(m_view, &ContactorView::contactorDeleteRequested,  this, &MainWindow::onContactorDelete);

    // NOWE: Wstawienie i usuwanie: zasilanie 3F
    connect(m_view, &ContactorView::powerPlaced,               this, &MainWindow::onPowerPlaced);
    connect(m_view, &ContactorView::powerDeleteRequested,      this, &MainWindow::onPowerDelete);

    // Start — pusto
    recomputeSignals();
    statusBar()->showMessage(tr("Gotowy"));
}

// ===================== UI =====================
void MainWindow::buildUi() {
    auto* central = new QWidget(this);
    auto* layout  = new QGridLayout(central);

    m_view = new ContactorView(central, true);
    layout->addWidget(m_view, 0, 0, 1, 1);

    // Menu
    auto* menuBar = new QMenuBar(this);
    auto* menuPlik = new QMenu(tr("Plik"), menuBar);
    auto* menuWstaw = new QMenu(tr("Wstaw"), menuBar);

    menuPlik->addAction(tr("Nowy schemat"), this, [this]{
        if (m_view && m_view->scene()) {
            m_view->scene()->clear();
            m_auxNodes.clear();
            m_nodeToView.clear();
            m_phaseSources.clear();
            m_neutralSources.clear();
            m_phaseHot.clear();
            m_neutralHot.clear();
            m_edges.clear();
            m_contactors.clear();
            m_contEnergized.clear();
            m_powers.clear();
            statusBar()->showMessage(tr("Nowy schemat"));
        }
    });
    menuPlik->addSeparator();
    menuPlik->addAction(tr("Zamknij"), this, &QWidget::close);

    // --- WSTAW ---
    menuWstaw->addAction(tr("Stycznik (Kx)"), this, [this]{
        if (m_view) m_view->beginPlaceContactor();
    });
    menuWstaw->addAction(tr("Zasilanie 3F (Px)"), this, [this]{   // NOWE
        if (m_view) m_view->beginPlacePower3();
    });
    menuWstaw->addAction(tr("Silnik 3F (Mx)"), this, [this]{        // NOWE
        if (m_view) m_view->beginPlaceMotor3();
    });

    menuBar->addMenu(menuPlik);
    menuBar->addMenu(menuWstaw);
    setMenuBar(menuBar);

    setCentralWidget(central);
    setWindowTitle(tr("ContactorSim — FAZA (czerwony) / ZERO (niebieski)"));
    resize(1200, 800);
}

// ===================== Sloty (placeholdery – brak paneli) =====================
void MainWindow::setLampState(class QLabel*, bool, const QString&, const QString&) {}
void MainWindow::updateCoilLamp(bool) {}
void MainWindow::updateMainContacts(ContactorModel::ContactState) {}
void MainWindow::updateAuxNO(ContactorModel::ContactState) {}
void MainWindow::updateAuxNC(ContactorModel::ContactState) {}
void MainWindow::refreshUpperFeed() {}
void MainWindow::addLink(const QString&, const QString&, std::function<bool()>) {}
void MainWindow::applyLinks() {}
void MainWindow::setupWiring() {}
std::function<bool()> MainWindow::resolveContact(const QString&) const { return {}; }
std::function<void(bool)> MainWindow::resolveCoilSetter(const QString&) { return {}; }

// ===================== LOGIKA: stycznik i krawędzie kontaktów =====================
void MainWindow::onContactorPlaced(const QString& K) {
    if (m_contactors.contains(K)) return;
    m_contactors.insert(K);
    m_contEnergized.insert(K, false);

    // Styk pomocniczy NO/NC
    addContactEdgeDyn(K, "13", "14", true);
    addContactEdgeDyn(K, "21", "22", false);
    addContactEdgeDyn(K, "53", "54", true);
    addContactEdgeDyn(K, "61", "62", false);
    addContactEdgeDyn(K, "75", "76", false);
    addContactEdgeDyn(K, "87", "88", true);

    // Tory mocy L↔T (NO, przewodzą gdy cewka jest zasilona)
    addContactEdgeDyn(K, "L1", "T1", true);
    addContactEdgeDyn(K, "L2", "T2", true);
    addContactEdgeDyn(K, "L3", "T3", true);

    // Rejestruj piny do malowania
    for (const char* p : {
             "13","14","21","22","53","54","61","62","75","76","87","88",
             "A1","A2",
             "L1","L2","L3","T1","T2","T3"
         }) {
        const QString node = K + p;
        m_auxNodes.insert(node);
        m_nodeToView.insert(node, node);
    }
    recomputeSignals();
}

void MainWindow::addContactEdgeDyn(const QString& K, const char* a, const char* b, bool isNO) {
    auto cond = [this, K, isNO]() -> bool {
        const bool en = m_contEnergized.value(K, false);
        return isNO ? en : !en;
    };
    addWire(K + a, K + b, cond);
}

// NOWE: zasilanie 3F — rejestracja pinów
void MainWindow::onPowerPlaced(const QString& P) {
    if (m_powers.contains(P)) return;
    m_powers.insert(P);

    for (const char* p : {"L1","L2","L3"}) {
        const QString node = P + p;
        m_auxNodes.insert(node);
        m_nodeToView.insert(node, node);
    }
    recomputeSignals();
}

// NOWE: kasowanie zasilania 3F
void MainWindow::onPowerDelete(const QString& P) {
    if (!m_powers.contains(P)) return;

    // 1) Usuń krawędzie związane z tym prefiksem (jeśli jakieś mostki logiczne by istniały)
    for (int i = m_edges.size() - 1; i >= 0; --i) {
        const Edge& e = m_edges[i];
        if (e.a.startsWith(P) || e.b.startsWith(P))
            m_edges.remove(i);
    }

    // 2) Usuń źródła FAZA/ZERO tego P
    auto filterOutWithPrefix = [&](QSet<QString>& set){
        for (auto it = set.begin(); it != set.end(); ) {
            if (it->startsWith(P)) it = set.erase(it);
            else ++it;
        }
    };
    filterOutWithPrefix(m_phaseSources);
    filterOutWithPrefix(m_neutralSources);
    filterOutWithPrefix(m_phaseHot);
    filterOutWithPrefix(m_neutralHot);

    // 3) Usuń pomocnicze węzły i mapowania widoku
    for (auto it = m_auxNodes.begin(); it != m_auxNodes.end(); ) {
        if (it->startsWith(P)) it = m_auxNodes.erase(it);
        else ++it;
    }
    for (auto it = m_nodeToView.begin(); it != m_nodeToView.end(); ) {
        if (it.key().startsWith(P)) it = m_nodeToView.erase(it);
        else ++it;
    }

    // 4) Usuń z rejestru
    m_powers.remove(P);

    // 5) Usuń grafikę
    if (m_view) m_view->removePowerBlock(P);

    // 6) Przelicz
    recomputeSignals();

    if (auto* sb = statusBar()) sb->showMessage(tr("Usunięto %1").arg(P.left(P.size()-1)));
}

// ===================== GRAF POŁĄCZEŃ =====================
void MainWindow::addWire(const QString& a, const QString& b, std::function<bool()> cond) {
    if (!cond) cond = []{ return true; };
    m_edges.push_back(Edge{a, b, cond});
    m_edges.push_back(Edge{b, a, cond});
    m_auxNodes.insert(a); m_auxNodes.insert(b);
    m_nodeToView.insert(a, a);
    m_nodeToView.insert(b, b);
}
void MainWindow::removeWire(const QString& a, const QString& b) {
    for (int i = m_edges.size()-1; i >= 0; --i) {
        const Edge& e = m_edges[i];
        if ((e.a == a && e.b == b) || (e.a == b && e.b == a))
            m_edges.remove(i);
    }
}

// ===================== ŹRÓDŁA =====================
void MainWindow::setPhaseSource(const QString& node, bool on) {
    if (on) m_phaseSources.insert(node);
    else    m_phaseSources.remove(node);
    m_auxNodes.insert(node);
    m_nodeToView.insert(node, node);
    recomputeSignals();
}
void MainWindow::setNeutralSource(const QString& node, bool on) {
    if (on) m_neutralSources.insert(node);
    else    m_neutralSources.remove(node);
    m_auxNodes.insert(node);
    m_nodeToView.insert(node, node);
    recomputeSignals();
}
bool MainWindow::isNodePhaseHot(const QString& node) const { return m_phaseHot.contains(node); }
bool MainWindow::isNodeNeutralHot(const QString& node) const { return m_neutralHot.contains(node); }


// ===================== PROPAGACJA z iteracją =====================
void MainWindow::recomputeSignals() {
    auto paintClear = [&](){
        if (!m_view) return;
        for (const QString& nView : std::as_const(m_auxNodes)) {
            m_view->setTerminalNeutral(nView, false);
            m_view->setTerminalPhase(nView,  false);
            m_view->setTerminalFault(nView,  false);
        }
    };

    // --- iteracja: hot-sets → energizacja styczników → aż do stabilizacji
    const int MAX_IT = 12;
    for (int it = 0; it < MAX_IT; ++it) {
        const HotResult hot = computeHot(m_edges, m_phaseSources, m_neutralSources);
        m_phaseHot   = hot.phaseHot;
        m_neutralHot = hot.neutralHot;

        bool changed = false;
        for (const QString& K : std::as_const(m_contactors)) {
            const bool en = m_phaseHot.contains(K + "A1") && m_neutralHot.contains(K + "A2");
            if (m_contEnergized.value(K, false) != en) {
                m_contEnergized[K] = en;
                changed = true;
            }
        }
        if (!changed) break;
        if (it == MAX_IT - 1) break; // bezpiecznik
    }

    // --- malowanie + zwarcia L/N
    paintClear();
    if (m_view) {
        QStringList faultPins;
        for (auto it = m_nodeToView.constBegin(); it != m_nodeToView.constEnd(); ++it) {
            const QString& node = it.key();
            const QString& pin  = it.value();
            const bool nHot = m_neutralHot.contains(node);
            const bool pHot = m_phaseHot.contains(node);
            const bool fault = nHot && pHot;
            if (nHot) m_view->setTerminalNeutral(pin, true);
            if (pHot) m_view->setTerminalPhase(pin,  true);
            m_view->setTerminalFault(pin, fault);
            if (fault) faultPins << pin;
        }
        if (auto* sb = statusBar()) {
            if (faultPins.isEmpty()) sb->showMessage(tr("Brak zwarć"));
            else sb->showMessage(tr("Zwarcie na: %1").arg(faultPins.join(", ")));
        }
    }

    // --- zwarcie międzyfazowe (aktywny tor) — analiza bitmask
    const QSet<QString> interPhase = computeInterPhaseFault(m_edges, m_phaseSources);
    if (!interPhase.isEmpty()) {
        QStringList list;
        for (const QString& pinNode : interPhase) {
            const QString pinView = m_nodeToView.value(pinNode, pinNode);
            if (m_view) m_view->setTerminalFault(pinView, true);
            list << pinNode;
        }
        if (auto* sb = statusBar())
            sb->showMessage(tr("Zwarcie międzyfazowe (aktywny tor) na: %1").arg(list.join(", ")));
    }

    // --- NOWE: maski faz na węzłach + podanie do silników w KAŻDEJ rundzie
    const QHash<QString,int> phaseMask = computePhaseMask(m_edges, m_phaseSources);

    // helper do pobrania maski faz na konkretnym pinie (domyślnie 0 = brak fazy)
    auto mOf = [&](const QString& node) -> int {
        return phaseMask.value(node, 0);
    };

    // WYPISZ MASKI DO SILNIKÓW:
    // Jeśli masz rejestr silników, iteruj po nim. Poniżej przykład dla jednego „M1_”.
    auto pushMotor = [&](const QString& pref){
        // dopasuj nazwy pinów do Twojego Motor3PhaseBlock (U/V/W lub T1/T2/T3)
        const int mU = mOf(pref + "U");
        const int mV = mOf(pref + "V");
        const int mW = mOf(pref + "W");
        if (m_view) {
            // metoda w ContactorView, która przekaże dalej do Motor3PhaseBlock::syncPhaseMasks(...)
            m_view->setMotorPhaseMasks(pref, mU, mV, mW);
        }
    };

    // TODO: zamień na pętlę po wszystkich silnikach, jeśli je rejestrujesz
    pushMotor("M1_");
    // np. dla drugiego: pushMotor("M2_");
}


// ===================== Kasowanie stycznika (istniejące) =====================
void MainWindow::onContactorDelete(const QString& K) {
    if (!m_contactors.contains(K)) return;

    for (int i = m_edges.size() - 1; i >= 0; --i) {
        const Edge& e = m_edges[i];
        if (e.a.startsWith(K) || e.b.startsWith(K)) {
            m_edges.remove(i);
        }
    }
    auto filterOutWithPrefix = [&](QSet<QString>& set){
        for (auto it = set.begin(); it != set.end(); ) {
            if (it->startsWith(K)) it = set.erase(it);
            else ++it;
        }
    };
    filterOutWithPrefix(m_phaseSources);
    filterOutWithPrefix(m_neutralSources);
    filterOutWithPrefix(m_phaseHot);
    filterOutWithPrefix(m_neutralHot);

    for (auto it = m_auxNodes.begin(); it != m_auxNodes.end(); ) {
        if (it->startsWith(K)) it = m_auxNodes.erase(it);
        else ++it;
    }
    for (auto it = m_nodeToView.begin(); it != m_nodeToView.end(); ) {
        if (it.key().startsWith(K)) it = m_nodeToView.erase(it);
        else ++it;
    }

    m_contactors.remove(K);
    m_contEnergized.remove(K);

    if (m_view) m_view->removeContactor(K);

    recomputeSignals();

    if (auto* sb = statusBar()) sb->showMessage(tr("Usunięto %1").arg(K.left(K.size()-1)));
}
