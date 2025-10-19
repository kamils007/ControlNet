#include "contactor_model.h"

ContactorModel::ContactorModel(QObject* parent)
    : QObject(parent) {}

void ContactorModel::setEnergized(bool on) {
    // Ustawiamy oba końce zgodnie (kompatybilny skrót)
    if (m_a1 != on) { m_a1 = on; emit a1Changed(m_a1); }
    if (m_a2 != on) { m_a2 = on; emit a2Changed(m_a2); }
    recomputeEnergizedFromCoil();
}

void ContactorModel::toggle() {
    setEnergized(!m_energized);
}

void ContactorModel::setA1(bool on) {
    if (m_a1 == on) return;
    m_a1 = on;
    emit a1Changed(m_a1);
    recomputeEnergizedFromCoil();
}

void ContactorModel::setA2(bool on) {
    if (m_a2 == on) return;
    m_a2 = on;
    emit a2Changed(m_a2);
    recomputeEnergizedFromCoil();
}

void ContactorModel::recomputeEnergizedFromCoil() {
    // Właściwa logika cewki: MUSZĄ być oba końce (A1 && A2)
    const bool newE = (m_a1 && m_a2);
    if (m_energized == newE) return;
    m_energized = newE;
    emit energizedChanged(m_energized);
    propagateStates();
}

void ContactorModel::propagateStates() {
    const auto mainState = m_energized ? ContactState::Closed : ContactState::Open;
    emit mainContactsChanged(mainState);
    emit auxNOChanged(m_energized ? ContactState::Closed : ContactState::Open);
    emit auxNCChanged(m_energized ? ContactState::Open  : ContactState::Closed);
}
