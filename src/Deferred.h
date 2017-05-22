/*! \file
 *
 * \date Created on: 17.02.2017
 * \author jochen.ulrich
 */

#ifndef QTPROMISE_DEFERRED_H_
#define QTPROMISE_DEFERRED_H_

#include <QObject>
#include <QVariant>
#include <QMutex>
#include <QException>
#include <QSharedPointer>


namespace QtPromise {

class Deferred;

#ifndef QT_NO_EXCEPTIONS

/*! \brief Exception indicating that a Deferred was destroyed while still pending.
 *
 * This exception is used to reject a Deferred when it is still pending when being destroyed.
 *
 * \note If QtPromise is compiled with \c QT_NO_EXCEPTIONS defined, this class is omitted.
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
	virtual ~DeferredDestroyed() = default;

	/*! \return The pointer to the Deferred being destroyed.
	 * Can be a \c nullptr, depending on how the exception was constructed.
	 */
	const Deferred* const deferred() const { return m_deferred; }

private:
	const Deferred* const m_deferred;
};

#endif // QT_NO_EXCEPTIONS


/*! \brief Communicates the outcome of an asynchronous operation.
 *
 * The usage pattern of Deferred is:
 * - Create a new Deferred
 * - Prepare the asynchronous operation
 * - Configure that the Deferred is resolved or rejected when the
 * result of the asynchronous operation is available
 * - Possibly: configure that the Deferred is notified when the
 * asynchronous operation progresses
 * - Start the asynchronous operation
 * - Create a Promise for the Deferred and return it
 *
 * For example:
\code
using namespace QtPromise;

Deferred::Ptr deferred = Deferred::create();
MyAsyncObject* asyncObj = new MyAsyncObject(deferred.data()); // We use the Deferred as parent of
                                                              // asyncObj so we do not leak memory.
connect(asyncObj, &MyAsyncObject::success, deferred.data(), &Deferred::resolve);
connect(asyncObj, &MyAsyncObject::failure, deferred.data(), &Deferred::reject);
connect(asyncObj, &MyAsyncObject::progress, deferred.data(), &Deferred::notify);
asyncObj->start();
return Promise::create(deferred);
\endcode
 *
 * ## Subclassing ##
 * As a general rule when deriving from Deferred:
 * After construction, a Deferred must always be Pending.
 * The Deferred may only be resolved/rejected when explicitly requested by
 * the creator (i.e. calling resolve() or reject()) or when the control
 * returns to the event loop.\n
 * In other words: do not resolve/reject a Deferred directly in the constructor.
 * Instead use a QTimer::singleShot() with a \c 0 timeout to resolve/reject
 * the Deferred when the control returns to the event loop.
 *
 * When providing specialized resolved() / rejected() / notified() signals in subclasses
 * (see for example NetworkDeferred::resolved()), check if the return value of
 * resolve() / reject() / notify() is \c true before emitting those signals.
 *
 * \threadsafeClass
 * \author jochen.ulrich
 */
class Deferred : public QObject
{
	Q_OBJECT

public:
	/*! Smart pointer to Deferred. */
	typedef QSharedPointer<Deferred> Ptr;

	/*! Possible states of a Deferred or Promise. */
	enum State
	{
		Pending = 0, /*!< The outcome of the asynchronous operation has not
		              * been reported yet.
		              */
		Resolved = 1,//!< The asynchronous operation completed successfully.
		Rejected = -1//!< The asynchronous operation failed.
	};

	/*! Creates a pending Deferred object.
	 *
	 * \return QSharedPointer to a new, pending Deferred.
	 */
	static Ptr create();
	/*! Creates a resolved or rejected Deferred.
	 *
	 * This is a convenience method which creates a Deferred and directly
	 * resolves or rejects it with \p data according to \p state.
	 *
	 * \param state Defines whether the Deferred should resolved or rejected.
	 * Should be either Resolved or Rejected. Pending will be treated like Resolved.
	 * \param data The value or rejection reason used to resolve or reject
	 * the Deferred depending on \p state.
	 * \return A QSharedPointer to a new, resolved or rejected Deferred.
	 */
	static Ptr create(State state, const QVariant& data);
	/*! Destroys a Deferred.
	 *
	 * When the Deferred is still pending when being destroyed,
	 * it logs a warning using qDebug() and rejects the Deferred with either
	 * a DeferredDestroyed exception if exceptions are enabled or
	 * a QString if exceptions are disabled (\c QT_NO_EXCEPTIONS is defined).
	 */
	virtual ~Deferred();

	/*! \return The current state of the Deferred. */
	Deferred::State state() const { QMutexLocker locker(&m_lock); return m_state; }
	/*! \return The current data of the Deferred.
	 * Depending on the state of the Deferred, this is either the resolve value,
	 * the rejection reason or an invalid QVariant when the Deferred is still pending.
	 */
	QVariant data() const { QMutexLocker locker(&m_lock); return m_data; }

signals:
	/*! Emitted when the asynchronous operation was successful.
	 *
	 * \param value The result of the asynchronous operation.
	 * The actual type of the value depends on the asynchronous operation
	 * and should be documented by the creator of the Deferred.
	 */
	void resolved(const QVariant& value) const;
	/*! Emitted when the asynchronous operation failed.
	 *
	 * \param reason Indication of the reason why the operation failed.
	 * The actual type of the value depends on the asynchronous operation
	 * and should be documented by the creator of the Deferred.
	 */
	void rejected(const QVariant& reason) const;
	/*! Emitted to notify about progress of the asynchronous operation.
	 *
	 * \param progress Information about the progress of the operation.
	 * The actual type of the progress depends on the asynchronous operation
	 * and should be documented by the creator of the Deferred.
	 */
	void notified(const QVariant& progress) const;

public slots:
	/*! Communicates success of the asynchronous operation.
	 *
	 * Call this slot when the asynchronous operation succeeded to submit
	 * the result to the promises.
	 * It is important to make the actual type of the value clear for the promises
	 * as QVariant::value() requires the exact type to be able to convert the value.
	 *
	 * \param value The result of the asynchronous operation.
	 * \return \c true if the Deferred has been set to \ref Resolved.
	 * \c false if the Deferred was not in the \ref Pending state.
	 */
	bool resolve(const QVariant& value = QVariant());
	/*! Communicates failure of the asynchronous operation.
	 *
	 * Call this slot when the asynchronous operation failed and provide a reason
	 * describing why the operation failed.
	 * It is important to make the actual type of the value clear for the promises
	 * as QVariant::value() requires the exact type to be able to convert the value.
	 *
	 * \param reason An object indicating why the operation failed.
	 * \return \c true if the Deferred has been set to \ref Rejected.
	 * \c false if the Deferred was not in the \ref Pending state.
	 */
	bool reject(const QVariant& reason = QVariant());
	/*! Communicates progress of the asynchronous operation.
	 *
	 * Call this slot when you want to inform the promises about progress
	 * of the asynchronous operation.
	 * It is important to make the actual type of the value clear for the promises
	 * as QVariant::value() requires the exact type to be able to convert the value.
	 *
	 * \param progress An object representing the progress of the operation.
	 * \return \c true if the Deferred has been notified.
	 * \c false if the Deferred was not in the \ref Pending state.
	 */
	bool notify(const QVariant& progress = QVariant());

protected:
	/*! Creates a pending Deferred object.
	 */
	Deferred();
	/*! Defines whether the Deferred logs a debug message when resolve() or
	 * reject() is called when the Deferred is already resolved/rejected.
	 *
	 * By default, the debug message is logged.
	 *
	 * @param logInvalidActionMessage If \c true, the message is logged.
	 * If \c false, no message is logged when resolve() / reject() is called
	 * multiple times.
	 */
	void setLogInvalidActionMessage(bool logInvalidActionMessage);

private:
	void logInvalidActionMessage(const char* action) const;
	
	mutable QMutex m_lock;
	State m_state;
	QVariant m_data;
	bool m_logInvalidActionMessage = true;
};

}  // namespace QtPromise

Q_DECLARE_METATYPE(QtPromise::DeferredDestroyed)
Q_DECLARE_METATYPE(QtPromise::Deferred::State)

/*! Returns the hash value for a Deferred smart pointer.
 * @param deferredPtr The QSharedPointer who's hash value should be returned.
 * @param seed The seed used for the calculation.
 * @return The hash value based on the address of the pointer.
 */
uint qHash(const QtPromise::Deferred::Ptr& deferredPtr, uint seed = 0);

#endif /* QTPROMISE_DEFERRED_H_ */
