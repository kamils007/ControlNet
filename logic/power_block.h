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

class PowerBlock : public QObject {
    Q_OBJECT
public:
    using AddTerminalFn = std::function<QGraphicsEllipseItem*(const QString&, const QPointF&)>;
    using AddLineFn     = std::function<QGraphicsLineItem*(const QPointF&, const QPointF&)>; // zwraca QGraphicsLineItem*
    using TrackFn       = std::function<void(QGraphicsItem*)>;

    PowerBlock(QGraphicsScene* scene,
               const QString& prefix,
               const QPointF& topLeft,
               AddTerminalFn addTerminal,
               AddLineFn addLine,
               TrackFn track,
               QObject* parent = nullptr);

    const QString& prefix() const { return m_prefix; }
    const QSet<QString>& pins() const { return m_pins; }
    const QVector<QGraphicsItem*>& items() const { return m_items; }

signals:
    void requestAddPhase(const QString& pin);
    void requestRemovePhase(const QString& pin);

private:
    void build(const QPointF& topLeft);

private:
    QGraphicsScene* m_scene = nullptr;
    QString m_prefix;
    AddTerminalFn m_addTerminal;
    AddLineFn     m_addLine;
    TrackFn       m_track;

    QVector<QGraphicsItem*> m_items; // WSZYSTKIE itemy (rect, napisy, elipsy, linie, przycisk)
    QSet<QString>           m_pins;

    SchematicButton* m_btnPower = nullptr;
};
