#include "Deferred.h"

#include <QHash>

namespace QtPromise {

QAtomicInteger<qint8> Deferred::m_metaTypesRegistered{0};

Deferred::Deferred()
	: QObject(nullptr), m_state(Pending), m_lock(QMutex::Recursive)
{
	registerMetaTypes();
}

void Deferred::registerMetaTypes()
{
	if (m_metaTypesRegistered.testAndSetOrdered(0, 1))
	{
		qRegisterMetaType<DeferredDestroyed>();
		qRegisterMetaType<DeferredDestroyed>("QtPromise::DeferredDestroyed");
		qRegisterMetaType<State>();
		QMetaType::registerEqualsComparator<State>();
		qRegisterMetaType<State>("Deferred::State");
		qRegisterMetaType<State>("QtPromise::Deferred::State");
	}
}

Deferred::Ptr Deferred::create()
{
	return Deferred::Ptr(new Deferred(), &QObject::deleteLater);
}

Deferred::Ptr Deferred::create(State state, const QVariant& data)
{
	Deferred::Ptr deferred = create();
	switch (state)
	{
	case Rejected:
		deferred->reject(data);
		break;
	case Resolved:
	default:
		deferred->resolve(data);
		break;
	}
	return deferred;
}


Deferred::~Deferred()
{
	if (m_state == Pending)
	{
		qDebug("Deferred %s destroyed while still pending", qUtf8Printable(pointerToQString(this)));
#ifndef QT_NO_EXCEPTIONS
	this->reject(QVariant::fromValue(DeferredDestroyed(this)));
#else
	this->reject(QString("Deferred 0x%1 destroyed").arg(reinterpret_cast<quintptr>(this), QT_POINTER_SIZE * 2, 16, QChar('0')));
#endif // QT_NO_EXCEPTIONS
	}
}

void Deferred::setLogInvalidActionMessage(bool logInvalidActionMessage)
{
	m_logInvalidActionMessage = logInvalidActionMessage;
}

void Deferred::logInvalidActionMessage(const char* action) const
{
	if (m_logInvalidActionMessage)
		qDebug("Cannot %s Deferred %s which is already %s", action, qUtf8Printable(pointerToQString(this)), m_state==Resolved?"resolved":"rejected");
}

bool Deferred::resolve(const QVariant& value)
{
	QMutexLocker locker(&m_lock);

	if (m_state == Pending)
	{
		m_data = value;
		m_state = Resolved;
		Q_EMIT resolved(m_data);
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
	QMutexLocker locker(&m_lock);

	if (m_state == Pending)
	{
		m_data = reason;
		m_state = Rejected;
		Q_EMIT rejected(m_data);
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
	QMutexLocker locker(&m_lock);

	if (m_state == Pending)
	{
		Q_EMIT notified(progress);
		return true;
	}
	else
	{
		logInvalidActionMessage("notify");
		return false;
	}
}

QString pointerToQString(const void* pointer)
{
	return QString("0x%1").arg(reinterpret_cast<quintptr>(pointer),
	                           QT_POINTER_SIZE * 2, 16, QChar('0'));
}

}  // namespace QtPromise


uint qHash(const QtPromise::Deferred::Ptr& deferredPtr, uint seed)
{
	return qHash(deferredPtr.data(), seed);
}
