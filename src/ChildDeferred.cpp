#include "ChildDeferred.h"

namespace QtPromise
{

ChildDeferred::ChildDeferred(Deferred::Ptr parent)
	: Deferred(), m_parent(parent)
{
	connect(m_parent.data(), &QObject::destroyed, this, &ChildDeferred::onParentDestroyed);
}

ChildDeferred::Ptr ChildDeferred::create(Deferred::Ptr parent)
{
	return Ptr(new ChildDeferred(parent), &QObject::deleteLater);
}

void ChildDeferred::onParentDestroyed() const
{
	qCritical("Parent deferred %08p is destroyed while child %08p is still holding a reference.", m_parent.data(), this);
}



} /* namespace QtPromise */
