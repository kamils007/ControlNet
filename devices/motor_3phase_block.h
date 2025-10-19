#pragma once

#include <QFont>
#include <QList>
#include <QObject>
#include <QPointF>
#include <QSet>
#include <QString>
#include <QVector>

class QGraphicsScene;
class QGraphicsItem;
class QGraphicsEllipseItem;
class QGraphicsLineItem;
class QGraphicsPathItem;
class QGraphicsSimpleTextItem;

class Motor3PhaseBlock : public QObject {
    Q_OBJECT
public:
    struct Terminal {
        QString name;
        QGraphicsEllipseItem* item = nullptr;
    };

    Motor3PhaseBlock(QGraphicsScene* scene,
                     const QString&  prefix,
                     const QPointF&  topLeft,
                     QObject* parent = nullptr);

    const QString& prefix() const { return m_prefix; }
    const QSet<QString>& pins() const { return m_pins; }
    const QVector<QGraphicsItem*>& items() const { return m_items; }
    const QList<Terminal>& terminals() const { return m_terminals; }

    // Aktualizuje przypisanie faz do zacisków U/V/W. maska: 0x1=L1, 0x2=L2, 0x4=L3, 0 brak fazy.
    void setPhaseMasks(int maskU, int maskV, int maskW);

private:
    void build(const QPointF& topLeft);
    QGraphicsEllipseItem* addTerminal(const QString& name, const QPointF& center);
    QGraphicsLineItem* addLine(const QPointF& p1, const QPointF& p2, Qt::PenStyle style = Qt::SolidLine);
    QGraphicsSimpleTextItem* addLabel(const QPointF& pos,
                                      const QString& text,
                                      int pointSize = 12,
                                      QFont::Weight weight = QFont::Normal);
    void updateRotation();
    void updateTerminalAppearance(int index);
    void ensureDirectionLabel();

private:
    QGraphicsScene* m_scene = nullptr;
    QString         m_prefix;

    QVector<QGraphicsItem*>  m_items;
    QSet<QString>            m_pins;
    QList<Terminal>          m_terminals;

    QGraphicsEllipseItem*    m_body   = nullptr;
    QGraphicsSimpleTextItem* m_labelM = nullptr;
    QGraphicsSimpleTextItem* m_dirLabel = nullptr;
    QGraphicsPathItem*       m_arrow = nullptr;
    QVector<QGraphicsLineItem*> m_phaseTicks;

    int m_phaseMask[3] = {0, 0, 0};
};
