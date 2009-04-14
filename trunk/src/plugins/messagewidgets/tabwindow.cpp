#include "tabwindow.h"

#define BDI_TABWINDOW_GEOMETRY      "TabWindowGeometry"

#define ADR_TABWINDOWID             Action::DR_Parametr1

TabWindow::TabWindow(IMessageWidgets *AMessageWidgets, int AWindowId)
{
  setAttribute(Qt::WA_DeleteOnClose,true);

  ui.setupUi(this);

  FWindowId = AWindowId;
  FMessageWidgets = AMessageWidgets;
  connect(FMessageWidgets->instance(),SIGNAL(tabWindowCreated(ITabWindow *)),SLOT(onTabWindowCreated(ITabWindow *)));
  connect(FMessageWidgets->instance(),SIGNAL(tabWindowDestroyed(ITabWindow *)),SLOT(onTabWindowDestroyed(ITabWindow *)));

  FCloseButton = new QToolButton(ui.twtTabs);
  ui.twtTabs->setCornerWidget(FCloseButton);

  FCloseAction = new Action(FCloseButton);
  FCloseAction->setIcon(RSR_STORAGE_MENUICONS,MNI_MESSAGEWIDGETS_CLOSE_TAB);
  FCloseAction->setText(tr("Close tab"));
  FCloseAction->setShortcut(tr("Esc"));
  FCloseButton->setDefaultAction(FCloseAction);
  connect(FCloseAction,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));
  
  FTabMenu = new Menu(FCloseButton);
  FTabMenu->addAction(FCloseAction,AG_DEFAULT-1);
  FCloseButton->setMenu(FTabMenu);
  FCloseButton->setPopupMode(QToolButton::DelayedPopup);

  FNewTabAction = new Action(FTabMenu);
  FNewTabAction->setText(tr("New tab window"));
  FTabMenu->addAction(FNewTabAction,AG_DEFAULT+1);
  connect(FNewTabAction,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));
  
  FDetachWindowAction = new Action(FTabMenu);
  FDetachWindowAction->setText(tr("Detach window"));
  FTabMenu->addAction(FDetachWindowAction,AG_DEFAULT+1);
  connect(FDetachWindowAction,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

  FNextTabAction = new Action(FTabMenu);
  FNextTabAction->setText(tr("Next tab"));
  FNextTabAction->setShortcut(tr("Ctrl+Tab"));
  FTabMenu->addAction(FNextTabAction,AG_DEFAULT-1);
  connect(FNextTabAction,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

  FPrevTabAction = new Action(FTabMenu);
  FPrevTabAction->setText(tr("Prev. tab"));
  FPrevTabAction->setShortcut(tr("Ctrl+Shift+Tab"));
  FTabMenu->addAction(FPrevTabAction,AG_DEFAULT-1);
  connect(FPrevTabAction,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

  ui.twtTabs->widget(0)->deleteLater();
  ui.twtTabs->removeTab(0);
  connect(ui.twtTabs,SIGNAL(currentChanged(int)),SLOT(onTabChanged(int)));

  initialize();
}

TabWindow::~TabWindow()
{
  clear();
  emit windowDestroyed();
}

void TabWindow::showWindow()
{
  isVisible() ? (isMinimized() ? showNormal() : activateWindow()) : show();
  raise();
}

void TabWindow::addWidget(ITabWidget *AWidget)
{
  int index = ui.twtTabs->addTab(AWidget->instance(),AWidget->instance()->windowTitle());
  connect(AWidget->instance(),SIGNAL(windowShow()),SLOT(onTabWidgetShow()));
  connect(AWidget->instance(),SIGNAL(windowClose()),SLOT(onTabWidgetClose()));
  connect(AWidget->instance(),SIGNAL(windowChanged()),SLOT(onTabWidgetChanged()));
  connect(AWidget->instance(),SIGNAL(windowDestroyed()),SLOT(onTabWidgetDestroyed()));
  updateTab(index);
  updateWindow();
  FNewTabAction->setVisible(ui.twtTabs->count()>1);
  emit widgetAdded(AWidget);
}

bool TabWindow::hasWidget(ITabWidget *AWidget) const
{
  return ui.twtTabs->indexOf(AWidget->instance()) >= 0;
}

ITabWidget *TabWindow::currentWidget() const
{
  return qobject_cast<ITabWidget *>(ui.twtTabs->currentWidget());
}

void TabWindow::setCurrentWidget(ITabWidget *AWidget)
{
  if (AWidget)
    ui.twtTabs->setCurrentWidget(AWidget->instance());
}

void TabWindow::removeWidget(ITabWidget *AWidget)
{
  if (AWidget)
  {
    int index = ui.twtTabs->indexOf(AWidget->instance());
    if (index >=0)
    {
      ui.twtTabs->removeTab(index);
      AWidget->instance()->close();
      AWidget->instance()->setParent(NULL);
      disconnect(AWidget->instance(),SIGNAL(windowShow()),this,SLOT(onTabWidgetShow()));
      disconnect(AWidget->instance(),SIGNAL(windowClose()),this,SLOT(onTabWidgetClose()));
      disconnect(AWidget->instance(),SIGNAL(windowChanged()),this,SLOT(onTabWidgetChanged()));
      disconnect(AWidget->instance(),SIGNAL(windowDestroyed()),this,SLOT(onTabWidgetDestroyed()));
      FNewTabAction->setVisible(ui.twtTabs->count()>1);
      emit widgetRemoved(AWidget);
      if (ui.twtTabs->count() == 0)
        close();
    }
  }
}

void TabWindow::clear()
{
  while (ui.twtTabs->count() > 0)
  {
    ITabWidget *widget = qobject_cast<ITabWidget *>(ui.twtTabs->widget(0));
    if (widget)
      removeWidget(widget);
    else
      ui.twtTabs->removeTab(0);
  }
}

void TabWindow::initialize()
{
  IPlugin *plugin = FMessageWidgets->pluginManager()->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    ISettingsPlugin *settingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (settingsPlugin)
    {
      FSettings = settingsPlugin->settingsForPlugin(MESSAGEWIDGETS_UUID);
    }
  }

  QList<int> windows = FMessageWidgets->tabWindows();
  foreach(int windowId,windows)
    onTabWindowCreated(FMessageWidgets->findTabWindow(windowId));

  loadWindowState();
}

void TabWindow::saveWindowState()
{
  if (FSettings)
  {
    FSettings->saveBinaryData(BDI_TABWINDOW_GEOMETRY+QString::number(FWindowId),saveGeometry());
  }
}

void TabWindow::loadWindowState()
{
  if (FSettings)
  {
    restoreGeometry(FSettings->loadBinaryData(BDI_TABWINDOW_GEOMETRY+QString::number(FWindowId)));
  }
}

void TabWindow::updateWindow()
{
  QWidget *widget = ui.twtTabs->currentWidget();
  if (widget)
  {
    setWindowIcon(widget->windowIcon());
    setWindowTitle(widget->windowTitle());
    emit windowChanged();
  }
}

void TabWindow::updateTab(int AIndex)
{
  QWidget *widget = ui.twtTabs->widget(AIndex);
  if (widget)
  {
    ui.twtTabs->setTabIcon(AIndex,widget->windowIcon());
    ui.twtTabs->setTabText(AIndex,widget->windowIconText());
  }
}

void TabWindow::closeEvent(QCloseEvent *AEvent)
{
  saveWindowState();
  QMainWindow::closeEvent(AEvent);
}

void TabWindow::onTabChanged(int /*AIndex*/)
{
  updateWindow();
  emit currentChanged(currentWidget());
}

void TabWindow::onTabWidgetShow()
{
  ITabWidget *widget = qobject_cast<ITabWidget *>(sender());
  if (widget)
  {
    setCurrentWidget(widget);
    showWindow();
  }
}

void TabWindow::onTabWidgetClose()
{
  ITabWidget *widget = qobject_cast<ITabWidget *>(sender());
  if (widget)
  {
    removeWidget(widget);
  }
}

void TabWindow::onTabWidgetChanged()
{
  ITabWidget *widget = qobject_cast<ITabWidget *>(sender());
  if (widget)
  {
    int index = ui.twtTabs->indexOf(widget->instance());
    updateTab(index);
    if (index == ui.twtTabs->currentIndex())
      updateWindow();
  }
}

void TabWindow::onTabWidgetDestroyed()
{
  ITabWidget *widget = qobject_cast<ITabWidget *>(sender());
  if (widget)
    removeWidget(widget);
}

void TabWindow::onTabWindowCreated(ITabWindow *AWindow)
{
  if (AWindow != this)
  {
    Action *action = new Action(FTabMenu);
    action->setText(AWindow->instance()->windowTitle());
    action->setIcon(AWindow->instance()->windowIcon());
    action->setData(ADR_TABWINDOWID,AWindow->windowId());
    FTabMenu->addAction(action,AG_DEFAULT,true);
    FMoveActions.insert(AWindow->windowId(),action);
    connect(action,SIGNAL(triggered(bool)),SLOT(onMoveTabWindowAction(bool)));
    connect(AWindow->instance(),SIGNAL(windowChanged()),SLOT(onTabWindowChanged()));
  }
}

void TabWindow::onTabWindowChanged()
{
  ITabWindow *window = qobject_cast<ITabWindow *>(sender());
  if (window)
  {
    Action *action = FMoveActions.value(window->windowId());
    if (action)
    {
      action->setText(window->instance()->windowTitle());
      action->setIcon(window->instance()->windowIcon());
    }
  }
}

void TabWindow::onTabWindowDestroyed(ITabWindow *AWindow)
{
  Action *action = FMoveActions.value(AWindow->windowId());
  if (action)
  {
    FTabMenu->removeAction(action);
    FMoveActions.remove(AWindow->windowId());
    delete action;
  }
}

void TabWindow::onActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action == FCloseAction)
  {
    removeWidget(currentWidget());
  }
  else if (action == FNewTabAction)
  {
    int windowId = 0;
    QList<int> windows = FMessageWidgets->tabWindows();
    while (windows.contains(windowId))
      windowId++;

    ITabWidget *widget = currentWidget();
    removeWidget(widget);

    ITabWindow *window = FMessageWidgets->openTabWindow(windowId);
    window->addWidget(widget);
  }
  else if (action == FDetachWindowAction)
  {
    ITabWidget *widget = currentWidget();
    removeWidget(widget);

    widget->instance()->setParent(NULL);
    widget->instance()->show();
    
    if (widget->instance()->x()<0 || widget->instance()->y()<0)
      widget->instance()->move(0,0);
  }
  else if (action == FNextTabAction)
  {
    ui.twtTabs->setCurrentIndex((ui.twtTabs->currentIndex()+1) % ui.twtTabs->count());
  }
  else if (action == FPrevTabAction)
  {
    ui.twtTabs->setCurrentIndex(ui.twtTabs->currentIndex()>0 ? ui.twtTabs->currentIndex()-1 : ui.twtTabs->count()-1);
  }
  else if (FMoveActions.values().contains(action))
  {
    ITabWidget *widget = currentWidget();
    removeWidget(widget);

    int windowId = action->data(ADR_TABWINDOWID).toInt();

    ITabWindow *window = FMessageWidgets->openTabWindow(windowId);
    window->addWidget(widget);
  }
}
