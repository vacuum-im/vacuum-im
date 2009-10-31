#include "tabwindow.h"

#include <QMessageBox>
#include <QInputDialog>

#define SVN_TABWINDOW               "tabWindow[]"
#define SVN_SHOW_CLOSE_BUTTONS      SVN_TABWINDOW":showCloseButtons"
#define BDI_TABWINDOW_GEOMETRY      "TabWindowGeometry"

#define ADR_TABWINDOWID             Action::DR_Parametr1

TabWindow::TabWindow(IMessageWidgets *AMessageWidgets, const QUuid &AWindowId)
{
  setAttribute(Qt::WA_DeleteOnClose,true);

  ui.setupUi(this);
  ui.twtTabs->widget(0)->deleteLater();
  ui.twtTabs->removeTab(0);
  ui.twtTabs->setMovable(true);
  ui.twtTabs->setTabsClosable(true);

  FWindowId = AWindowId;
  FMessageWidgets = AMessageWidgets;
  connect(FMessageWidgets->instance(),SIGNAL(tabWindowAppended(const QUuid &, const QString &)),
    SLOT(onTabWindowAppended(const QUuid &, const QString &)));
  connect(FMessageWidgets->instance(),SIGNAL(tabWindowNameChanged(const QUuid &, const QString &)),
    SLOT(onTabWindowNameChanged(const QUuid &, const QString &)));
  connect(FMessageWidgets->instance(),SIGNAL(defaultTabWindowChanged(const QUuid &)),
    SLOT(onDefaultTabWindowChanged(const QUuid &)));
  connect(FMessageWidgets->instance(),SIGNAL(tabWindowDeleted(const QUuid &)),SLOT(onTabWindowDeleted(const QUuid &)));

  QPushButton *menuButton = new QPushButton(ui.twtTabs);
  IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(menuButton,MNI_MESSAGEWIDGETS_TAB_MENU);
  menuButton->setFlat(true);
  
  FWindowMenu = new Menu(menuButton);
  menuButton->setMenu(FWindowMenu);
  ui.twtTabs->setCornerWidget(menuButton);

  initialize();
  loadWindowState();
  createActions();

  connect(ui.twtTabs,SIGNAL(currentChanged(int)),SLOT(onTabChanged(int)));
  connect(ui.twtTabs,SIGNAL(tabCloseRequested(int)),SLOT(onTabCloseRequested(int)));
}

TabWindow::~TabWindow()
{
  clear();
  saveWindowState();
  emit windowDestroyed();
}

void TabWindow::showWindow()
{
  isVisible() ? (isMinimized() ? showNormal() : activateWindow()) : show();
  raise();
}

QUuid TabWindow::windowId() const
{
  return FWindowId;
}

QString TabWindow::windowName() const
{
  return FMessageWidgets->tabWindowName(FWindowId);
}

Menu *TabWindow::windowMenu() const
{
  return FWindowMenu;
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

void TabWindow::detachWidget(ITabWidget *AWidget)
{
  removeWidget(AWidget);
  AWidget->instance()->show();
  if (AWidget->instance()->x()<=0 || AWidget->instance()->y()<0)
    AWidget->instance()->move(0,0);
  emit widgetDetached(AWidget);
}

void TabWindow::removeWidget(ITabWidget *AWidget)
{
  if (AWidget)
  {
    int index = ui.twtTabs->indexOf(AWidget->instance());
    if (index >=0)
    {
      AWidget->instance()->close();
      ui.twtTabs->removeTab(index);
      AWidget->instance()->setParent(NULL);
      disconnect(AWidget->instance(),SIGNAL(windowShow()),this,SLOT(onTabWidgetShow()));
      disconnect(AWidget->instance(),SIGNAL(windowClose()),this,SLOT(onTabWidgetClose()));
      disconnect(AWidget->instance(),SIGNAL(windowChanged()),this,SLOT(onTabWidgetChanged()));
      disconnect(AWidget->instance(),SIGNAL(windowDestroyed()),this,SLOT(onTabWidgetDestroyed()));
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
  IPlugin *plugin = FMessageWidgets->pluginManager()->pluginInterface("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    ISettingsPlugin *settingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (settingsPlugin)
    {
      FSettings = settingsPlugin->settingsForPlugin(MESSAGEWIDGETS_UUID);
    }
  }
}

void TabWindow::createActions()
{
  FNextTab = new Action(FWindowMenu);
  FNextTab->setText(tr("Next Tab"));
  FNextTab->setShortcut(tr("Ctrl+Tab"));
  FWindowMenu->addAction(FNextTab,AG_MWTW_MWIDGETS_TAB_ACTIONS);
  connect(FNextTab,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

  FPrevTab = new Action(FWindowMenu);
  FPrevTab->setText(tr("Prev. Tab"));
  FPrevTab->setShortcut(tr("Ctrl+Shift+Tab"));
  FWindowMenu->addAction(FPrevTab,AG_MWTW_MWIDGETS_TAB_ACTIONS);
  connect(FPrevTab,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

  FCloseTab = new Action(FWindowMenu);
  FCloseTab->setText(tr("Close Tab"));
  FCloseTab->setShortcut(tr("Esc"));
  FWindowMenu->addAction(FCloseTab,AG_MWTW_MWIDGETS_TAB_ACTIONS);
  connect(FCloseTab,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

  FDetachWindow = new Action(FWindowMenu);
  FDetachWindow->setText(tr("Detach to Separate Window"));
  FWindowMenu->addAction(FDetachWindow,AG_MWTW_MWIDGETS_TAB_ACTIONS);
  connect(FDetachWindow,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

  FJoinMenu = new Menu(FWindowMenu);
  FJoinMenu->setTitle(tr("Join to"));
  FWindowMenu->addAction(FJoinMenu->menuAction(),AG_MWTW_MWIDGETS_TAB_ACTIONS);
  foreach(QUuid windowId,FMessageWidgets->tabWindowList())
    if (windowId!=FWindowId)
      onTabWindowAppended(windowId, FMessageWidgets->tabWindowName(windowId));

  FNewTab = new Action(FJoinMenu);
  FNewTab->setText(tr("New Tab Window"));
  FJoinMenu->addAction(FNewTab,AG_DEFAULT+1);
  connect(FNewTab,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

  FShowCloseButtons = new Action(FWindowMenu);
  FShowCloseButtons->setText(tr("Tabs Closable"));
  FShowCloseButtons->setCheckable(true);
  FShowCloseButtons->setChecked(ui.twtTabs->tabsClosable());
  FWindowMenu->addAction(FShowCloseButtons,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
  connect(FShowCloseButtons,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

  FSetAsDefault = new Action(FWindowMenu);
  FSetAsDefault->setText(tr("Use as Default Tab Window"));
  FSetAsDefault->setCheckable(true);
  FWindowMenu->addAction(FSetAsDefault,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
  connect(FSetAsDefault,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

  FRenameWindow = new Action(FWindowMenu);
  FRenameWindow->setText(tr("Rename Tab Window"));
  FWindowMenu->addAction(FRenameWindow,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
  connect(FRenameWindow,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

  FDeleteWindow = new Action(FWindowMenu);
  FDeleteWindow->setText(tr("Delete Tab Window"));
  FWindowMenu->addAction(FDeleteWindow,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
  connect(FDeleteWindow,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

  onDefaultTabWindowChanged(FMessageWidgets->defaultTabWindow());
}

void TabWindow::saveWindowState()
{
  if (FSettings)
  {
    QString ns = FWindowId.toString();
    FSettings->setValueNS(SVN_SHOW_CLOSE_BUTTONS,ns,ui.twtTabs->tabsClosable());
    FSettings->saveBinaryData(BDI_TABWINDOW_GEOMETRY"|"+ns,saveGeometry());
  }
}

void TabWindow::loadWindowState()
{
  if (FSettings)
  {
    QString ns = FWindowId.toString();
    ui.twtTabs->setTabsClosable(FSettings->valueNS(SVN_SHOW_CLOSE_BUTTONS,ns,true).toBool());
    restoreGeometry(FSettings->loadBinaryData(BDI_TABWINDOW_GEOMETRY"|"+ns));
  }
}

void TabWindow::updateWindow()
{
  QWidget *widget = ui.twtTabs->currentWidget();
  if (widget)
  {
    setWindowIcon(widget->windowIcon());
    setWindowTitle(widget->windowTitle()+" - "+windowName());
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

void TabWindow::onTabChanged(int /*AIndex*/)
{
  updateWindow();
  emit currentChanged(currentWidget());
}

void TabWindow::onTabCloseRequested(int AIndex)
{
  removeWidget(qobject_cast<ITabWidget *>(ui.twtTabs->widget(AIndex)));
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
    removeWidget(widget);
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

void TabWindow::onTabWindowAppended(const QUuid &AWindowId, const QString &AName)
{
  Action *action = new Action(FJoinMenu);
  action->setText(AName);
  action->setData(ADR_TABWINDOWID,AWindowId.toString());
  FJoinMenu->addAction(action,AG_DEFAULT);
  connect(action,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));
}

void TabWindow::onTabWindowNameChanged(const QUuid &AWindowId, const QString &AName)
{
  if (AWindowId == FWindowId)
    updateWindow();

  foreach(Action *action, FJoinMenu->groupActions(AG_DEFAULT))
    if (AWindowId == action->data(ADR_TABWINDOWID).toString())
      action->setText(AName);
}

void TabWindow::onTabWindowDeleted(const QUuid &AWindowId)
{
  foreach(Action *action, FJoinMenu->groupActions(AG_DEFAULT))
    if (AWindowId == action->data(ADR_TABWINDOWID).toString())
      FJoinMenu->removeAction(action);
}

void TabWindow::onDefaultTabWindowChanged(const QUuid &AWindowId)
{
  FSetAsDefault->setChecked(FWindowId==AWindowId);
  FDeleteWindow->setVisible(FWindowId!=AWindowId);
}

void TabWindow::onActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action == FCloseTab)
  {
    removeWidget(currentWidget());
  }
  else if (action == FNextTab)
  {
    ui.twtTabs->setCurrentIndex((ui.twtTabs->currentIndex()+1) % ui.twtTabs->count());
  }
  else if (action == FPrevTab)
  {
    ui.twtTabs->setCurrentIndex(ui.twtTabs->currentIndex()>0 ? ui.twtTabs->currentIndex()-1 : ui.twtTabs->count()-1);
  }
  else if (action == FDetachWindow)
  {
    detachWidget(currentWidget());
  }
  else if (action == FNewTab)
  {
    QString name = QInputDialog::getText(this,tr("New Tab Window"),tr("Tab window name:"));
    if (!name.isEmpty())
    {
      ITabWidget *widget = currentWidget();
      removeWidget(widget);
      ITabWindow *window = FMessageWidgets->openTabWindow(FMessageWidgets->appendTabWindow(name));
      window->addWidget(widget);
    }
  }
  else if (action == FShowCloseButtons)
  {
    ui.twtTabs->setTabsClosable(action->isChecked());
  }
  else if (action == FSetAsDefault)
  {
    FMessageWidgets->setDefaultTabWindow(FWindowId);
    FSetAsDefault->setChecked(true);
  }
  else if (action == FRenameWindow)
  {
    QString name = QInputDialog::getText(this,tr("Rename Tab Window"),tr("Tab window name:"),QLineEdit::Normal,FMessageWidgets->tabWindowName(FWindowId));
    if (!name.isEmpty())
      FMessageWidgets->setTabWindowName(FWindowId,name);
  }
  else if (action == FDeleteWindow)
  {
    if (QMessageBox::question(this,tr("Delete Tab Window"),tr("Are you sure you want to delete this tab window?"),
      QMessageBox::Ok|QMessageBox::Cancel) == QMessageBox::Ok)
    {
      FMessageWidgets->deleteTabWindow(FWindowId);
    }
  }
  else if (FJoinMenu->groupActions(AG_DEFAULT).contains(action))
  {
    ITabWidget *widget = currentWidget();
    removeWidget(widget);

    ITabWindow *window = FMessageWidgets->openTabWindow(action->data(ADR_TABWINDOWID).toString());
    window->addWidget(widget);
  }
}
