#pragma once
#include <QObject>
#include <QPointF>
#include <QVector>
#include <QSet>
#include <functional>

class QGraphicsScene;
class QGraphicsItem;
class QGraphicsEllipseItem;
class QGraphicsLineItem;
class QGraphicsSimpleTextItem;
class QVariantAnimation;
class ContactorView;

class Motor3PhaseBlock : public QObject {
    Q_OBJECT
public:
    using AddTerminalFn = std::function<QGraphicsEllipseItem*(const QString&, const QPointF&)>;
    using AddLineFn     = std::function<QGraphicsLineItem*(const QPointF&, const QPointF&)>;
    using TrackFn       = std::function<void(QGraphicsItem*)>;
    using BridgeQueryFn = std::function<QString(const QString&)>;

    Motor3PhaseBlock(QGraphicsScene* scene,
                     ContactorView*  view,
                     const QString&  prefix,
                     const QPointF&  topLeft,
                     AddTerminalFn   addTerminal,
                     AddLineFn       addLine,
                     TrackFn         track,
                     BridgeQueryFn   bridgeQuery,
                     QObject* parent = nullptr);

    const QString& prefix() const { return m_prefix; }
    const QSet<QString>& pins() const { return m_pins; }
    const QVector<QGraphicsItem*>& items() const { return m_items; }

public slots:
    void onTerminalPhaseChanged(const QString& pin, bool on);

public:
    // **NOWE**: wstrzyknięcie aktualnych masek faz na zaciskach U,V,W (0x1=L1, 0x2=L2, 0x4=L3)
    void syncPhaseMasks(int mU, int mV, int mW);

private:
    void build(const QPointF& topLeft);
    void updateRotation();
    int  phaseIdFromPeer(const QString&);

private:
    QGraphicsScene* m_scene = nullptr;
    ContactorView*  m_view  = nullptr;
    QString m_prefix;

    AddTerminalFn m_addTerminal;
    AddLineFn     m_addLine;
    TrackFn       m_track;
    BridgeQueryFn m_bridgeQuery;

    QVector<QGraphicsItem*> m_items;
    QSet<QString>           m_pins;

    QGraphicsSimpleTextItem* m_labelM = nullptr;
    QGraphicsEllipseItem*    m_circle = nullptr;
    QVariantAnimation*       m_anim   = nullptr;

    bool m_tOn[3] = {false, false, false};
    // **UWAGA**: teraz trzymamy tutaj maski (0/0x1/0x2/0x4), a nie „cache fazy” rozpoznany raz
    int  m_tPhaseId[3] = {0,0,0}; // U,V,W+
};
