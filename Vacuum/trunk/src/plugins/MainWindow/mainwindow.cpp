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
  FRostersLayout = new RosterLayout(0);
  FRostersLayout->setMargin(2);
  FRostersArea = new QScrollArea;
  FRostersArea->setSizePolicy(QSizePolicy::Expanding ,QSizePolicy::Expanding);
  QWidget *scrollWidget = new QWidget;
  scrollWidget->setLayout(FRostersLayout);
  FRostersArea->setWidget(scrollWidget);
  FRostersArea->setWidgetResizable(true);

  FUpperWidget = new QStackedWidget; 
  FUpperWidget->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Maximum);

  FMiddleWidget = new QStackedWidget; 
  FMiddleWidget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
  FMiddleWidget->addWidget(FRostersArea); 

  FBottomWidget = new QStackedWidget; 
  FBottomWidget->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Maximum);  
                     
  FMainLayout = new QVBoxLayout;
  FMainLayout->addWidget(FUpperWidget);  
  FMainLayout->addWidget(FMiddleWidget);  
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

  actAbout = new Action(900,tr("About program"),this);
  mnuAbout = new Menu(900,"mainwindow::aboutmenu",tr("About"),this);
  mnuAbout->addAction(actAbout);
  mnuMain->addMenuActions(mnuAbout); 

  FMainToolBar->addAction(mnuMain->menuAction()); 
}

void MainWindow::connectActions()
{
  connect(actQuit,SIGNAL(triggered()),FPluginManager->instance(),SLOT(quit())); 
}