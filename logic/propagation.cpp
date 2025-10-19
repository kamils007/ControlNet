#include "propagation.h"
#include <QQueue>




static QVector<QString> neighbors(const QVector<Edge>& edges, const QString& node) {
    QVector<QString> out; out.reserve(8);
    for (const auto& e : edges) {
        if (e.a == node && (!e.conducts || e.conducts())) {
            out.push_back(e.b);
        }
    }
    return out;
}

HotResult computeHot(const QVector<Edge>& edges,
                     const QSet<QString>& phaseSources,
                     const QSet<QString>& neutralSources)
{
    HotResult r;

    if (!phaseSources.isEmpty()) {
        QQueue<QString> q;
        for (const QString& s : phaseSources) {
            if (r.phaseHot.contains(s)) continue;
            r.phaseHot.insert(s);
            q.enqueue(s);
        }
        while (!q.isEmpty()) {
            const QString u = q.dequeue();
            for (const auto& e : edges) {
                if (e.a != u) continue;
                if (e.conducts && !e.conducts()) continue;
                if (r.phaseHot.contains(e.b)) continue;
                r.phaseHot.insert(e.b);
                q.enqueue(e.b);
            }
        }
    }

    if (!neutralSources.isEmpty()) {
        QQueue<QString> q;
        for (const QString& s : neutralSources) {
            if (r.neutralHot.contains(s)) continue;
            r.neutralHot.insert(s);
            q.enqueue(s);
        }
        while (!q.isEmpty()) {
            const QString u = q.dequeue();
            for (const auto& e : edges) {
                if (e.a != u) continue;
                if (e.conducts && !e.conducts()) continue;
                if (r.neutralHot.contains(e.b)) continue;
                r.neutralHot.insert(e.b);
                q.enqueue(e.b);
            }
        }
    }

    return r;
}

static int phaseMaskOf(const QString& n) {
    if (n.endsWith("L1")) return 0x1;
    if (n.endsWith("L2")) return 0x2;
    if (n.endsWith("L3")) return 0x4;
    return 0;
}

QSet<QString> computeInterPhaseFault(const QVector<Edge>& edges,
                                     const QSet<QString>& phaseSources)
{
    // Źródła per faza
    QVector<QString> srcL1, srcL2, srcL3;
    srcL1.reserve(8); srcL2.reserve(8); srcL3.reserve(8);

    for (const QString& s : phaseSources) {
        const int m = phaseMaskOf(s);
        if (m & 0x1) srcL1.push_back(s);
        if (m & 0x2) srcL2.push_back(s);
        if (m & 0x4) srcL3.push_back(s);
    }

    // BFS z maskowaniem bitów
    QHash<QString,int> mask;
    auto bfsMark = [&](const QVector<QString>& sources, int bit){
        if (sources.isEmpty()) return;
        QSet<QString> visited;
        QQueue<QString> q;
        for (const QString& s : sources) { visited.insert(s); q.enqueue(s); mask[s] |= bit; }
        while (!q.isEmpty()) {
            const QString u = q.dequeue();
            for (const QString& v : neighbors(edges, u)) {
                if (visited.contains(v)) continue;
                visited.insert(v);
                q.enqueue(v);
                mask[v] |= bit;
            }
        }
    };

    bfsMark(srcL1, 0x1);
    bfsMark(srcL2, 0x2);
    bfsMark(srcL3, 0x4);

    // Zwarcie międzyfazowe: >= 2 bity
    QSet<QString> faults;
    for (auto it = mask.constBegin(); it != mask.constEnd(); ++it) {
        const int m = it.value();
        if (m && (m & (m - 1))) faults.insert(it.key());
    }
    return faults;
}

// propagation.cpp (wyciąg z Twojego BFS od L1/L2/L3)
QHash<QString,int> computePhaseMask(const QVector<Edge>& edges,
                                     const QSet<QString>& phaseSources) {
    auto neighbors = [&](const QString& node){
        QVector<QString> out; out.reserve(8);
        for (const auto& e : edges)
            if (e.a == node && (!e.conducts || e.conducts())) out.push_back(e.b);
        return out;
    };
    auto phaseOf = [](const QString& n)->int {
        if (n.endsWith("L1")) return 0x1;
        if (n.endsWith("L2")) return 0x2;
        if (n.endsWith("L3")) return 0x4;
        return 0;
    };

    QVector<QString> srcL1, srcL2, srcL3;
    for (const QString& s : phaseSources) {
        const int m = phaseOf(s);
        if (m & 0x1) srcL1.push_back(s);
        if (m & 0x2) srcL2.push_back(s);
        if (m & 0x4) srcL3.push_back(s);
    }

    QHash<QString,int> mask;
    auto bfsMark = [&](const QVector<QString>& sources, int bit){
        if (sources.isEmpty()) return;
        QSet<QString> vis; QQueue<QString> q;
        for (const QString& s : sources) { vis.insert(s); q.enqueue(s); mask[s] |= bit; }
        while (!q.isEmpty()) {
            const QString u = q.dequeue();
            for (const QString& v : neighbors(u)) {
                if (!vis.contains(v)) { vis.insert(v); q.enqueue(v); mask[v] |= bit; }
            }
        }
    };
    bfsMark(srcL1, 0x1); bfsMark(srcL2, 0x2); bfsMark(srcL3, 0x4);
    return mask;
}

