/*! \file
 *
 * \date Created on: 17.02.2017
 * \author jochen.ulrich
 */

#ifndef QTPROMISE_DEFERRED_H_
#define QTPROMISE_DEFERRED_H_

#include <QObject>
#include <QVariant>
#include <QReadWriteLock>
#include <QException>
#include <QSharedPointer>


namespace QtPromise {

class Deferred;

#ifndef QT_NO_EXCEPTIONS

/*! \brief Exception indicating that a Deferred was destroyed while still pending.
 *
 * This exception is used to reject a Deferred when it is still pending when being destroyed.
 */
class DeferredDestroyed : public QException
{
public:
	DeferredDestroyed() : m_deferred(nullptr) {}
	DeferredDestroyed(const Deferred* const deferred) : m_deferred(deferred) {}
	virtual ~DeferredDestroyed() {};

	const Deferred* const deferred() const { return m_deferred; }

private:
	const Deferred* const m_deferred;
};

#endif // QT_NO_EXCEPTIONS


/*! \brief
 *
 * As a general rule when deriving from Deferred:
 * After construction, a Deferred must always be Pending.
 * The Deferred may only be resolved/rejected when explicitly requested by
 * the creator (i.e. calling resolve() or reject()) or when the control
 * returns to the event loop.
 * In other words: do not resolve/reject a Deferred in the constructor.
 * Instead use a QTimer::singleShot() with a \c 0 timeout to resolve/reject
 * the Deferred when the control returns to the event loop.
 *
 * \threadsafe
 */
class Deferred : public QObject
{
	Q_OBJECT

public:
	typedef QSharedPointer<Deferred> Ptr;

	enum State
	{
		Pending = 0,
		Resolved = 1,
		Rejected = -1
	};

	static Ptr create();
	virtual ~Deferred();

	Deferred::State state() const { QReadLocker locker(&m_lock); return m_state; }
	QVariant data() const { QReadLocker locker(&m_lock); return m_data; }

signals:
	void resolved(const QVariant& value) const;
	void rejected(const QVariant& reason) const;
	void notified(const QVariant& progress) const;

public slots:
	bool resolve(const QVariant& value);
	bool reject(const QVariant& reason);
	bool notify(const QVariant& progress);

protected:
	Deferred();

private:
	void logInvalidActionMessage(const char* action) const;

	mutable QReadWriteLock m_lock;
	State m_state;
	QVariant m_data;
};

}  // namespace QtPromise

Q_DECLARE_METATYPE(QtPromise::DeferredDestroyed)
Q_DECLARE_METATYPE(QtPromise::Deferred::State)

#endif /* QTPROMISE_DEFERRED_H_ */
