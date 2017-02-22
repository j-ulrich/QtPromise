#include "Deferred.h"

namespace QtPromise {


Deferred::Ptr Deferred::create()
{
	return Deferred::Ptr(new Deferred(), &QObject::deleteLater);
}


Deferred::~Deferred()
{
	if (m_state == Pending)
	{
		qDebug("Deferred %08p destroyed while still pending", this);
#ifndef QT_NO_EXCEPTIONS
	this->reject(QVariant::fromValue(DeferredDestroyed(this)));
#else
	this->reject(QString("Deferred 0x%1 destroyed").arg(reinterpret_cast<quintptr>(this), QT_POINTER_SIZE * 2, 16, QChar('0')));
#endif // QT_NO_EXCEPTIONS
	}
}

void Deferred::logInvalidActionMessage(const char* action) const
{
	qDebug("Cannot %s Deferred %08p which is already %s", action, this, m_state==Resolved?"resolved":"rejected");
}

bool Deferred::resolve(const QVariant& value)
{
	QWriteLocker locker(&m_lock);

	if (m_state == Pending)
	{
		m_data = value;
		m_state = Resolved;
		emit resolved(m_data);
		return true;
	}
	else
	{
		logInvalidActionMessage("resolve");
		return false;
	}
}

bool Deferred::reject(const QVariant& reason)
{
	QWriteLocker locker(&m_lock);

	if (m_state == Pending)
	{
		m_data = reason;
		m_state = Rejected;
		emit rejected(m_data);
		return true;
	}
	else
	{
		logInvalidActionMessage("reject");
		return false;
	}
}

bool Deferred::notify(const QVariant& progress)
{
	QReadLocker locker(&m_lock);

	if (m_state == Pending)
	{
		emit notified(progress);
		return true;
	}
	else
	{
		logInvalidActionMessage("notify");
		return false;
	}
}

}  // namespace QtPromise
