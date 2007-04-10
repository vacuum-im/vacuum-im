#include <QtDebug>
#include "mainwindow.h"
#include <QCloseEvent>
#include <QToolButton>

#define SYSTEM_ICONSETFILE "system/common.jisp"

MainWindow::MainWindow(QWidget *AParent, Qt::WindowFlags AFlags)
  : QMainWindow(AParent,AFlags)
{
  FPluginManager = NULL;
  FSettings = NULL;
  setIconSize(QSize(16,16));
  createLayouts();
  createToolBars();
  createMenus();
  createActions();
}

MainWindow::~MainWindow()
{
  qDebug() << "~MainWindow";
}

bool MainWindow::init(IPluginManager *APluginManager, ISettings *ASettings)
{
  FPluginManager = APluginManager;
  FSettings = ASettings;
  if (FSettings)
  {
    connect(FSettings->instance(),SIGNAL(opened()),SLOT(onSettingsOpened()));
    connect(FSettings->instance(),SIGNAL(closed()),SLOT(onSettingsClosed()));
  }
  connectActions();
  return true;
}

bool MainWindow::start()
{
  show();
  return true;
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

void MainWindow::createActions()
{
  actQuit = new Action(this);
  actQuit->setIcon(SYSTEM_ICONSETFILE,"psi/quit");
  actQuit->setText(tr("Quit"));
  mnuMain->addAction(actQuit,MAINWINDOW_ACTION_GROUP_QUIT);
}

void MainWindow::connectActions()
{
  connect(actQuit,SIGNAL(triggered()),FPluginManager->instance(),SLOT(quit())); 
}

void MainWindow::onSettingsOpened()
{
  setGeometry(FSettings->value("window:geometry",geometry()).toRect());
}

void MainWindow::onSettingsClosed()
{
  if (isVisible())
    FSettings->setValue("window:geometry",geometry());
}

void MainWindow::closeEvent(QCloseEvent *AEvent)
{
  //showMinimized();
  //AEvent->ignore();
}
