#ifndef IANNOTATIONS_H
#define IANNOTATIONS_H

#include <QDateTime>
#include <utils/jid.h>

#define ANNOTATIONS_UUID  "{529e8149-5e10-465b-a026-f7d08c637955}"

class IAnnotations {
public:
  virtual QObject *instance() =0;
  virtual bool isEnabled(const Jid &AStreamJid) const =0;
  virtual QList<Jid> annotations(const Jid &AStreamJid) const =0;
  virtual QString annotation(const Jid &AStreamJid, const Jid &AContactJid) const =0;
  virtual QDateTime annotationCreateDate(const Jid &AStreamJid, const Jid &AContactJid) const =0;
  virtual QDateTime annotationModifyDate(const Jid &AStreamJid, const Jid &AContactJid) const =0;
  virtual void setAnnotation(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANote) =0;
  virtual bool loadAnnotations(const Jid &AStreamJid) =0;
  virtual bool saveAnnotations(const Jid &AStreamJid) =0;
signals:
  virtual void annotationsLoaded(const Jid &AStreamJid) =0;
  virtual void annotationsSaved(const Jid &AStreamJid) =0;
  virtual void annotationsError(const Jid &AStreamJid, const QString &AError) =0;
  virtual void annotationModified(const Jid &AStreamJid, const Jid &AContactJid) =0;
};

Q_DECLARE_INTERFACE(IAnnotations,"Vacuum.Plugin.IAnnotations/1.0")

#endif
