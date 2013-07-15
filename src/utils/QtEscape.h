#ifndef QTESCAPE_H_
#define QTESCAPE_H_

#if QT_VERSION >= 0x050000
#  include <QString>

namespace Qt {
	inline QString escape(const QString &plain) { return plain.toHtmlEscaped(); }
}
#else
#  include <QTextDocument>
#endif

#endif /* QTESCAPE_H_ */

