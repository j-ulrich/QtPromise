#include "Promise.h"
#include "ChildDeferred.h"

namespace QtPromise {

Promise::Promise(QSharedPointer<Deferred> deferred)
	: QObject(), m_deferred(deferred)
{
	connect(m_deferred.data(), &Deferred::resolved, this, &Promise::resolved);
	connect(m_deferred.data(), &Deferred::rejected, this, &Promise::rejected);
	connect(m_deferred.data(), &Deferred::notified, this, &Promise::notified);
}

Promise::Promise(Deferred::State state, const QVariant& data)
	: Promise(Deferred::create())
{
	switch (state)
	{
	case Deferred::Rejected:
		m_deferred->reject(data);
		break;
	case Deferred::Resolved:
	default:
		m_deferred->resolve(data);
		break;
	}
}

Promise::Ptr Promise::create(Deferred::Ptr deferred)
{
	return Ptr(new Promise(deferred), &QObject::deleteLater);
}

Promise::Ptr Promise::createResolved(const QVariant& value)
{
	return Ptr(new Promise(Deferred::Resolved, value), &QObject::deleteLater);
}

Promise::Ptr Promise::createRejected(const QVariant& reason)
{
	return Ptr(new Promise(Deferred::Rejected, reason), &QObject::deleteLater);
}


Deferred::State Promise::state() const
{
	return m_deferred->state();
}

QVariant Promise::data() const
{
	return m_deferred->data();
}

void Promise::reemitSignals() const
{
	switch (state())
	{
	case Deferred::Resolved:
		emit resolved(data());
		break;
	case Deferred::Rejected:
		emit rejected(data());
		break;
	default:
	case Deferred::Pending:
		break;
	}
}

QSharedPointer<Deferred> Promise::createChildDeferred() const
{
	return ChildDeferred::create(m_deferred).staticCast<Deferred>();
}



}  // namespace QtPromise

