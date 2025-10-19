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
class SchematicButton;

// Blok „Stycznik LC1D09 + przystawka LADC22”.
// Rysuje korpus, piny L/T, A1/A2, styki pomocnicze oraz przyciski START/RET.
// Nie zależy bezpośrednio od ContactorView — współpraca przez lambdy przekazane w ctorze.
class Contactor_LC1D09_LADC22 : public QObject {
    Q_OBJECT
public:
    using AddTerminalFn = std::function<QGraphicsEllipseItem*(const QString&, const QPointF&)>;
    using AddLineFn     = std::function<QGraphicsLineItem*(const QPointF&, const QPointF&, Qt::PenStyle)>;
    using TrackFn       = std::function<void(QGraphicsItem*)>;

    Contactor_LC1D09_LADC22(QGraphicsScene* scene,
                            const QString& prefix,   // np. "K1_"
                            const QPointF& topLeft,
                            AddTerminalFn addTerminal,
                            AddLineFn addLine,
                            TrackFn track,
                            QObject* parent = nullptr);

    const QString& prefix() const { return m_prefix; }
    const QSet<QString>& pins() const { return m_pins; }
    const QVector<QGraphicsItem*>& items() const { return m_items; }

signals:
    // Forwardowane przez ContactorView do logiki okna
    void requestAddPhase(const QString& pin);      // A1 = faza
    void requestRemovePhase(const QString& pin);
    void requestAddNeutral(const QString& pin);    // A2 = zero
    void requestRemoveNeutral(const QString& pin);

private:
    void build(const QPointF& topLeft);

private:
    QGraphicsScene* m_scene = nullptr;
    QString m_prefix; // "K1_"
    AddTerminalFn m_addTerminal;
    AddLineFn     m_addLine;
    TrackFn       m_track;

    QVector<QGraphicsItem*> m_items; // wszystkie itemy graficzne (w tym elipsy pinów)
    QSet<QString>           m_pins;  // pełne nazwy pinów

    // lokalne kontrolki
    SchematicButton* m_btnA1 = nullptr;
    SchematicButton* m_btnA2 = nullptr;
};
