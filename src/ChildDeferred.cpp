#include "ChildDeferred.h"
#include <QTimer>

namespace QtPromise
{

/*!
 * \cond INTERNAL
 */

ChildDeferred::ChildDeferred(const QVector<Deferred::Ptr>& parents, bool trackResults)
	: Deferred(), m_lock(QMutex::Recursive), m_resolvedCount(0), m_rejectedCount(0)
{
	setLogInvalidActionMessage(false);
	setParents(parents, trackResults);
}

ChildDeferred::Ptr ChildDeferred::create(Deferred::Ptr parent, bool trackResults)
{
	return create(QVector<Deferred::Ptr>{parent}, trackResults);
}

ChildDeferred::Ptr ChildDeferred::create(const QVector<Deferred::Ptr>& parents, bool trackResults)
{
	return Ptr(new ChildDeferred(parents, trackResults));
}

ChildDeferred::~ChildDeferred()
{
	/* We disconnect all parent Deferreds to avoid that they trigger
	 * onParentDestroyed() and onParentRejected() when m_parents is released
	 * and they are still pending.
	 * This ChildDeferred itself will be rejected by the Deferred destructor.
	 */
	for (Deferred::Ptr parent : const_cast<const QVector<Deferred::Ptr>&>(m_parents))
		QObject::disconnect(parent.data(), 0, this, 0);
}

void ChildDeferred::setParent(Deferred::Ptr parent, bool trackResults)
{
	setParents(QVector<Deferred::Ptr>{parent}, trackResults);
}

void ChildDeferred::setParents(const QVector<Deferred::Ptr>& parents, bool trackResults)
{
	QMutexLocker locker(&m_lock);

	for (Deferred::Ptr oldParent : const_cast<const QVector<Deferred::Ptr>&>(m_parents))
		QObject::disconnect(oldParent.data(), 0 , this, 0);

	for (Deferred::Ptr parent : parents)
		QObject::connect(parent.data(), &QObject::destroyed, this, &ChildDeferred::onParentDestroyed);

	if (trackResults)
	{
		m_resolvedCount = m_rejectedCount = 0;

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
				QObject::connect(parent.data(), &Deferred::resolved, this, &ChildDeferred::onParentResolved);
				QObject::connect(parent.data(), &Deferred::rejected, this, &ChildDeferred::onParentRejected);
			}
		}
	}

	m_parents = parents;
}

void ChildDeferred::onParentDestroyed(QObject* parent)
{
	QMutexLocker locker(&m_lock);
	qCritical("Parent deferred %s is destroyed while child %s is still holding a reference.", qUtf8Printable(pointerToQString(parent)), qUtf8Printable(pointerToQString(this)));
	QObject::disconnect(parent, 0, this, 0);
	auto deferredParent = static_cast<Deferred*>(parent);
	QVector<int> removeIndices;
	for (int i = m_parents.size()-1; i >= 0; --i)
	{
		if (m_parents.at(i) == deferredParent)
		{
			removeIndices.append(i);
		}
	}

	for (int i : const_cast<const QVector<int>&>(removeIndices))
		m_parents.removeAt(i);
		
}

void ChildDeferred::onParentResolved(const QVariant& value)
{
	QMutexLocker locker(&m_lock);
	m_resolvedCount += 1;
	Q_EMIT parentResolved(value);
	if (m_resolvedCount == m_parents.size())
	{
		// All parents have been resolved
		QList<QVariant> results;
		for (Deferred::Ptr parent : const_cast<const QVector<Deferred::Ptr>&>(m_parents))
			results.append(parent->data());
		Q_EMIT parentsResolved(results);
	}
}

void ChildDeferred::onParentRejected(const QVariant& reason)
{
	QMutexLocker locker(&m_lock);
	m_rejectedCount += 1;
	Q_EMIT parentRejected(reason);
	if (m_rejectedCount == m_parents.size())
	{
		// All parents have been rejected
		QList<QVariant> reasons;
		for (Deferred::Ptr parent : const_cast<const QVector<Deferred::Ptr>&>(m_parents))
			reasons.append(parent->data());
		Q_EMIT parentsRejected(reasons);
	}
}

/*!
 * \endcond
 */

} /* namespace QtPromise */
