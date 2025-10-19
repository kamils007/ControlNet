#pragma once
#include <QMainWindow>
#include <QPointer>
#include <QString>
#include <QVector>
#include <QSet>
#include <QHash>
#include <functional>

#include "propagation.h"
#include "contactor_model.h"
#include "contactor_view.h"

class QGraphicsPathItem;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    // LOGIKA SYGNAŁU
    void setPhaseSource(const QString& node, bool on);
    void setNeutralSource(const QString& node, bool on);
    bool isNodePhaseHot(const QString& node) const;
    bool isNodeNeutralHot(const QString& node) const;

private slots:
    void updateCoilLamp(bool on);
    void updateMainContacts(ContactorModel::ContactState st);
    void updateAuxNO(ContactorModel::ContactState st);
    void updateAuxNC(ContactorModel::ContactState st);

    // Styczniki
    void onContactorPlaced(const QString& K);      // "K1_"
    void onContactorDelete(const QString& K);      // "K1_"

    // NOWE: Zasilanie 3F
    void onPowerPlaced(const QString& P);          // "P1_"
    void onPowerDelete(const QString& P);          // "P1_"

private:
    void buildUi();
    static void setLampState(class QLabel* lamp, bool on,
                             const QString& onText, const QString& offText);
    void refreshUpperFeed();

    struct NamedLink {
        QString srcContact;
        QString dstCoil;
        std::function<bool()> feed;
    };

    void addLink(const QString& srcContact, const QString& dstCoil,
                 std::function<bool()> feed = {});
    void applyLinks();
    void setupWiring();
    std::function<bool()>     resolveContact(const QString& name) const;
    std::function<void(bool)> resolveCoilSetter(const QString& name);

    void addContactEdgeDyn(const QString& K, const char* a, const char* b, bool isNO);
    void addWire(const QString& a, const QString& b, std::function<bool()> cond = {});
    void removeWire(const QString& a, const QString& b);
    void recomputeSignals(); // z iteracją do zbieżności
    static QString toViewPin(const QString& nodeLogic) { return nodeLogic; }

    // Stan
    ContactorView* m_view = nullptr;

    QVector<NamedLink> m_namedLinks;
    QVector<Edge>      m_edges;          // <-- używa Edge z logic/propagation.h

    QSet<QString>  m_phaseSources;
    QSet<QString>  m_neutralSources;
    QSet<QString>  m_phaseHot;
    QSet<QString>  m_neutralHot;

    QSet<QString>  m_auxNodes;
    QHash<QString, QString> m_nodeToView;

    // Zbiór styczników i ich stan energizacji
    QSet<QString>  m_contactors;            // "K1_", "K2_", ...
    QHash<QString, bool> m_contEnergized;   // prefix -> energized

    // NOWE: zasilanie 3F
    QSet<QString>  m_powers;                // "P1_", "P2_", ...
};
