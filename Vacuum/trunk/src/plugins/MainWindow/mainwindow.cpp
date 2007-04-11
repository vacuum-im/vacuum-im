#include <QtDebug>
#include "mainwindow.h"

#include <QCloseEvent>
#include <QToolButton>

#define SYSTEM_ICONSETFILE "system/common.jisp"

MainWindow::MainWindow(Qt::WindowFlags AFlags)
  : QMainWindow(NULL,AFlags)
{
  setIconSize(QSize(16,16));
  createLayouts();
  createToolBars();
  createMenus();
}

MainWindow::~MainWindow()
{

}

void MainWindow::createLayouts()
{
  FUpperWidget = new QStackedWidget; 
  FUpperWidget->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Maximum);
  FUpperWidget->setVisible(false);

  FRostersWidget = new QStackedWidget; 
  FRostersWidget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

  FBottomWidget = new QStackedWidget; 
  FBottomWidget->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Maximum);  
  FBottomWidget->setVisible(false);
                     
  FMainLayout = new QVBoxLayout;
  FMainLayout->setMargin(2);
  FMainLayout->addWidget(FUpperWidget);  
  FMainLayout->addWidget(FRostersWidget);  
  FMainLayout->addWidget(FBottomWidget);  

  QWidget *centralWidget = new QWidget;
  centralWidget->setLayout(FMainLayout); 
  setCentralWidget(centralWidget);
}

void MainWindow::createToolBars()
{
  FTopToolBar = addToolBar(tr("Top ToolBar"));
  FTopToolBar->setMovable(false); 
  addToolBar(Qt::TopToolBarArea,FTopToolBar);

  FBottomToolBar = addToolBar(tr("Bottom ToolBar"));
  FBottomToolBar->setMovable(false); 
  FBottomToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  addToolBar(Qt::BottomToolBarArea,FBottomToolBar);
}

void MainWindow::createMenus()
{
  mnuMain = new Menu(this);
  mnuMain->setIcon(SYSTEM_ICONSETFILE,"psi/jabber");
  mnuMain->setTitle(tr("Menu"));
  QToolButton *tbutton = new QToolButton;
  tbutton->setDefaultAction(mnuMain->menuAction());
  tbutton->setPopupMode(QToolButton::InstantPopup);
  tbutton->setToolButtonStyle(Qt::ToolButtonIconOnly);
  FBottomToolBar->addWidget(tbutton);
}

void MainWindow::closeEvent(QCloseEvent *AEvent)
{
  //showMinimized();
  //AEvent->ignore();
}
