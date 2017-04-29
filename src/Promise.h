/*! \file
 *
 * \date Created on: 17.02.2017
 * \author jochen.ulrich
 */

#ifndef QTPROMISE_PROMISE_H_
#define QTPROMISE_PROMISE_H_

#include <QObject>
#include <QVariant>
#include <QSharedPointer>
#include <functional>
#include <type_traits>
#include "Deferred.h"
#include "ChildDeferred.h"

namespace QtPromise {


/*! "No operation" to be used in combination with Promise::then().
 *
 * This function simply does nothing.
 * It can be used as parameter to Promise::then() to "skip" parameters:
\code
myPromise.then(QtPromise::noop, [](const QVariant& reason) {
  // handle rejection
});
\endcode
 *
 * \param ignored As this function does nothing, this parameter is ignored as well.
 */
void noop(const QVariant& ignored);


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
	 * The container type must be iterable using a range-based for loop.
	 * \param promises A \p PromiseContainer of the promises which should
	 * be combined.
	 * \return A QSharedPointer to a new Promise which is resolved when all
	 * \p promises are resolved and rejected when any of the \p promises is rejected.
	 * The returned Promise is *not* notified.
	 */
	template<typename PromiseContainer>
	static Ptr all(PromiseContainer promises) { return Promise::all_impl(promises); }
	/*! \overload
	 * Overload for initializer lists.
	 */
	template<typename ListType>
	static Ptr all(std::initializer_list<ListType> promises) { return Promise::all_impl(promises); }

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
	 * The container type must be iterable using a range-based for loop.
	 * \param promises A \p PromiseContainer of the promises which should
	 * be combined.
	 * \return A QSharedPointer to a new Promise which is resolved when any
	 * promise of the \p promises is resolved and rejected when all the \p promises
	 * are rejected.
	 * The returned Promise is *not* notified.
	 */
	template<typename PromiseContainer>
	static Ptr any(PromiseContainer promises) { return Promise::any_impl(promises); }
	/*! \overload
	 * Overload for initializer lists.
	 */
	template<typename ListType>
	static Ptr any(std::initializer_list<ListType> promises) { return Promise::any_impl(promises); }

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

	template<typename ResolvedFunc, typename RejectedFunc = decltype(noop), typename NotifiedFunc = decltype(noop)>
	Ptr then(ResolvedFunc&& resolvedFunc, RejectedFunc&& rejectedFunc = noop, NotifiedFunc&& notifiedFunc = noop ) const;

	template <typename AlwaysFunc>
	Ptr always(AlwaysFunc&& alwaysFunc) const { return this->then(alwaysFunc, alwaysFunc); }

	typedef std::function<void(const QVariant&)> WrappedCallbackFunc;

signals:
	void resolved(const QVariant& value) const;
	void rejected(const QVariant& reason) const;
	void notified(const QVariant& progress) const;

protected:
	Promise(Deferred::Ptr deferred);
	Promise(Deferred::State state, const QVariant& data);

	Deferred::Ptr m_deferred;

private:
	template<typename VoidCallbackFunc>
	typename std::enable_if<std::is_same<typename std::result_of<VoidCallbackFunc(const QVariant&)>::type, void>::value, Promise::Ptr>::type
	callThenFunc(VoidCallbackFunc func) const;
	template<typename VariantCallbackFunc>
	typename std::enable_if<std::is_same<typename std::result_of<VariantCallbackFunc(const QVariant&)>::type, QVariant>::value, Promise::Ptr>::type
	callThenFunc(VariantCallbackFunc func) const;
	template<typename PromiseCallbackFunc>
	typename std::enable_if<std::is_same<typename std::result_of<PromiseCallbackFunc(const QVariant&)>::type, Promise::Ptr>::value, Promise::Ptr>::type
	callThenFunc(PromiseCallbackFunc func) const;

	template <typename VoidCallbackFunc>
	static
	typename std::enable_if<std::is_same<typename std::result_of<VoidCallbackFunc(const QVariant&)>::type, void>::value, WrappedCallbackFunc>::type
	createThenFuncWrapper(Deferred::Ptr newDeferred, VoidCallbackFunc func, Deferred::State state);
	template<typename VariantCallbackFunc>
	static
	typename std::enable_if<std::is_same<typename std::result_of<VariantCallbackFunc(const QVariant&)>::type, QVariant>::value, WrappedCallbackFunc>::type
	createThenFuncWrapper(Deferred::Ptr newDeferred, VariantCallbackFunc func, Deferred::State state);
	template<typename PromiseCallbackFunc>
	static
	typename std::enable_if<std::is_same<typename std::result_of<PromiseCallbackFunc(const QVariant&)>::type, Promise::Ptr>::value, WrappedCallbackFunc>::type
	createThenFuncWrapper(Deferred::Ptr newDeferred, PromiseCallbackFunc func, Deferred::State state);

	template <typename VoidCallbackFunc>
	static
	typename std::enable_if<std::is_same<typename std::result_of<VoidCallbackFunc(const QVariant&)>::type, void>::value, WrappedCallbackFunc>::type
	createNotifyFuncWrapper(Deferred::Ptr newDeferred, VoidCallbackFunc func);
	template<typename VariantCallbackFunc>
	static
	typename std::enable_if<std::is_same<typename std::result_of<VariantCallbackFunc(const QVariant&)>::type, QVariant>::value, WrappedCallbackFunc>::type
	createNotifyFuncWrapper(Deferred::Ptr newDeferred, VariantCallbackFunc func);

	template<typename PromiseContainer>
	static Ptr all_impl(PromiseContainer promises);
	template<typename PromiseContainer>
	static Ptr any_impl(PromiseContainer promises);


};


//####### Template Method Implementation #######
/*!
 *
 * \param resolvedFunc
 * \param rejectedFunc
 * \return
 */
template<typename ResolvedFunc, typename RejectedFunc, typename NotifiedFunc>
Promise::Ptr Promise::then(ResolvedFunc&& resolvedFunc, RejectedFunc&& rejectedFunc, NotifiedFunc&& notifiedFunc) const
{
	switch(this->state())
	{
	case Deferred::Resolved:
		return callThenFunc(resolvedFunc);
	case Deferred::Rejected:
		return callThenFunc(rejectedFunc);
	case Deferred::Pending:
	default:
		Deferred::Ptr newDeferred = ChildDeferred::create(m_deferred).staticCast<Deferred>();
		connect(m_deferred.data(), &Deferred::resolved, createThenFuncWrapper(newDeferred, resolvedFunc, Deferred::Resolved));
		connect(m_deferred.data(), &Deferred::rejected, createThenFuncWrapper(newDeferred, rejectedFunc, Deferred::Rejected));
		connect(m_deferred.data(), &Deferred::notified, createNotifyFuncWrapper(newDeferred, notifiedFunc));
		return create(newDeferred);
	}
}

template<typename VoidCallbackFunc>
typename std::enable_if<std::is_same<typename std::result_of<VoidCallbackFunc(const QVariant&)>::type, void>::value, Promise::Ptr>::type
Promise::callThenFunc(VoidCallbackFunc func) const
{
	func(m_deferred->data());
	return create(m_deferred);
}

template<typename VariantCallbackFunc>
typename std::enable_if<std::is_same<typename std::result_of<VariantCallbackFunc(const QVariant&)>::type, QVariant>::value, Promise::Ptr>::type
Promise::callThenFunc(VariantCallbackFunc func) const
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
Promise::callThenFunc(PromiseCallbackFunc func) const
{
	return func(m_deferred->data());
}


template <typename VoidCallbackFunc>
typename std::enable_if<std::is_same<typename std::result_of<VoidCallbackFunc(const QVariant&)>::type, void>::value, Promise::WrappedCallbackFunc>::type
Promise::createThenFuncWrapper(Deferred::Ptr newDeferred, VoidCallbackFunc func, Deferred::State state)
{
	Q_ASSERT_X(state != Deferred::Pending, "Promise::createThenFuncWrapper()", "state must not be Pending (this is a bug in QtPromise)");
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
Promise::createThenFuncWrapper(Deferred::Ptr newDeferred, VariantCallbackFunc func, Deferred::State state)
{
	Q_UNUSED(state)
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
Promise::createThenFuncWrapper(Deferred::Ptr newDeferred, PromiseCallbackFunc func, Deferred::State state)
{
	Q_UNUSED(state)
	return [newDeferred, func](const QVariant& data) {
		Promise::Ptr intermediatePromise = func(data);
		intermediatePromise->then([newDeferred](const QVariant& data) {newDeferred->resolve(data);},
		                          [newDeferred](const QVariant& data) {newDeferred->reject(data);});
	};
}

template <typename VoidCallbackFunc>
typename std::enable_if<std::is_same<typename std::result_of<VoidCallbackFunc(const QVariant&)>::type, void>::value, Promise::WrappedCallbackFunc>::type
Promise::createNotifyFuncWrapper(Deferred::Ptr newDeferred, VoidCallbackFunc func)
{
	return [newDeferred, func](const QVariant& data) {
		func(data);
	};
}

template<typename VariantCallbackFunc>
typename std::enable_if<std::is_same<typename std::result_of<VariantCallbackFunc(const QVariant&)>::type, QVariant>::value, Promise::WrappedCallbackFunc>::type
Promise::createNotifyFuncWrapper(Deferred::Ptr newDeferred, VariantCallbackFunc func)
{
	return [newDeferred, func](const QVariant& data) {
		newDeferred->notify(func(data));
	};
}

template<typename PromiseContainer>
Promise::Ptr Promise::all_impl(PromiseContainer promises)
{
	QList<Deferred::Ptr> deferreds;
	for (Promise::Ptr promise : promises)
		deferreds.append(promise->m_deferred);
	ChildDeferred::Ptr combinedDeferred = ChildDeferred::create(deferreds, true);

	QObject::connect(combinedDeferred.data(), &ChildDeferred::parentsResolved, combinedDeferred.data(), &Deferred::resolve);
	QObject::connect(combinedDeferred.data(), &ChildDeferred::parentRejected, combinedDeferred.data(), &Deferred::reject);

	return create(combinedDeferred);
}

template<typename PromiseContainer>
Promise::Ptr Promise::any_impl(PromiseContainer promises)
{
	QList<Deferred::Ptr> deferreds;
	for (Promise::Ptr promise : promises)
		deferreds.append(promise->m_deferred);
	ChildDeferred::Ptr combinedDeferred = ChildDeferred::create(deferreds, true);

	QObject::connect(combinedDeferred.data(), &ChildDeferred::parentResolved, combinedDeferred.data(), &Deferred::resolve);
	QObject::connect(combinedDeferred.data(), &ChildDeferred::parentsRejected, combinedDeferred.data(), &Deferred::reject);

	return create(combinedDeferred);
}


}  // namespace QtPromise

/*! Returns the hash value for a Promise smart pointer.
 * @param deferredPtr The QSharedPointer who's hash value should be returned.
 * @param seed The seed used for the calculation.
 * @return The hash value based on the address of the pointer.
 */
uint qHash(const QtPromise::Promise::Ptr& promisePtr, uint seed = 0);

#endif /* QTPROMISE_PROMISE_H_ */
