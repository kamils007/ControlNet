#include "mainwindow.h"
#include "contactor_LC1D09_LADC22.h"

#include <QGraphicsItem>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGridLayout>
#include <QStatusBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QWidget>
#include <QEvent>
#include <QMouseEvent>
#include <QCursor>   // ← potrzebne do QCursor::pos()

// ----------------------------------------------------------
// Konstruktor — uruchamia budowę interfejsu
// ----------------------------------------------------------
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    buildUi();
    statusBar()->showMessage(tr("Gotowy"));
}

// ----------------------------------------------------------
// Tworzy centralny widget, scenę i widok
// ----------------------------------------------------------
void MainWindow::buildUi()
{
    auto* central = new QWidget(this);
    auto* layout  = new QGridLayout(central);
    layout->setContentsMargins(0,0,0,0);

    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(-2000, -2000, 4000, 4000);

    m_view = new QGraphicsView(m_scene, central);
    m_view->setRenderHint(QPainter::Antialiasing, true);
    m_view->setDragMode(QGraphicsView::RubberBandDrag);
    m_view->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);

    // Duch ma reagować na ruch bez wciśniętego przycisku:
    m_view->viewport()->setMouseTracking(true);
    // filtrujemy zdarzenia myszki, gdy włączymy tryb wstawiania
    m_view->viewport()->installEventFilter(this);

    layout->addWidget(m_view, 0, 0);
    setCentralWidget(central);

    createMenus();

    setWindowTitle(tr("ContactorSim — wstawianie z kursorem"));
    resize(1200, 800);
}

// ----------------------------------------------------------
// Buduje menu „Plik” i „Wstaw”
// ----------------------------------------------------------
void MainWindow::createMenus()
{
    auto* mb = new QMenuBar(this);

    // --- Plik ---
    m_menuPlik = new QMenu(tr("Plik"), mb);
    m_actNowy = new QAction(tr("Nowy schemat"), m_menuPlik);
    m_actZamknij = new QAction(tr("Zamknij"), m_menuPlik);

    connect(m_actNowy, &QAction::triggered, this, [this]{
        if (m_scene) {
            m_scene->clear();
            statusBar()->showMessage(tr("Utworzono nowy, pusty schemat"));
            m_kCounter = 1;
            m_isPlacingContactor = false;
            m_ghostItem = nullptr;
        }
    });
    connect(m_actZamknij, &QAction::triggered, this, &QWidget::close);

    m_menuPlik->addAction(m_actNowy);
    m_menuPlik->addSeparator();
    m_menuPlik->addAction(m_actZamknij);

    // --- Wstaw ---
    m_menuWstaw = new QMenu(tr("Wstaw"), mb);

    // Wstaw kontaktor — tryb z „duchem” pod kursorem
    m_actWstawK1 = new QAction(tr("Stycznik LC1D09 + LADC22"), m_menuWstaw);
    connect(m_actWstawK1, &QAction::triggered, this, [this]{ beginPlaceContactor(); });
    m_menuWstaw->addAction(m_actWstawK1);

    mb->addMenu(m_menuPlik);
    mb->addMenu(m_menuWstaw);
    setMenuBar(mb);
}

// ----------------------------------------------------------
// Start trybu wstawiania kontaktora (ghost pod kursorem)
// ----------------------------------------------------------
void MainWindow::beginPlaceContactor()
{
    if (!m_scene || !m_view) return;

    // jeśli coś już wstawiamy, najpierw anuluj poprzednie
    if (m_isPlacingContactor)
        finishPlaceContactor(false);

    // Ustal pozycję startową = bieżący kursor nad widokiem (jeśli jest),
    // inaczej fallback na środek widoku.
    QPoint  vpPos = m_view->mapFromGlobal(QCursor::pos());
    QPointF scenePos;
    if (m_view->viewport()->rect().contains(vpPos))
        scenePos = m_view->mapToScene(vpPos);
    else
        scenePos = m_view->mapToScene(m_view->viewport()->rect().center());

    const QString label = QStringLiteral("K%1_").arg(m_kCounter); // numer zwiększymy przy commit

    // tworzymy „ducha”: półprzezroczysty, niemodyfikowalny
    m_ghostItem = createContactor_LC1D09_LADC22(m_scene, label, scenePos);
    m_ghostItem->setOpacity(0.6);
    m_ghostItem->setFlag(QGraphicsItem::ItemIsMovable, false);
    m_ghostItem->setAcceptedMouseButtons(Qt::NoButton); // nie przechwytuje kliknięć

    m_isPlacingContactor = true;
    statusBar()->showMessage(tr("Wstawianie %1: kliknij LPM aby umieścić, PPM aby anulować").arg(label));
}

// ----------------------------------------------------------
// Zakończenie trybu wstawiania (commit=true zatwierdza)
// ----------------------------------------------------------
void MainWindow::finishPlaceContactor(bool commit)
{
    if (!m_isPlacingContactor) return;

    if (!commit) {
        // anuluj: usuń ducha
        if (m_ghostItem) {
            m_scene->removeItem(m_ghostItem);
            delete m_ghostItem;
        }
        m_ghostItem = nullptr;
        m_isPlacingContactor = false;
        statusBar()->showMessage(tr("Anulowano wstawianie"));
        return;
    }

    // zatwierdź: zamień ducha w normalny obiekt
    if (m_ghostItem) {
        m_ghostItem->setOpacity(1.0);
        m_ghostItem->setFlag(QGraphicsItem::ItemIsMovable, true);
        m_ghostItem->setAcceptedMouseButtons(Qt::AllButtons);
        ++m_kCounter; // dopiero przy commit
    }

    m_ghostItem = nullptr;
    m_isPlacingContactor = false;
    statusBar()->showMessage(tr("Wstawiono element"), 1200);
}

// ----------------------------------------------------------
// Event filter: przesuwa „ducha” za kursorem i obsługuje klik
// ----------------------------------------------------------
bool MainWindow::eventFilter(QObject* obj, QEvent* ev)
{
    if (!m_isPlacingContactor) return QMainWindow::eventFilter(obj, ev);
    if (obj != m_view->viewport()) return QMainWindow::eventFilter(obj, ev);

    switch (ev->type()) {
    case QEvent::MouseMove: {
        auto* me = static_cast<QMouseEvent*>(ev);
        if (m_ghostItem) {
            const QPointF scenePos = m_view->mapToScene(me->pos());
            m_ghostItem->setPos(scenePos);
        }
        return true; // przechwycone
    }
    case QEvent::MouseButtonPress: {
        auto* me = static_cast<QMouseEvent*>(ev);
        if (me->button() == Qt::LeftButton) {
            finishPlaceContactor(true);   // zatwierdź
            return true;
        }
        if (me->button() == Qt::RightButton) {
            finishPlaceContactor(false);  // anuluj
            return true;
        }
        break;
    }
    default: break;
    }

    return QMainWindow::eventFilter(obj, ev);
}
