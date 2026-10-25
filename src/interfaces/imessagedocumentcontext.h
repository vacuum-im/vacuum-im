#ifndef IMESSAGEDOCUMENTCONTEXT_H
#define IMESSAGEDOCUMENTCONTEXT_H

#include <QPointer>
#include <QTextCursor>
#include <QTextDocument>

// Identifies one message inside a QTextDocument managed by MessageWidgets.
// The marker is maintained by QTextCursor, while id and generation keep the
// context stable when other messages are removed or the document is cleared.
struct MessageDocumentContext
{
    QPointer<QTextDocument> document;
    QTextCursor marker;
    quint64 generation = 0;
    quint64 id = 0;
    bool atStart = false;
};

// Owns message-boundary bookkeeping for text-based message views. Feature
// plugins can keep a MessageDocumentContext, but must not manage the shared
// marker list themselves.
class IMessageDocumentContextManager
{
public:
    virtual bool beginMessageContext(QTextDocument *ADocument, MessageDocumentContext &AContext) = 0;
    virtual bool isMessageContextValid(const MessageDocumentContext &AContext) const = 0;
    // Returns the position where the next message's content starts, or -1 if
    // this is the last message. The position is an exclusive upper bound for
    // this message's own content.
    virtual int nextMessagePosition(const MessageDocumentContext &AContext) const = 0;
    // Invalidates every context associated with a full document reset.
    virtual void resetMessageContexts(QTextDocument *ADocument) = 0;
    // Call before removing a range containing whole messages. This removes
    // contexts whose message start lies in [APosition, APosition + ALength).
    virtual void removeMessageContextsInRange(QTextDocument *ADocument, int APosition, int ALength) = 0;
};

Q_DECLARE_INTERFACE(IMessageDocumentContextManager, "Vacuum.Plugin.IMessageDocumentContextManager/1.0")

#endif // IMESSAGEDOCUMENTCONTEXT_H
