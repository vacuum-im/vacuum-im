#include "mainwindow.h"

#include <QApplication>

MainWindow::MainWindow(QWidget *AParent, Qt::WindowFlags AFlags) : QMainWindow(AParent,AFlags)
{
  setAttribute(Qt::WA_DeleteOnClose,false);

  QIcon icon;
  IconStorage *iconStorage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);
  icon.addFile(iconStorage->fileFullName(MNI_MAINWINDOW_LOGO16), QSize(16,16));
  icon.addFile(iconStorage->fileFullName(MNI_MAINWINDOW_LOGO24), QSize(24,24));
  icon.addFile(iconStorage->fileFullName(MNI_MAINWINDOW_LOGO32), QSize(32,32));
  icon.addFile(iconStorage->fileFullName(MNI_MAINWINDOW_LOGO48), QSize(48,48));
  icon.addFile(iconStorage->fileFullName(MNI_MAINWINDOW_LOGO64), QSize(64,64));
  icon.addFile(iconStorage->fileFullName(MNI_MAINWINDOW_LOGO96), QSize(96,96));
  icon.addFile(iconStorage->fileFullName(MNI_MAINWINDOW_LOGO128), QSize(128,128));
  setWindowIcon(icon);

  setIconSize(QSize(16,16));
  createLayouts();
  createToolBars();
  createMenus();
}

MainWindow::~MainWindow()
{

}

QMenu *MainWindow::createPopupMenu()
{
  return NULL;
}

void MainWindow::createLayouts()
{
  FUpperWidget = new QStackedWidget; 
  FUpperWidget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
  FUpperWidget->layout()->setSizeConstraint(QLayout::SetFixedSize);
  FUpperWidget->setVisible(false);
  connect(FUpperWidget,SIGNAL(widgetRemoved(int)),SLOT(onStackedWidgetRemoved(int)));

  FRostersWidget = new QStackedWidget; 
  FRostersWidget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

  FBottomWidget = new QStackedWidget; 
  FBottomWidget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);  
  FBottomWidget->layout()->setSizeConstraint(QLayout::SetFixedSize);
  FBottomWidget->setVisible(false);
  connect(FBottomWidget,SIGNAL(widgetRemoved(int)),SLOT(onStackedWidgetRemoved(int)));
 
  FMainLayout = new QVBoxLayout;
  FMainLayout->setMargin(2);
  FMainLayout->addWidget(FUpperWidget);  
  FMainLayout->addWidget(FRostersWidget);  
  FMainLayout->addWidget(FBottomWidget);  

  QWidget *centralWidget = new QWidget(this);
  centralWidget->setLayout(FMainLayout); 
  setCentralWidget(centralWidget);
}

void MainWindow::createToolBars()
{
  QToolBar *toolbar = new QToolBar(tr("Top toolbar"), this);
  toolbar->setFloatable(false);
  toolbar->setMovable(false);
  addToolBar(Qt::TopToolBarArea,toolbar);
  FTopToolBarChanger = new ToolBarChanger(toolbar);
  FTopToolBarChanger->setSeparatorsVisible(false);

  toolbar = new QToolBar(tr("Left toolbar"), this);
  toolbar->setFloatable(false);
  toolbar->setMovable(false);
  addToolBar(Qt::LeftToolBarArea,toolbar);
  FLeftToolBarChanger = new ToolBarChanger(toolbar);
  FLeftToolBarChanger->setSeparatorsVisible(false);

  toolbar =  new QToolBar(tr("Bottom toolbar"), this);
  toolbar->setFloatable(false);
  toolbar->setMovable(false); 
  addToolBar(Qt::BottomToolBarArea,toolbar);
  FBottomToolBarChanger = new ToolBarChanger(toolbar);
  FBottomToolBarChanger->setSeparatorsVisible(false);
}

void MainWindow::createMenus()
{
  FMainMenu = new Menu(this);
  FMainMenu->setTitle(tr("Menu"));
  FMainMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_MAINWINDOW_MENU);
  QToolButton *button = FBottomToolBarChanger->insertAction(FMainMenu->menuAction());
  button->setPopupMode(QToolButton::InstantPopup);
}

void MainWindow::onStackedWidgetRemoved(int /*AIndex*/)
{
  QStackedWidget *widget = qobject_cast<QStackedWidget *>(sender());
  if (widget == FUpperWidget)
    FUpperWidget->setVisible(FUpperWidget->count() > 0);
  else if (widget == FBottomWidget)
    FBottomWidget->setVisible(FBottomWidget->count() > 0);
}
