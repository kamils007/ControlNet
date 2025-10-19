#pragma once
#include <QObject>

class ContactorModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool energized READ isEnergized NOTIFY energizedChanged)
public:
    enum class ContactState { Open = 0, Closed = 1 };
    Q_ENUM(ContactState)

    explicit ContactorModel(QObject* parent = nullptr);

    bool isEnergized() const { return m_energized; }
    bool a1() const { return m_a1; }
    bool a2() const { return m_a2; }

public slots:
    // Skrót: ustawia oba wejścia (A1 i A2) naraz
    void setEnergized(bool on);
    void toggle();

    // Niezależne sterowanie końcami cewki
    void setA1(bool on);
    void setA2(bool on);

signals:
    void energizedChanged(bool on);
    void a1Changed(bool on);
    void a2Changed(bool on);

    // 3P razem (prosta sygnalizacja)
    void mainContactsChanged(ContactState state);
    void auxNOChanged(ContactState state);
    void auxNCChanged(ContactState state);

private:
    void propagateStates();
    void recomputeEnergizedFromCoil();   // wypadkowa = (A1 && A2)

    bool m_energized = false; // wynik AND z A1/A2
    bool m_a1 = false;        // „gorący” koniec cewki
    bool m_a2 = false;        // powrót/neutral zasilania
};
