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

/*! \brief Provides access to asynchronous data.
 *
 *	\threadsafeClass
 */
class Promise : public QObject
{
	Q_OBJECT

public:
	typedef QSharedPointer<Promise> Ptr;

	static Ptr create(Deferred::Ptr deferred);
	static Ptr createResolved(const QVariant& value);
	static Ptr createRejected(const QVariant& reason);

	virtual ~Promise() = default;

	Deferred::State state() const;
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

	Deferred::Ptr createChildDeferred() const;

	template <typename VoidCallbackFunc>
	typename std::enable_if<std::is_same<typename std::result_of<VoidCallbackFunc(const QVariant&)>::type, void>::value, WrappedCallbackFunc>::type
	createThenFuncWrapper(Deferred::Ptr newDeferred, VoidCallbackFunc func, Deferred::State state) const;
	template<typename VariantCallbackFunc>
	typename std::enable_if<std::is_same<typename std::result_of<VariantCallbackFunc(const QVariant&)>::type, QVariant>::value, WrappedCallbackFunc>::type
	createThenFuncWrapper(Deferred::Ptr newDeferred, VariantCallbackFunc func, Deferred::State state) const;
	template<typename PromiseCallbackFunc>
	typename std::enable_if<std::is_same<typename std::result_of<PromiseCallbackFunc(const QVariant&)>::type, Promise::Ptr>::value, WrappedCallbackFunc>::type
	createThenFuncWrapper(Deferred::Ptr newDeferred, PromiseCallbackFunc func, Deferred::State state) const;

	template <typename VoidCallbackFunc>
	typename std::enable_if<std::is_same<typename std::result_of<VoidCallbackFunc(const QVariant&)>::type, void>::value, WrappedCallbackFunc>::type
	createNotifyFuncWrapper(Deferred::Ptr newDeferred, VoidCallbackFunc func) const;
	template<typename VariantCallbackFunc>
	typename std::enable_if<std::is_same<typename std::result_of<VariantCallbackFunc(const QVariant&)>::type, QVariant>::value, WrappedCallbackFunc>::type
	createNotifyFuncWrapper(Deferred::Ptr newDeferred, VariantCallbackFunc func) const;

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
		Deferred::Ptr newDeferred = createChildDeferred();
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
Promise::createThenFuncWrapper(Deferred::Ptr newDeferred, VoidCallbackFunc func, Deferred::State state) const
{
	// We may not access m_deferred here to avoid deadlocks!
	return [state, newDeferred, func](const QVariant& data) {
		func(data);
		if (state == Deferred::Resolved)
			newDeferred->resolve(data);
		else if (state == Deferred::Rejected)
			newDeferred->reject(data);
	};
}

template<typename VariantCallbackFunc>
typename std::enable_if<std::is_same<typename std::result_of<VariantCallbackFunc(const QVariant&)>::type, QVariant>::value, Promise::WrappedCallbackFunc>::type
Promise::createThenFuncWrapper(Deferred::Ptr newDeferred, VariantCallbackFunc func, Deferred::State state) const
{
	// We may not access m_deferred here to avoid deadlocks!
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
Promise::createThenFuncWrapper(Deferred::Ptr newDeferred, PromiseCallbackFunc func, Deferred::State state) const
{
	// We may not access m_deferred here to avoid deadlocks!
	Q_UNUSED(state)
	return [newDeferred, func](const QVariant& data) {
		Promise::Ptr intermediatePromise = func(data);
		intermediatePromise->then([newDeferred](const QVariant& data) {newDeferred->resolve(data);},
				[newDeferred](const QVariant& data) {newDeferred->reject(data);});
	};
}

template <typename VoidCallbackFunc>
typename std::enable_if<std::is_same<typename std::result_of<VoidCallbackFunc(const QVariant&)>::type, void>::value, Promise::WrappedCallbackFunc>::type
Promise::createNotifyFuncWrapper(Deferred::Ptr newDeferred, VoidCallbackFunc func) const
{
	// We may not access m_deferred here to avoid deadlocks!
	return [newDeferred, func](const QVariant& data) {
		func(data);
	};
}

template<typename VariantCallbackFunc>
typename std::enable_if<std::is_same<typename std::result_of<VariantCallbackFunc(const QVariant&)>::type, QVariant>::value, Promise::WrappedCallbackFunc>::type
Promise::createNotifyFuncWrapper(Deferred::Ptr newDeferred, VariantCallbackFunc func) const
{
	// We may not access m_deferred here to avoid deadlocks!
	return [newDeferred, func](const QVariant& data) {
		newDeferred->notify(func(data));
	};
}




}  // namespace QtPromise

#endif /* QTPROMISE_PROMISE_H_ */
