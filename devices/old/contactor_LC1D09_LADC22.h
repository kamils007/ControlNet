#ifndef CONTACTOR_LC1D09_LADC22_H
#define CONTACTOR_LC1D09_LADC22_H

#include <QObject>
#include <QPointF>
#include <QSet>
#include <QVector>
#include <QString>

class QGraphicsScene;
class QGraphicsItem;
class QGraphicsItemGroup;

// ----------------------------------------------------------
// Kontaktor LC1D09 + LADC22 (rysuje się na scenie; styl jak w starym).
// Samowystarczalny: ma własne kolory/pióra i własny „SchematicButton”.
// ----------------------------------------------------------
class Contactor_LC1D09_LADC22 : public QObject
{
    Q_OBJECT
public:
    // prefix np. "K1_" (z podkreśleniem), pozycja = lewy-górny korpusu
    explicit Contactor_LC1D09_LADC22(QGraphicsScene* scene,
                                     const QString& prefix,
                                     const QPointF& topLeft,
                                     QObject* parent = nullptr);
    ~Contactor_LC1D09_LADC22();

    QGraphicsItem*      rootItem() const;  // uchwyt (grupa) do przesuwania/ghostowania
    const QSet<QString>& pins()    const;  // nazwy pinów z prefiksem
    const QString&       prefix()  const;

    // usuń wszystkie elementy ze sceny (bezpieczne wielokrotne wywołanie)
    void destroy();

signals:
    // Żądania do „mózgu” (podłącz do setPhase/Neutral Source on/off)
    void requestAddPhase(const QString& pin);
    void requestRemovePhase(const QString& pin);
    void requestAddNeutral(const QString& pin);
    void requestRemoveNeutral(const QString& pin);

private:
    void build(const QPointF& topLeft);

private:
    QGraphicsScene*         m_scene   = nullptr;
    QGraphicsItemGroup*     m_group   = nullptr;
    QString                 m_prefix;
    QSet<QString>           m_pins;
    QVector<QGraphicsItem*> m_items;
};

// Prosta fabryka — zwraca root (QGraphicsItem*) gotowy do dodania do sceny
QGraphicsItem* createContactor_LC1D09_LADC22(QGraphicsScene* scene,
                                             const QString& prefix,
                                             const QPointF& topLeft);

#endif // CONTACTOR_LC1D09_LADC22_H
