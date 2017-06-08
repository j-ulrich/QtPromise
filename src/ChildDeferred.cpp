#include "ChildDeferred.h"
#include <QTimer>

namespace QtPromise
{

ChildDeferred::ChildDeferred(QList<Deferred::Ptr> parents, bool trackResults)
	: Deferred(), m_lock(QMutex::Recursive), m_resolvedCount(0), m_rejectedCount(0)
{
	setLogInvalidActionMessage(false);
	setParents(parents, trackResults);
}

ChildDeferred::Ptr ChildDeferred::create(Deferred::Ptr parent, bool trackResults)
{
	return create(QList<Deferred::Ptr>({parent}), trackResults);
}

ChildDeferred::Ptr ChildDeferred::create(QList<Deferred::Ptr> parents, bool trackResults)
{
	return Ptr(new ChildDeferred(parents, trackResults), &QObject::deleteLater);
}

void ChildDeferred::setParent(Deferred::Ptr parent, bool trackResults)
{
	setParents(QList<Deferred::Ptr>({parent}), trackResults);
}

void ChildDeferred::setParents(QList<Deferred::Ptr> parents, bool trackResults)
{
	for (Deferred::Ptr oldParent : m_parents)
		disconnect(oldParent.data(), 0 , this, 0);

	for (Deferred::Ptr parent : parents)
		connect(parent.data(), &QObject::destroyed, this, &ChildDeferred::onParentDestroyed);

	if (trackResults)
	{
		for (Deferred::Ptr parent : parents)
		{
			switch(parent->state())
			{
			case Resolved:
				QTimer::singleShot(0, this, [this, parent]() {
					this->onParentResolved(parent->data());
				});
				break;
			case Rejected:
				QTimer::singleShot(0, this, [this, parent]() {
					this->onParentRejected(parent->data());
				});
				break;
			case Pending:
			default:
				connect(parent.data(), &Deferred::resolved, this, &ChildDeferred::onParentResolved);
				connect(parent.data(), &Deferred::rejected, this, &ChildDeferred::onParentRejected);
			}
		}
	}

	m_parents = parents;
}

void ChildDeferred::onParentDestroyed(QObject* parent) const
{
	qCritical("Parent deferred %08p is destroyed while child %08p is still holding a reference.", parent, this);
}

void ChildDeferred::onParentResolved(const QVariant& value)
{
	QMutexLocker locker(&m_lock);
	m_resolvedCount += 1;
	emit parentResolved(value);
	if (m_resolvedCount == m_parents.size())
	{
		QList<QVariant> results;
		for (Deferred::Ptr parent : m_parents)
			results.append(parent->data());
		emit parentsResolved(results);
	}
}

void ChildDeferred::onParentRejected(const QVariant& reason)
{
	QMutexLocker locker(&m_lock);
	m_rejectedCount += 1;
	emit parentRejected(reason);
	if (m_rejectedCount == m_parents.size())
	{
		QList<QVariant> reasons;
		for (Deferred::Ptr parent : m_parents)
			reasons.append(parent->data());
		emit parentsRejected(reasons);
	}
}


} /* namespace QtPromise */
