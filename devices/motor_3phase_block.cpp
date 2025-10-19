#include "motor_3phase_block.h"
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsSimpleTextItem>
#include <QVariantAnimation>
#include <utility> // std::move

namespace {
// pomocnik: zamiana maski 0x1/0x2/0x4 na indeks 0/1/2; -1 jeśli brak/niejednoznaczne
inline int maskToIdx(int m) {
    if (m == 0x1) return 0; // L1
    if (m == 0x2) return 1; // L2
    if (m == 0x4) return 2; // L3
    return -1;
}
}

Motor3PhaseBlock::Motor3PhaseBlock(QGraphicsScene* scene,
                                   ContactorView*  view,
                                   const QString&  prefix,
                                   const QPointF&  topLeft,
                                   AddTerminalFn   addTerminal,
                                   AddLineFn       addLine,
                                   TrackFn         track,
                                   BridgeQueryFn   bridgeQuery,
                                   QObject* parent)
    : QObject(parent)
    , m_scene(scene)
    , m_view(view)
    , m_prefix(prefix)
    , m_addTerminal(std::move(addTerminal))
    , m_addLine(std::move(addLine))
    , m_track(std::move(track))
    , m_bridgeQuery(std::move(bridgeQuery))
{
    build(topLeft);
}

void Motor3PhaseBlock::build(const QPointF& topLeft)
{
    // szkic minimalny – zakładamy, że gdzieś tutaj tworzysz U/V/W, m_circle, m_labelM itd.
    // Nie ingeruję w Twój istniejący kod rysowania – zostaje jak miałeś.
    Q_UNUSED(topLeft);
}

void Motor3PhaseBlock::onTerminalPhaseChanged(const QString& pin, bool on)
{
    // Pozostawione dla kompatybilności, ale kierunek liczymy w syncPhaseMasks() co recompute.
    if      (pin == m_prefix + "U") m_tOn[0] = on;
    else if (pin == m_prefix + "V") m_tOn[1] = on;
    else if (pin == m_prefix + "W") m_tOn[2] = on;

    // Nie wyliczamy tutaj faz (byłby nieświeży cache przy zmianie źródła).
    // updateRotation();
}

void Motor3PhaseBlock::syncPhaseMasks(int mU, int mV, int mW)
{
    // zapisujemy AKTUALNE maski faz na zaciskach U,V,W (0x1=L1, 0x2=L2, 0x4=L3, 0=brak)
    m_tPhaseId[0] = mU; // U
    m_tPhaseId[1] = mV; // V
    m_tPhaseId[2] = mW; // W

    // Czy na zacisku jest „jakaś” faza?
    m_tOn[0] = (mU != 0);
    m_tOn[1] = (mV != 0);
    m_tOn[2] = (mW != 0);

    updateRotation();
}

void Motor3PhaseBlock::updateRotation()
{
    // jeśli któryś zacisk nie ma jednoznacznej fazy, zatrzymaj wskazanie/animację
    const int a = maskToIdx(m_tPhaseId[0]); // U
    const int b = maskToIdx(m_tPhaseId[1]); // V
    const int c = maskToIdx(m_tPhaseId[2]); // W

    int dir = 0; // 0 = brak kompletu, +1 = CW, -1 = CCW
    if (a >= 0 && b >= 0 && c >= 0) {
        // parzystość permutacji względem (0,1,2)
        const int inversions = (a>b) + (a>c) + (b>c);
        dir = (inversions % 2 == 0) ? +1 : -1;
    }

    // TODO: podłącz animację/etykietę:
    // - jeśli masz QVariantAnimation m_anim, ustaw jej kierunek/prędkość wg dir
    // - zaktualizuj m_labelM tekstem „↻/↺/–” itp.
    Q_UNUSED(dir);
}

int Motor3PhaseBlock::phaseIdFromPeer(const QString& peer)
{
    // jeśli nadal gdzieś używasz tej metody – zostaw. W nowym przepływie nie jest krytyczna.
    Q_UNUSED(peer);
    return -1;
}
