#include <QtDebug>
#include "mainwindow.h"

#define IN_JABBER "psi/jabber"

MainWindow::MainWindow(QWidget *AParent, Qt::WindowFlags AFlags):IMainWindow(AParent,AFlags)
{
  setAttribute(Qt::WA_DeleteOnClose,false);
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
  return IMainWindow::createPopupMenu();
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
  QToolBar *toolbar = addToolBar(tr("Top toolbar"));
  toolbar->setFloatable(false);
  toolbar->setMovable(false);
  addToolBar(Qt::TopToolBarArea,toolbar);
  FTopToolBarChanger = new ToolBarChanger(toolbar);
  FTopToolBarChanger->setSeparatorsVisible(false);

  toolbar = addToolBar(tr("Left toolbar"));
  toolbar->setFloatable(false);
  toolbar->setMovable(false);
  addToolBar(Qt::LeftToolBarArea,toolbar);
  FLeftToolBarChanger = new ToolBarChanger(toolbar);
  FLeftToolBarChanger->setSeparatorsVisible(false);

  toolbar = addToolBar(tr("Bottom toolbar"));
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
  FMainMenu->setIcon(SYSTEM_ICONSETFILE,IN_JABBER);
  QToolButton *button = FBottomToolBarChanger->addToolButton(FMainMenu->menuAction(),AG_DEFAULT,false);
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
