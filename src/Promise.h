/*! \file
 *
 * \date Created on: 17.02.2017
 * \author jochen.ulrich
 */

#ifndef QTPROMISE_PROMISE_H_
#define QTPROMISE_PROMISE_H_

#include "Deferred.h"
#include "ChildDeferred.h"

#include <QObject>
#include <QVariant>
#include <QSharedPointer>
#include <QVector>

#include <cstddef>
#include <functional>
#include <type_traits>

namespace QtPromise {


/*! \deprecated Since 2.0.0, you can use \c nullptr instead.
 *
 * "No operation" to be used in combination with Promise::then().
 *
 * This function simply does nothing.
 * It can be used as parameter to Promise::then() to "skip" parameters:
\code
myPromise->then(QtPromise::noop, [](const QVariant& reason) {
  // handle rejection
});
\endcode
 */
inline void noop(const QVariant&) {};


/*! \brief Provides read-only access to the outcome of an asynchronous operation.
 *
 * There are two main usage patterns of Promise objects:
 * - promise chaining using Promise::then()
 * - using the signals
 *
 *	\threadsafeClass
 *	\author jochen.ulrich
 */
class Promise : public QObject
{
	Q_OBJECT

public:
	/*! Smart pointer to a Promise. */
	typedef QSharedPointer<Promise> Ptr;

	/*! Creates a Promise for a given Deferred.
	 *
	 * \param deferred The Deferred whose state is communicated
	 * by the created Promise.
	 * \return QSharedPointer to a new Promise for the given \p deferred.
	 */
	static Ptr create(Deferred::Ptr deferred);
	/*! Creates a resolved Promise.
	 * 
	 * Creates a Deferred, resolves it with the given \p value and returns
	 * a Promise on the Deferred.
	 * 
	 * \param value The value used to resolve the Promise.
	 * \return QSharedPointer to a new, resolved Promise.
	 */
	static Ptr createResolved(const QVariant& value = QVariant());
	/*! Creates a rejected Promise.
	 *
	 * Creates a Deferred, rejects it with the given \p reason and returns
	 * a Promise on the Deferred.
	 * 
	 * \param reason The reason used to reject the Promise.
	 * \return QSharedPointer to a new, rejected Promise.
	 */
	static Ptr createRejected(const QVariant& reason = QVariant());

	/*! Combines multiple Promises using "and" semantics.
	 *
	 * Creates a Promise which is resolved when *all* provided promises
	 * are resolved and rejected when any of the promises is rejected.
	 * When resolved, the value is a QList<QVariant> of the values of promises
	 * in the order of the \p promises.
	 * When rejected, the reason is the rejection reason of the first rejected
	 * promise.
	 *
	 * \tparam PromiseContainer A container type of Promise::Ptr objects.
	 * The container type must be iterable using a range-based \c for loop.
	 * \param promises A \p PromiseContainer of the promises which should
	 * be combined.
	 * \return A QSharedPointer to a new Promise which is resolved when all
	 * \p promises are resolved and rejected when any of the \p promises is rejected.
	 * The returned Promise is *not* notified.
	 */
	template<typename PromiseContainer>
	static Ptr all(PromiseContainer&& promises) { return Promise::all_impl(std::forward<PromiseContainer>(promises)); }
	/*! \overload
	 * Overload for initializer lists.
	 */
	template<typename ListType>
	static Ptr all(const std::initializer_list<ListType>& promises) { return Promise::all_impl(promises); }

	/*! Combines multiple Promises using "or" semantics.
	 *
	 * Creates a Promise which is resolved when *any* provided promise
	 * is resolved and rejected when all of the promises are rejected.
	 * When resolved, the value is the resolve value of the first resolved
	 * promise.
	 * When rejected, the reason is a QList<QVariant> of the rejection reasons
	 * of the promises in the order of the \p promises.
	 *
	 * \tparam PromiseContainer A container type of Promise::Ptr objects.
	 * The container type must be iterable using a range-based \c for loop.
	 * \param promises A \p PromiseContainer of the promises which should
	 * be combined.
	 * \return A QSharedPointer to a new Promise which is resolved when any
	 * promise of the \p promises is resolved and rejected when all the \p promises
	 * are rejected.
	 * The returned Promise is *not* notified.
	 */
	template<typename PromiseContainer>
	static Ptr any(PromiseContainer&& promises) { return Promise::any_impl(std::forward<PromiseContainer>(promises)); }
	/*! \overload
	 * Overload for initializer lists.
	 */
	template<typename ListType>
	static Ptr any(const std::initializer_list<ListType>& promises) { return Promise::any_impl(promises); }

	/*! Default destructor */
	virtual ~Promise() = default;

	/*! \return The current state of the Promise's Deferred.
	 *
	 * \sa Deferred::state()
	 */
	Deferred::State state() const;
	/*! \return The current data of the Promise's Deferred.
	 *
	 * \sa Deferred::data()
	 */
	QVariant data() const;

	/*! Attaches actions to be executed when the Promise is resolved, rejected
	 * or notified.
	 *
	 * This method ensures that the provided callback functions are called when this
	 * Promise is resolved, rejected or notified. This method then returns a new Promise
	 * which is resolved, rejected and notified depending on the types of the callbacks
	 * and their returned values.
	 *
	 * For the \p resolvedCallback and \p rejectedCallback, the following rules apply:
	 * - If the callback returns `void`, the returned Promise is resolved or rejected
	 * identical to this Promise.
	 * - If the callback returns `QVariant`, the returned Promise is **resolved** with
	 * the returned value.
	 * To make this absolute clear: returning a `QVariant` from a \p rejectedCallback
	 * will **resolve** the returned Promise.
	 * - If the callback returns `Promise::Ptr`, the returned Promise is resolved or
	 * rejected identical to the Promise returned by the callback.
	 * Once this Promise has been resolved or rejected, notifying the Promise returned
	 * by the callback will notify the returned Promise.
	 *
	 * For the \p notifiedCallback, the following rules apply:
	 * - If the callback returns `void`, the returned Promise is notified identically to
	 * this Promise.
	 * - If the callback returns `QVariant`, the callback is called when this promise is notified
	 * and the returned Promise is notified with the value returned by the callback.
	 * - If the callback returns `Promise::Ptr`, the returned Promise is notified identical
	 * to the Promise returned by the callback.
	 * Additionally, resolving the Promise returned by the callback or returning an already resolved
	 * Promise will also notify the returned Promise with the resolve data.
	 * Rejecting the Promise or returning a rejected Promise will do nothing which means it filters
	 * the notification.
	 *
	 * Note the special behavior of notifications:\n
	 * Before this Promise is resolved or rejected, the Promise returned by then() will be notified
	 * with the notifications from this Promise, possibly modified by the \p notifiedCallback.
	 * Once this Promise is resolved or rejected, if the corresponding callback (\p resolvedCallback
	 * or \p rejectedCallback) returns a Promise::Ptr, the Promise returned by then() will be
	 * notified with the notifications from that Promise.
	 *
	 * \tparam ResolvedFunc A callback function type expecting a `const QVariant&` as parameter
	 * and returning either `void`, `QVariant` or `Promise::Ptr`. Or std::nullptr_t.
	 * \tparam RejectedFunc A callback function type expecting a `const QVariant&` as parameter
	 * and returning either `void`, `QVariant` or `Promise::Ptr`. Or std::nullptr_t.
	 * \tparam NotifiedFunc A callback function type expecting a `const QVariant&` as parameter
	 * and returning either `void` or `QVariant` or `Promise::Ptr`. Or std::nullptr_t.
	 * \param resolvedCallback A callback which is executed when the Promise's Deferred is resolved.
	 * The callback will receive the data passed to Deferred::resolve() as parameter.
	 * \param rejectedCallback A callback which is executed when the Promise's Deferred is rejected.
	 * The callback will receive the data passed to Deferred::reject() as parameter.
	 * \param notifiedCallback A callback which is executed when the Promise's Deferred is notified.
	 * The callback will receive the data passed to Deferred::notify() as parameter.
	 * \return A new Promise which is resolved/rejected/notified depending on the type and return
	 * value of the \p resolvedCallback/\p rejectedCallback/\p notifiedCallback callback. See above for details.
	 */
	template<typename ResolvedFunc, typename RejectedFunc = std::nullptr_t, typename NotifiedFunc = std::nullptr_t>
	Ptr then(ResolvedFunc&& resolvedCallback, RejectedFunc&& rejectedCallback = nullptr, NotifiedFunc&& notifiedCallback = nullptr ) const;

	/*! Attaches an action to be executed when the Promise is either resolved or rejected.
	 *
	 * `promise->always(func)` equivalent to `promise->then(func, func)`.
	 *
	 * \tparam AlwaysFunc A callback function type expecting a `const QVariant&` as parameter
	 * and returning either `void`, `QVariant` or `Promise::Ptr`.
	 * \param alwaysCallback A callback which is executed when the Promise's Deferred is resolved
	 * or rejected. The callback will receive the data passed to Deferred::resolve() or
	 * Deferred::reject() as parameter.
	 * \return A new Promise which is resolved/rejected depending on the type and return
	 * value of the \p alwaysCallback callback. See above for details.
	 *
	 * \sa then()
	 */
	template <typename AlwaysFunc>
	Ptr always(AlwaysFunc&& alwaysCallback) const { return this->then(std::forward<AlwaysFunc>(alwaysCallback), std::forward<AlwaysFunc>(alwaysCallback)); }


Q_SIGNALS:
	/*! Emitted when the Promise's Deferred is resolved.
	 *
	 * \param value The result of the asynchronous operation.
	 * The actual type of the value depends on the asynchronous operation
	 * and should be documented by the creator of the Promise.
	 *
	 * \sa Deferred::resolved()
	 */
	void resolved(const QVariant& value) const;

	/*! Emitted when the Promise's Deferred is rejected.
	 *
	 * \param reason Indication of the reason why the operation failed.
	 * The actual type of the value depends on the asynchronous operation
	 * and should be documented by the creator of the Promise.
	 *
	 * \sa Deferred::rejected()
	 */
	void rejected(const QVariant& reason) const;
	/*! Emitted when the Promise's Deferred is notified.
	 *
	 * \param progress Information about the progress of the operation.
	 * The actual type of the progress depends on the asynchronous operation
	 * and should be documented by the creator of the Promise.
	 *
	 * \sa Deferred::notified()
	 */
	void notified(const QVariant& progress) const;


protected:

	/*! Defines the type of functions returned by createCallbackWrapper() and createNotifyCallbackWrapper()
	 */
	typedef std::function<void(const QVariant&)> WrappedCallbackFunc;

	/*! Creates a Promise object for a Deferred.
	 *
	 * \param deferred The Deferred which should be represented by the Promise.
	 */
	Promise(Deferred::Ptr deferred);

	/*! Convenience constructor to create a resolved or rejected Promise.
	 *
	 * Creates a Deferred, creates a Promise on that Deferred and
	 * resolves or rejects it with \p data depending on \p state.
	 *
	 * \param state Defines whether the created Promise's Deferred
	 * should resolved or rejected.
	 * \param data The value or rejection reason according to \p state.
	 */
	Promise(Deferred::State state, const QVariant& data);

	/*! The Deferred represented by this Promise.
	 */
	Deferred::Ptr m_deferred;


private:

	template<typename NullCallbackFunc>
	typename std::enable_if<std::is_same<NullCallbackFunc, std::nullptr_t>::value, Promise::Ptr>::type
	callCallback(NullCallbackFunc&&) const;
	template<typename VoidCallbackFunc>
	typename std::enable_if<std::is_same<typename std::result_of<VoidCallbackFunc(const QVariant&)>::type, void>::value, Promise::Ptr>::type
	callCallback(VoidCallbackFunc&& func) const;
	template<typename VariantCallbackFunc>
	typename std::enable_if<std::is_same<typename std::result_of<VariantCallbackFunc(const QVariant&)>::type, QVariant>::value, Promise::Ptr>::type
	callCallback(VariantCallbackFunc&& func) const;
	template<typename PromiseCallbackFunc>
	typename std::enable_if<std::is_same<typename std::result_of<PromiseCallbackFunc(const QVariant&)>::type, Promise::Ptr>::value, Promise::Ptr>::type
	callCallback(PromiseCallbackFunc&& func) const;

	template <typename NullCallbackFunc>
	static
	typename std::enable_if<std::is_same<NullCallbackFunc, std::nullptr_t>::value, Promise::WrappedCallbackFunc>::type
	createCallbackWrapper(ChildDeferred::Ptr newDeferred, NullCallbackFunc func, Deferred::State state);
	template <typename VoidCallbackFunc>
	static
	typename std::enable_if<std::is_same<typename std::result_of<VoidCallbackFunc(const QVariant&)>::type, void>::value, WrappedCallbackFunc>::type
	createCallbackWrapper(ChildDeferred::Ptr newDeferred, VoidCallbackFunc func, Deferred::State state);
	template<typename VariantCallbackFunc>
	static
	typename std::enable_if<std::is_same<typename std::result_of<VariantCallbackFunc(const QVariant&)>::type, QVariant>::value, WrappedCallbackFunc>::type
	createCallbackWrapper(ChildDeferred::Ptr newDeferred, VariantCallbackFunc func, Deferred::State state);
	template<typename PromiseCallbackFunc>
	static
	typename std::enable_if<std::is_same<typename std::result_of<PromiseCallbackFunc(const QVariant&)>::type, Promise::Ptr>::value, WrappedCallbackFunc>::type
	createCallbackWrapper(ChildDeferred::Ptr newDeferred, PromiseCallbackFunc func, Deferred::State state);

	template <typename NullCallbackFunc>
	static
	typename std::enable_if<std::is_same<NullCallbackFunc, std::nullptr_t>::value, Promise::WrappedCallbackFunc>::type
	createNotifyCallbackWrapper(ChildDeferred::Ptr newDeferred, NullCallbackFunc func);
	template <typename VoidCallbackFunc>
	static
	typename std::enable_if<std::is_same<typename std::result_of<VoidCallbackFunc(const QVariant&)>::type, void>::value, WrappedCallbackFunc>::type
	createNotifyCallbackWrapper(ChildDeferred::Ptr newDeferred, VoidCallbackFunc func);
	template<typename VariantCallbackFunc>
	static
	typename std::enable_if<std::is_same<typename std::result_of<VariantCallbackFunc(const QVariant&)>::type, QVariant>::value, WrappedCallbackFunc>::type
	createNotifyCallbackWrapper(ChildDeferred::Ptr newDeferred, VariantCallbackFunc func);
	template<typename PromiseCallbackFunc>
	static
	typename std::enable_if<std::is_same<typename std::result_of<PromiseCallbackFunc(const QVariant&)>::type, Promise::Ptr>::value, WrappedCallbackFunc>::type
	createNotifyCallbackWrapper(ChildDeferred::Ptr newDeferred, PromiseCallbackFunc func);


	template<typename PromiseContainer>
	static Ptr all_impl(const PromiseContainer& promises);
	template<typename PromiseContainer>
	static Ptr any_impl(const PromiseContainer& promises);


};


//####### Template Method Implementation #######
template<typename ResolvedFunc, typename RejectedFunc, typename NotifiedFunc>
Promise::Ptr Promise::then(ResolvedFunc&& resolvedCallback, RejectedFunc&& rejectedCallback, NotifiedFunc&& notifiedCallback) const
{
	switch(this->state())
	{
	case Deferred::Resolved:
		return callCallback(std::forward<ResolvedFunc>(resolvedCallback));
	case Deferred::Rejected:
		return callCallback(std::forward<RejectedFunc>(rejectedCallback));
	case Deferred::Pending:
	default:
		ChildDeferred::Ptr newDeferred = ChildDeferred::create(m_deferred);
		QObject::connect(m_deferred.data(), &Deferred::resolved, createCallbackWrapper(newDeferred, std::forward<ResolvedFunc>(resolvedCallback), Deferred::Resolved));
		QObject::connect(m_deferred.data(), &Deferred::rejected, createCallbackWrapper(newDeferred, std::forward<RejectedFunc>(rejectedCallback), Deferred::Rejected));
		QObject::connect(m_deferred.data(), &Deferred::notified, createNotifyCallbackWrapper(newDeferred, std::forward<NotifiedFunc>(notifiedCallback)));
		return create(static_cast<Deferred::Ptr>(newDeferred));
	}
}

template<typename NullCallbackFunc>
typename std::enable_if<std::is_same<NullCallbackFunc, std::nullptr_t>::value, Promise::Ptr>::type
Promise::callCallback(NullCallbackFunc&&) const
{
	return create(m_deferred);
}

template<typename VoidCallbackFunc>
typename std::enable_if<std::is_same<typename std::result_of<VoidCallbackFunc(const QVariant&)>::type, void>::value, Promise::Ptr>::type
Promise::callCallback(VoidCallbackFunc&& func) const
{
	func(m_deferred->data());
	return create(m_deferred);
}

template<typename VariantCallbackFunc>
typename std::enable_if<std::is_same<typename std::result_of<VariantCallbackFunc(const QVariant&)>::type, QVariant>::value, Promise::Ptr>::type
Promise::callCallback(VariantCallbackFunc&& func) const
{
	QVariant newValue(func(m_deferred->data()));
	Deferred::Ptr newDeferred = Deferred::create();
	/* We always resolve the new deferred since returning a value from a RejectedFunc means
	 * "the problem has been resolved".
	 */
	newDeferred->resolve(newValue);
	return create(newDeferred);
}

template<typename PromiseCallbackFunc>
typename std::enable_if<std::is_same<typename std::result_of<PromiseCallbackFunc(const QVariant&)>::type, Promise::Ptr>::value, Promise::Ptr>::type
Promise::callCallback(PromiseCallbackFunc&& func) const
{
	return func(m_deferred->data());
}


template <typename NullCallbackFunc>
typename std::enable_if<std::is_same<NullCallbackFunc, std::nullptr_t>::value, Promise::WrappedCallbackFunc>::type
Promise::createCallbackWrapper(ChildDeferred::Ptr newDeferred, NullCallbackFunc, Deferred::State state)
{
	Q_ASSERT_X(state != Deferred::Pending, "Promise::createCallbackWrapper()", "state must not be Pending (this is a bug in QtPromise)");
	using namespace std::placeholders;

	if (state == Deferred::Resolved)
		return std::bind(&ChildDeferred::resolve, newDeferred.data(), _1);
	else // state == Deferred::Rejected
		return std::bind(&ChildDeferred::reject, newDeferred.data(), _1);
}

template <typename VoidCallbackFunc>
typename std::enable_if<std::is_same<typename std::result_of<VoidCallbackFunc(const QVariant&)>::type, void>::value, Promise::WrappedCallbackFunc>::type
Promise::createCallbackWrapper(ChildDeferred::Ptr newDeferred, VoidCallbackFunc func, Deferred::State state)
{
	Q_ASSERT_X(state != Deferred::Pending, "Promise::createCallbackWrapper()", "state must not be Pending (this is a bug in QtPromise)");
	if (state == Deferred::Resolved)
		return [newDeferred, func](const QVariant& data) {
		func(data);
		newDeferred->resolve(data);
	};
	else // state == Deferred::Rejected
		return [newDeferred, func](const QVariant& data) {
		func(data);
		newDeferred->reject(data);
	};
}

template<typename VariantCallbackFunc>
typename std::enable_if<std::is_same<typename std::result_of<VariantCallbackFunc(const QVariant&)>::type, QVariant>::value, Promise::WrappedCallbackFunc>::type
Promise::createCallbackWrapper(ChildDeferred::Ptr newDeferred, VariantCallbackFunc func, Deferred::State)
{
	return [newDeferred, func](const QVariant& data) {
		QVariant newValue = QVariant::fromValue(func(data));
		/* We always resolve the new deferred since returning a value from a RejectedFunc means
		 * "the problem has been resolved".
		 */
		newDeferred->resolve(newValue);
	};
}

template<typename PromiseCallbackFunc>
typename std::enable_if<std::is_same<typename std::result_of<PromiseCallbackFunc(const QVariant&)>::type, Promise::Ptr>::value, Promise::WrappedCallbackFunc>::type
Promise::createCallbackWrapper(ChildDeferred::Ptr newDeferred, PromiseCallbackFunc func, Deferred::State)
{
	return [newDeferred, func](const QVariant& data) {
		Deferred::Ptr intermedDeferred = func(data)->m_deferred;
		switch (intermedDeferred->state())
		{
		case Deferred::Resolved:
			newDeferred->resolve(intermedDeferred->data());
			break;
		case Deferred::Rejected:
			newDeferred->reject(intermedDeferred->data());
			break;
		case Deferred::Pending:
		default:
			newDeferred->setParent(intermedDeferred);
			QObject::connect(intermedDeferred.data(), &Deferred::resolved, newDeferred.data(), &Deferred::resolve);
			QObject::connect(intermedDeferred.data(), &Deferred::rejected, newDeferred.data(), &Deferred::reject);
			QObject::connect(intermedDeferred.data(), &Deferred::notified, newDeferred.data(), &Deferred::notify);
			break;
		}
	};
}

template <typename NullCallbackFunc>
typename std::enable_if<std::is_same<NullCallbackFunc, std::nullptr_t>::value, Promise::WrappedCallbackFunc>::type
Promise::createNotifyCallbackWrapper(ChildDeferred::Ptr newDeferred, NullCallbackFunc)
{
	using namespace std::placeholders;
	return std::bind(&ChildDeferred::notify, newDeferred.data(), _1);
}

template <typename VoidCallbackFunc>
typename std::enable_if<std::is_same<typename std::result_of<VoidCallbackFunc(const QVariant&)>::type, void>::value, Promise::WrappedCallbackFunc>::type
Promise::createNotifyCallbackWrapper(ChildDeferred::Ptr newDeferred, VoidCallbackFunc func)
{
	return [newDeferred, func](const QVariant& data) {
		func(data);
		newDeferred->notify(data);
	};
}

template<typename VariantCallbackFunc>
typename std::enable_if<std::is_same<typename std::result_of<VariantCallbackFunc(const QVariant&)>::type, QVariant>::value, Promise::WrappedCallbackFunc>::type
Promise::createNotifyCallbackWrapper(ChildDeferred::Ptr newDeferred, VariantCallbackFunc func)
{
	return [newDeferred, func](const QVariant& data) {
		newDeferred->notify(func(data));
	};
}

template<typename PromiseCallbackFunc>
typename std::enable_if<std::is_same<typename std::result_of<PromiseCallbackFunc(const QVariant&)>::type, Promise::Ptr>::value, Promise::WrappedCallbackFunc>::type
Promise::createNotifyCallbackWrapper(ChildDeferred::Ptr newDeferred, PromiseCallbackFunc func)
{
	// There can be only one parent at this time.
	Deferred::Ptr originalDeferred = newDeferred->parents().first();
	return [originalDeferred, newDeferred, func](const QVariant& data) {
		Deferred::Ptr intermedDeferred = func(data)->m_deferred;
		switch (intermedDeferred->state())
		{
		case Deferred::Pending:
			newDeferred->setParents(QVector<Deferred::Ptr>{originalDeferred, intermedDeferred});
			QObject::connect(intermedDeferred.data(), &Deferred::resolved, newDeferred.data(), &Deferred::notify);
			QObject::connect(intermedDeferred.data(), &Deferred::notified, newDeferred.data(), &Deferred::notify);
			break;
		case Deferred::Resolved:
			newDeferred->notify(intermedDeferred->data());
			break;
		case Deferred::Rejected:
		default:
			break;
		}
	};
}

template<typename PromiseContainer>
Promise::Ptr Promise::all_impl(const PromiseContainer& promises)
{
	QVector<Deferred::Ptr> deferreds;
	for (Promise::Ptr promise : promises)
		deferreds.append(promise->m_deferred);
	ChildDeferred::Ptr combinedDeferred = ChildDeferred::create(deferreds, true);

	QObject::connect(combinedDeferred.data(), &ChildDeferred::parentsResolved, combinedDeferred.data(), &Deferred::resolve);
	QObject::connect(combinedDeferred.data(), &ChildDeferred::parentRejected, combinedDeferred.data(), &Deferred::reject);

	return create(combinedDeferred);
}

template<typename PromiseContainer>
Promise::Ptr Promise::any_impl(const PromiseContainer& promises)
{
	QVector<Deferred::Ptr> deferreds;
	for (Promise::Ptr promise : promises)
		deferreds.append(promise->m_deferred);
	ChildDeferred::Ptr combinedDeferred = ChildDeferred::create(deferreds, true);

	QObject::connect(combinedDeferred.data(), &ChildDeferred::parentResolved, combinedDeferred.data(), &Deferred::resolve);
	QObject::connect(combinedDeferred.data(), &ChildDeferred::parentsRejected, combinedDeferred.data(), &Deferred::reject);

	return create(combinedDeferred);
}


}  // namespace QtPromise

/*! Returns the hash value for a Promise smart pointer.
 * @param promisePtr The QSharedPointer who's hash value should be returned.
 * @param seed The seed used for the calculation.
 * @return The hash value based on the address of the pointer.
 */
uint qHash(const QtPromise::Promise::Ptr& promisePtr, uint seed = 0);

#endif /* QTPROMISE_PROMISE_H_ */
