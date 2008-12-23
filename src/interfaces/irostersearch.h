#ifndef IROSTERSEARCH_H
#define IROSTERSEARCH_H

#include "../../utils/menu.h"

#define ROSTERSEARCH_UUID   "{69632D37-C382-8b0d-C5DA-627A65D9DC8A}"

class IRosterSearch {
public:
  virtual QObject *instance() =0;
  virtual void startSearch() =0;
  virtual QString searchPattern() const =0;
  virtual void setSearchPattern(const QString &APattern) =0;
  virtual bool isSearchEnabled() const =0;
  virtual void setSearchEnabled(bool AEnabled) =0;
  virtual void insertSearchField(int ADataRole, const QString &AName, bool AEnabled) =0;
  virtual Menu *searchFieldsMenu() const =0;
  virtual QList<int> searchFields() const =0;
  virtual bool isSearchFieldEnabled(int ADataRole) const =0;
  virtual void setSearchFieldEnabled(int ADataRole, bool AEnabled) =0;
  virtual void removeSearchField(int ADataRole) =0;
signals:
  virtual void searchResultUpdated() =0;
  virtual void searchStateChanged(bool AEnabled) =0;
  virtual void searchPatternChanged(const QString &APattern) =0;
  virtual void searchFieldInserted(int ADataRole, const QString &AName) =0;
  virtual void searchFieldChanged(int ADataRole) =0;
  virtual void searchFieldRemoved(int ADataRole) =0;
};

Q_DECLARE_INTERFACE(IRosterSearch,"Vacuum.Plugin.IRosterSearch/1.0")

#endif
