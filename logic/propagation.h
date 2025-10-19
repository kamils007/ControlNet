#pragma once
#include <QSet>
#include <QVector>
#include <QString>
#include <functional>  // <— potrzebne do std::function

struct Edge {
    QString a, b;
    std::function<bool()> conducts;
};

struct HotResult {
    QSet<QString> phaseHot;
    QSet<QString> neutralHot;
};

HotResult computeHot(const QVector<Edge>& edges,
                     const QSet<QString>& phaseSources,
                     const QSet<QString>& neutralSources);

QSet<QString> computeInterPhaseFault(const QVector<Edge>& edges,
                                     const QSet<QString>& phaseSources);


QHash<QString,int> computePhaseMask(const QVector<Edge>& edges,
                                     const QSet<QString>& phaseSources);

