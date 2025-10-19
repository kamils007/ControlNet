#pragma once
#include <QGraphicsView>
#include <QMap>
#include <QPointer>
#include <QGraphicsObject>
#include <QVector>
#include <QPointF>
#include <QHash>
#include <QSet>
#include <functional>

class QGraphicsScene;
class QGraphicsEllipseItem;
class QGraphicsLineItem;
class QGraphicsPathItem;
class QGraphicsRectItem;
class QPainter;
class QStyleOptionGraphicsItem;
class QGraphicsSceneMouseEvent;
class QWidget;
class QColor;
class QPen;
class QContextMenuEvent;
class QMouseEvent;

class PowerBlock;                   // fwd
class Contactor_LC1D09_LADC22;     // fwd
class Motor3PhaseBlock;            // fwd

// Klikalny przycisk prostokątny (używany przez bloki i widok)
class SchematicButton : public QGraphicsObject {
    Q_OBJECT
public:
    SchematicButton(const QRectF& r, const QString& text, QGraphicsItem* parent = nullptr, const QColor& onColor = Qt::green);
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

class ContactorView : public QGraphicsView {
    Q_OBJECT
public:
    explicit ContactorView(QWidget* parent = nullptr, bool buildDefault = true);

    // Styl
    static QColor colBg();
    static QColor colWire();
    static QColor colBlock();
    static QColor colDash();
    static QColor colText();
    static QColor colOn();
    static QColor colPhase();
    static QColor colNeutral();
    static QColor colFault();
    static QPen   penWire(qreal w = 1.6);
    static QPen   penBlock(qreal w = 1.6);
    static QPen   penDash(qreal w = 1.2);

    // **NOWE**: zasil bieżącymi maskami faz na zaciskach silnika (U,V,W)
    // mX: bitmaski 0x1=L1, 0x2=L2, 0x4=L3; 0 = brak fazy na danym zacisku
    void setMotorPhaseMasks(const QString& motorPrefix, int mU, int mV, int mW);

signals:
    // Źródła FAZA/ZERO
    void addPhaseSourceRequested(const QString& pinView);
    void addNeutralSourceRequested(const QString& pinView);
    void removePhaseSourceRequested(const QString& pinView);
    void removeNeutralSourceRequested(const QString& pinView);

    // Mostki
    void bridgeDeleteRequested(const QString& aPin, const QString& bPin, QGraphicsPathItem* item);

    // Styczniki
    void contactorPlaced(const QString& kPrefix);           // np. "K1_"
    void contactorDeleteRequested(const QString& kPrefix);  // PPM

    // Zasilanie 3F
    void powerPlaced(const QString& pPrefix);               // np. "P1_"
    void powerDeleteRequested(const QString& pPrefix);      // PPM

    // **Istniejące**: powiadomienie o zmianie „fazowości” na pinie (dla silnika 3F itp.)
    void terminalPhaseChanged(const QString& pinName, bool on);

public slots:
    // legacy slots (puste lub proste)
    void setEnergized(bool /*on*/) {}
    void setA1On(bool on);
    void setA2On(bool on);
    void setA1BOn(bool on);
    void setA2BOn(bool on);
    void setSupplyOn(bool on);
    void setUpperSupplyOn(bool on);
    void setMainClosed(bool closed);
    void setMainBClosed(bool closed);
    void setAuxNOClosed(bool closed);
    void setAuxNCClosed(bool closed);
    void setAuxNOBClosed(bool closed);
    void setAuxNCBClosed(bool closed);

    // Kolorowanie + zwarcie
    void setTerminalPhase(const QString& name, bool on);
    void setTerminalNeutral(const QString& name, bool on);
    void setTerminalFault(const QString& name, bool on);
    void setTerminalHot(const QString& name, bool on);

public:
    // API mostków
    QString  terminalAt(const QPointF& scenePos, qreal radius = 14.0) const;
    QPointF  terminalPos(const QString& name) const;
    QGraphicsPathItem* addBridgePolyline(const QVector<QPointF>& pts);
    void     registerBridge(QGraphicsPathItem* item, const QString& aPin, const QString& bPin);
    void     removeBridgeItem(QGraphicsPathItem* item);

    // Tryby wstawiania
    void beginPlaceContactor();
    void beginPlacePower3();
    void beginPlaceMotor3();      // **NOWE**

    // Usuwanie z widoku
    void removeContactor(const QString& kPrefix);
    void removePowerBlock(const QString& pPrefix);
    void removeMotor(const QString& mPrefix);  // **NOWE**

protected:
    void contextMenuEvent(QContextMenuEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;

private:
    void buildScene(); // puste
    void drawSingleContactorAt(const QPointF& topLeft, uint8_t idx); // tworzy Contactor_LC1D09_LADC22
    void drawPower3At(const QPointF& topLeft, uint8_t idx);          // tworzy PowerBlock
    void drawMotorAt(const QPointF& topLeft, uint8_t idx);           // **NOWE** tworzy Motor3PhaseBlock

    // bazowe
    QGraphicsEllipseItem* addTerminal(const QString& name, const QPointF& center);
    QGraphicsLineItem*    addLine(const QString& key, const QPointF& p1, const QPointF& p2, Qt::PenStyle style = Qt::SolidLine);
    void                  setTerminalOn(const QString& name, bool on);
    void                  registerAuxPair(const QString& id, const QString& upper, const QString& lower, bool isNO);

    // pomocnicze śledzenie grup
    struct Group {
        QVector<QGraphicsItem*> items;
        QSet<QString> pins;
    };
    void      trackItem(QGraphicsItem* it);
    QString   kOfItem(QGraphicsItem* it) const;
    QString   pOfItem(QGraphicsItem* it) const;
    QString   mOfItem(QGraphicsItem* it) const;   // **NOWE**

private:
    QPointer<QGraphicsScene> m_scene;
    QMap<QString, QGraphicsEllipseItem*> m_terms;

    // Przyciski (legacy)
    SchematicButton*   m_btnA1  = nullptr;
    SchematicButton*   m_btnA2  = nullptr;
    SchematicButton*   m_btnA1b = nullptr;
    SchematicButton*   m_btnA2b = nullptr;

    // Zwarcia
    QMap<QString, QVector<QGraphicsLineItem*>> m_faultMarks;

    // Mostki
    QVector<QGraphicsPathItem*> m_bridges;
    QHash<QGraphicsPathItem*, QPair<QString,QString>> m_bridgeToPins;

    // Tryb wstawiania
    bool                m_placeContactor = false;
    bool                m_placePower3    = false;
    bool                m_placeMotor3    = false;  // **NOWE**
    QGraphicsRectItem*  m_contGhost = nullptr;
    uint8_t             m_nextK = 1;
    uint8_t             m_nextP = 1;
    uint8_t             m_nextM = 1;               // **NOWE**

    // Rejestry i mapowania
    QHash<QString, Group>   m_contactors;                 // "K1_" -> items/pins
    QHash<QGraphicsItem*, QString>   m_itemToK;

    QHash<QString, Group>   m_powers;                     // "P1_" -> items/pins
    QHash<QGraphicsItem*, QString>   m_itemToP;

    QHash<QString, Group>   m_motors;                     // **NOWE** "M1_" -> items/pins
    QHash<QGraphicsItem*, QString>   m_itemToM;           // **NOWE**

    QHash<QString, PowerBlock*>                  m_powerBlocks; // "P1_" -> obiekt PowerBlock
    QHash<QString, Contactor_LC1D09_LADC22*>     m_contBlocks;  // "K1_" -> obiekt stycznika
    QHash<QString, Motor3PhaseBlock*>            m_motorBlocks; // **NOWE** "M1_" -> obiekt silnika

    // „Aktualnie buduję” — addTerminal/trackItem odkłada do właściwej grupy
    QString m_buildingK;
    QString m_buildingP;
    QString m_buildingM; // **NOWE** – jeśli używasz w drawMotorAt
};
