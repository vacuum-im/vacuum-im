#include <QtDebug>
#include "mainwindow.h"

MainWindow::MainWindow(QWidget *AParent, Qt::WindowFlags AFlags)
  : QMainWindow(AParent,AFlags)
{
  qDebug() << "MainWindow";
  setParent(AParent);
  setWindowFlags(AFlags);
  createLayouts();
  createToolBars();
  createActions();
}

MainWindow::~MainWindow()
{
  qDebug() << "~MainWindow";
}

bool MainWindow::init(IPluginManager *APluginManager)
{
  FPluginManager = APluginManager;
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
  addToolBar(Qt::LeftToolBarArea,FMainToolBar);
}

void MainWindow::createActions()
{
  mnuMain = new Menu(0,"mainwindow::mainmenu",tr("Menu"),this);

  actQuit = new Action(1000,tr("Quit"),this);
  actQuit->setShortcut(tr("Ctrl+Q"));
  mnuMain->addAction(actQuit);

  actAbout = new Action(900,tr("About vacuum"),this);
  mnuAbout = new Menu(900,"mainwindow::aboutmenu",tr("About"),this);
  mnuAbout->addAction(actAbout);
  mnuMain->addMenuActions(mnuAbout); 

  FMainToolBar->addAction(mnuMain->menuAction()); 
}

void MainWindow::connectActions()
{
  connect(actQuit,SIGNAL(triggered()),FPluginManager->instance(),SLOT(quit())); 
}