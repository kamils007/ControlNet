#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QGraphicsView;
class QGraphicsScene;
class QMenu;
class QAction;
class QGraphicsItem;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);                  // tworzy UI

private:
    void buildUi();                                                  // scena + widok
    void createMenus();                                              // pasek menu
    void beginPlaceContactor();                                      // start trybu wstawiania
    void finishPlaceContactor(bool commit);                          // zatwierdź/anuluj

    // Event filter, żeby śledzić mysz na widoku podczas wstawiania
    bool eventFilter(QObject* obj, QEvent* ev) override;             // mysz: move/click

private:
    // UI
    QGraphicsView*  m_view   = nullptr;                              // widok sceny
    QGraphicsScene* m_scene  = nullptr;                              // scena rysunkowa

    // Menu
    QMenu*   m_menuPlik    = nullptr;
    QMenu*   m_menuWstaw   = nullptr;
    QAction* m_actNowy     = nullptr;
    QAction* m_actZamknij  = nullptr;
    QAction* m_actWstawK1  = nullptr;

    // Wstawianie
    bool           m_isPlacingContactor = false;                     // tryb wstawiania?
    QGraphicsItem* m_ghostItem          = nullptr;                   // „duch” pod kursorem
    int            m_kCounter           = 1;                         // K1, K2, ...
};

#endif // MAINWINDOW_H
