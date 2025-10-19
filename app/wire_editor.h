#pragma once
#include <QObject>
#include <QPointF>
#include <QVector>

class ContactorView;
class QGraphicsPathItem;
class QEvent;
class QMouseEvent;

// Prosty edytor mostków orthogonalnych (H/V) na QGraphicsView.
class WireEditor : public QObject {
    Q_OBJECT
public:
    explicit WireEditor(ContactorView* view, QObject* parent = nullptr);

signals:
    // Zgłaszane po zakończeniu rysowania i „wpiciu” w terminal
    void wireCommitted(const QString& startPin, const QString& endPin, const QVector<QPointF>& points);

protected:
    bool eventFilter(QObject* obj, QEvent* ev) override;

private:
    enum class Dir { None, H, V };

    void startAt(const QString& pin, const QPointF& pos);
    void cancel();
    void finishAt(const QString& pin, const QPointF& pos);
    void onMouseMove(const QPointF& scenePos);
    void onClick(const QPointF& scenePos);    // pojedynczy klik: załamka + obrót osi

    QPointF orthoTo(const QPointF& scenePos) const; // wyrównanie do H/V względem ostatniego punktu

    // NOWE: każdy pin rozpoznany przez ContactorView jest dozwolony
    bool isAuxPin(const QString& pin) const { return !pin.isEmpty(); }

    ContactorView*      m_view = nullptr;
    QString             m_startPin;
    QString             m_lastHoverPin;
    bool                m_active = false;
    Dir                 m_dir = Dir::None;
    QVector<QPointF>    m_pts;        // zakotwiczone punkty (załamki)
    QGraphicsPathItem*  m_rubber = nullptr; // podążający „wąż”
};
