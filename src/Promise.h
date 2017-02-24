/*! \file
 *
 * \date Created on: 17.02.2017
 * \author jochen.ulrich
 */

#ifndef SRC_PROMISE_H_
#define SRC_PROMISE_H_

#include <QObject>
#include <QVariant>
#include <QSharedPointer>
#include <functional>
#include <type_traits>
#include "Deferred.h"

namespace QtPromise {


/*! \brief Provides access to asynchronous data.
 *
 */
class Promise : public QObject
{
	Q_OBJECT

public:
	typedef QSharedPointer<Promise> Ptr;

	static Ptr create(Deferred::Ptr deferred);
	static Ptr createResolved(const QVariant& value);
	static Ptr createRejected(const QVariant& reason);

	Deferred::State state() const;
	QVariant data() const;
	virtual void reemitSignals() const;

	template<typename ResolvedFunc, typename RejectedFunc>
	Ptr then(ResolvedFunc&& resolvedFunc, RejectedFunc&& rejectedFunc) const;

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
	typename std::enable_if<std::is_same<decltype(VoidCallbackFunc(QVariant())), void>::value, Promise::Ptr>::type
	callThenFunc(VoidCallbackFunc func) const;
	template<typename VariantCallbackFunc>
	typename std::enable_if<std::is_same<decltype(VariantCallbackFunc(QVariant())), QVariant>::value, Promise::Ptr>::type
	callThenFunc(VariantCallbackFunc func) const;
	template<typename PromiseCallbackFunc>
	typename std::enable_if<std::is_same<decltype(PromiseCallbackFunc(QVariant())), Promise::Ptr>::value, Promise::Ptr>::type
	callThenFunc(PromiseCallbackFunc func) const;

	Deferred::Ptr createChildDeferred() const;

	template <typename VoidCallbackFunc>
	typename std::enable_if<std::is_same<decltype(VoidCallbackFunc(QVariant())), void>::value, WrappedCallbackFunc>::type
	createThenFuncWrapper(Deferred::Ptr newDeferred, VoidCallbackFunc func) const;
	template<typename VariantCallbackFunc>
	typename std::enable_if<std::is_same<decltype(VariantCallbackFunc(QVariant())), QVariant>::value, WrappedCallbackFunc>::type
	createThenFuncWrapper(Deferred::Ptr newDeferred, VariantCallbackFunc func) const;
	template<typename PromiseCallbackFunc>
	typename std::enable_if<std::is_same<decltype(PromiseCallbackFunc(QVariant())), Promise::Ptr>::value, WrappedCallbackFunc>::type
	createThenFuncWrapper(Deferred::Ptr newDeferred, PromiseCallbackFunc func) const;

};


//####### Template Method Implementation #######
/*!
 *
 * \param resolvedFunc
 * \param rejectedFunc
 * \return
 */
template<typename ResolvedFunc, typename RejectedFunc>
Promise::Ptr Promise::then(ResolvedFunc&& resolvedFunc, RejectedFunc&& rejectedFunc) const
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
		connect(this, &Promise::resolved, createThenFuncWrapper(newDeferred, resolvedFunc));
		connect(this, &Promise::resolved, createThenFuncWrapper(newDeferred, rejectedFunc));
		return new Promise(newDeferred);
	}
}

template<typename VoidCallbackFunc>
typename std::enable_if<std::is_same<decltype(VoidCallbackFunc(QVariant())), void>::value, Promise::Ptr>::type
Promise::callThenFunc(VoidCallbackFunc func) const
{
	func(m_deferred->data());
	return Ptr(new Promise(m_deferred));
}

template<typename VariantCallbackFunc>
typename std::enable_if<std::is_same<decltype(VariantCallbackFunc(QVariant())), QVariant>::value, Promise::Ptr>::type
Promise::callThenFunc(VariantCallbackFunc func) const
{
	QVariant newValue(func(m_deferred->data()));
	Deferred::Ptr newDeferred = Deferred::create();
	/* We always resolve the new deferred since returning a value from a RejectedFunc means
	 * "the problem has been resolved".
	 */
	newDeferred->resolve(newValue);
	return Ptr(new Promise(newDeferred));
}

template<typename PromiseCallbackFunc>
typename std::enable_if<std::is_same<decltype(PromiseCallbackFunc(QVariant())), Promise::Ptr>::value, Promise::Ptr>::type
Promise::callThenFunc(PromiseCallbackFunc func) const
{
	return func(m_deferred->data());
}


template <typename VoidCallbackFunc>
typename std::enable_if<std::is_same<decltype(VoidCallbackFunc(QVariant())), void>::value, Promise::WrappedCallbackFunc>::type
Promise::createThenFuncWrapper(Deferred::Ptr newDeferred,VoidCallbackFunc func) const
{
	return [this, newDeferred, func](const QVariant& data) {
		func(data);
		if (this->state() == Deferred::Resolved)
			newDeferred->resolve(data);
		else if (this->state() == Deferred::Rejected)
			newDeferred->reject(data);
	};
}

template<typename VariantCallbackFunc>
typename std::enable_if<std::is_same<decltype(VariantCallbackFunc(QVariant())), QVariant>::value, Promise::WrappedCallbackFunc>::type
Promise::createThenFuncWrapper(Deferred::Ptr newDeferred, VariantCallbackFunc func) const
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
typename std::enable_if<std::is_same<decltype(PromiseCallbackFunc(QVariant())), Promise::Ptr>::value, Promise::WrappedCallbackFunc>::type
Promise::createThenFuncWrapper(Deferred::Ptr newDeferred, PromiseCallbackFunc func) const
{
	return [newDeferred, func](const QVariant& data) {
		Promise::Ptr intermediatePromise = func(data);
		intermediatePromise->then([newDeferred](const QVariant& data) {newDeferred->resolve(data);},
				[newDeferred](const QVariant& data) {newDeferred->reject(data);});
	};
}




}  // namespace QtPromise

#endif /* SRC_PROMISE_H_ */
