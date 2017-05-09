#include "PromiseSitter.h"

Q_GLOBAL_STATIC(PromiseSitter, promiseSitterGlobalInstance)

PromiseSitter* PromiseSitter::instance()
{
	return promiseSitterGlobalInstance;
}

void PromiseSitter::add(QSharedPointer<Promise> promise)
{
	QWriteLocker locker(&m_lock);
	if (!m_promises.contains(promise.data()))
	{
		QSharedPointer<Promise> removingPromise = promise->always([this, promise](const QVariant&) {
			this->remove(promise);
		});
		m_promises.insert(promise.data(), removingPromise);
	}
}

bool PromiseSitter::remove(QSharedPointer<Promise> promise)
{
	QWriteLocker locker(&m_lock);
	return m_promises.remove(promise.data()) > 0;
}

bool PromiseSitter::contains(QSharedPointer<Promise> promise) const
{
	QReadLocker locker(&m_lock);
	return m_promises.contains(promise.data());
}
