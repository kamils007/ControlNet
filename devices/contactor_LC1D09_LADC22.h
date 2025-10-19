#pragma once

#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QSet>
#include <QVector>
#include <QHash>

#include <QColor>
#include <QGraphicsObject>

class QGraphicsScene;
class QGraphicsItem;
class QGraphicsEllipseItem;
class QGraphicsLineItem;
class QGraphicsSceneMouseEvent;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

// Prosty prostokątny przycisk sceniczny wykorzystywany przez bloki urządzeń.
class SchematicButton : public QGraphicsObject {
    Q_OBJECT
public:
    SchematicButton(const QRectF& r,
                    const QString& text,
                    QGraphicsItem* parent = nullptr,
                    const QColor& onColor = Qt::green);

    QRectF boundingRect() const override { return m_rect; }
    void   setOn(bool on);
    bool   isOn() const { return m_on; }

signals:
    void toggled(bool on);

protected:
    void paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* e) override;

private:
    QRectF  m_rect;
    QString m_text;
    QColor  m_onColor;
    bool    m_on = false;
};

// Blok „Stycznik LC1D09 + przystawka LADC22”.
// Rysuje korpus, piny L/T, A1/A2, styki pomocnicze oraz przyciski START/RET.
// W pełni samodzielny — sam dodaje elementy graficzne do sceny.
class Contactor_LC1D09_LADC22 : public QObject {
    Q_OBJECT
public:
    Contactor_LC1D09_LADC22(QGraphicsScene* scene,
                            const QString& prefix,   // np. "K1_"
                            const QPointF& topLeft,
                            QObject* parent = nullptr);

    const QString& prefix() const { return m_prefix; }
    const QSet<QString>& pins() const { return m_pins; }
    const QVector<QGraphicsItem*>& items() const { return m_items; }
    const QHash<QString, QGraphicsEllipseItem*>& terminalItems() const { return m_terminals; }

signals:
    // Forwardowane przez ContactorView do logiki okna
    void requestAddPhase(const QString& pin);      // A1 = faza
    void requestRemovePhase(const QString& pin);
    void requestAddNeutral(const QString& pin);    // A2 = zero
    void requestRemoveNeutral(const QString& pin);

private:
    void build(const QPointF& topLeft);
    QGraphicsEllipseItem* createTerminal(const QString& name, const QPointF& center);
    QGraphicsLineItem*    createLine(const QPointF& a, const QPointF& b, Qt::PenStyle style = Qt::SolidLine);

private:
    QGraphicsScene* m_scene = nullptr;
    QString m_prefix; // "K1_"

    QVector<QGraphicsItem*> m_items; // wszystkie itemy graficzne (w tym elipsy pinów)
    QSet<QString>           m_pins;  // pełne nazwy pinów
    QHash<QString, QGraphicsEllipseItem*> m_terminals; // pin -> elipsa

    // lokalne kontrolki
    SchematicButton* m_btnA1 = nullptr;
    SchematicButton* m_btnA2 = nullptr;
};
