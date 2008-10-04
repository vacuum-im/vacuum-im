#ifndef DISCOINFOWINDOW_H
#define DISCOINFOWINDOW_H

#include <QDialog>
#include "../../interfaces/iservicediscovery.h"
#include "ui_discoinfowindow.h"

class DiscoInfoWindow : 
  public QDialog
{
  Q_OBJECT;
public:
  DiscoInfoWindow(IServiceDiscovery *ADiscovery, const Jid &AStreamJid, const Jid &AContactJid, 
    const QString &ANode, QWidget *AParent = NULL);
  ~DiscoInfoWindow();
  virtual Jid streamJid() const { return FStreamJid; }
  virtual Jid contactJid() const { return FContactJid; }
  virtual QString node() const { return FNode; }
protected:
  void updateWindow();
  void requestDiscoInfo();
protected slots:
  void onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo);
  void onCurrentFeatureChanged(QListWidgetItem *ACurrent, QListWidgetItem *APrevious);
  void onUpdateClicked();
  void onListItemActivated(QListWidgetItem *AItem);
  void onStreamJidChanged(const Jid &ABefour, const Jid &AAfter);
private:
  Ui::DiscoInfoWindowClass ui;
private:
  IServiceDiscovery *FDiscovery;
private:
  Jid FStreamJid;
  Jid FContactJid;
  QString FNode;
};

#endif // DISCOINFOWINDOW_H
