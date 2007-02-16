#include <QtDebug>
#include "mainwindow.h"
#include <QCloseEvent>

MainWindow::MainWindow(QWidget *AParent, Qt::WindowFlags AFlags)
  : QMainWindow(AParent,AFlags)
{
  qDebug() << "MainWindow";
  FPluginManager = NULL;
  FSettings = NULL;
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

  FRostersWidget = new QStackedWidget; 
  FRostersWidget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

  FBottomWidget = new QStackedWidget; 
  FBottomWidget->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Maximum);  
                     
  FMainLayout = new QVBoxLayout;
  FMainLayout->addWidget(FUpperWidget);  
  FMainLayout->addWidget(FRostersWidget);  
  FMainLayout->addWidget(FBottomWidget);  

  QWidget *centralWidget = new QWidget;
  centralWidget->setLayout(FMainLayout); 
  setCentralWidget(centralWidget);
}

void MainWindow::createToolBars()
{
  FMainToolBar = addToolBar(tr("Main"));
  FMainToolBar->setMovable(false); 
  addToolBar(Qt::TopToolBarArea,FMainToolBar);
}

void MainWindow::createMenus()
{
  mnuMain = new Menu(0,"mainwindow::mainmenu",tr("Menu"),this);
  FMainToolBar->addAction(mnuMain->menuAction()); 
}

void MainWindow::createActions()
{
  actQuit = new Action(1000,tr("Quit"),this);
  mnuMain->addAction(actQuit);
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

void MainWindow::closeEvent( QCloseEvent *AEvent )
{
  showMinimized();
  AEvent->ignore();
}
