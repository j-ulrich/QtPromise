/*! \file
 *
 * \date Created on: 17.02.2017
 * \author jochen.ulrich
 */

#ifndef SRC_DEFERRED_H_
#define SRC_DEFERRED_H_

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
	/*! Constructs the exception without a Deferred. */
	DeferredDestroyed() : m_deferred(nullptr) {}
	/*! Constructs the exception and saves the Deferred for debugging purposes.
	 *
	 * \param deferred Pointer to the Deferred being destroyed.
	 */
	DeferredDestroyed(const Deferred* const deferred) : m_deferred(deferred) {}
	/*! Default destructor. */
	virtual ~DeferredDestroyed() {};

	/*! \return The pointer to the Deferred being destroyed.
	 * Can be a \c nullptr, depending on how the exception was constructed.
	 */
	const Deferred* const deferred() const { return m_deferred; }

private:
	const Deferred* const m_deferred;
};

#endif // QT_NO_EXCEPTIONS


/*! \brief Reports the outcome of an operation.
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
 * \threadsafeClass
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

	/*! Creates a pending Deferred object.
	 *
	 * @return QSharedPointer to a new, pending Deferred.
	 */
	static Ptr create();
	virtual ~Deferred();

	Deferred::State state() const { QReadLocker locker(&m_lock); return m_state; }
	QVariant data() const { QReadLocker locker(&m_lock); return m_data; }

signals:
	void resolved(const QVariant& value) const;
	void rejected(const QVariant& reason) const;
	void notified(const QVariant& progress) const;

public slots:
	/*! Communicates success of the asynchronous operation.
	 *
	 * @param value The result of the asynchronous operation.
	 * @return \c true if the Deferred has been set to Resolved.
	 * \c false if the Deferred was not in the Pending state before.
	 */
	bool resolve(const QVariant& value);
	bool reject(const QVariant& reason);
	bool notify(const QVariant& progress);

protected:
	Deferred();

	mutable QReadWriteLock m_lock;
	State m_state;
	QVariant m_data;

private:
	void logInvalidActionMessage(const char* action) const;

};

}  // namespace QtPromise

Q_DECLARE_METATYPE(QtPromise::DeferredDestroyed)
Q_DECLARE_METATYPE(QtPromise::Deferred::State)

#endif /* SRC_DEFERRED_H_ */
