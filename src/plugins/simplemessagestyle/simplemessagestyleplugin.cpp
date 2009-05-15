#include "simplemessagestyleplugin.h"

#include <QTextBrowser>
#include <QFile>
#include <QTextDocument>

SimpleMessageStylePlugin::SimpleMessageStylePlugin()
{

}

SimpleMessageStylePlugin::~SimpleMessageStylePlugin()
{

}

void SimpleMessageStylePlugin::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Implements basic message styles.");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Simple Message Style"); 
  APluginInfo->uid = SIMPLEMESSAGESTYLE_UUID;
  APluginInfo->version = "0.1";
}

bool SimpleMessageStylePlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  return true;
}

bool SimpleMessageStylePlugin::startPlugin()
{
  //QTextBrowser *viewer = new QTextBrowser();

  //QFile css("main.css");
  //css.open(QFile::ReadOnly);
  //viewer->document()->setDefaultStyleSheet(QString::fromUtf8(css.readAll()));

  //QFile html("Mockup.html");
  //html.open(QFile::ReadOnly);
  //viewer->insertHtml(QString::fromUtf8(html.readAll()));

  //viewer->show();
  return true;
}
Q_EXPORT_PLUGIN2(SimpleMessageStylePlugin, SimpleMessageStylePlugin)
