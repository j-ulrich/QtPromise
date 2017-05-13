#include "PromiseSitter.h"

namespace QtPromise {

Q_GLOBAL_STATIC(PromiseSitter, promiseSitterGlobalInstance)

PromiseSitter* PromiseSitter::instance()
{
	return promiseSitterGlobalInstance;
}

void PromiseSitter::add(QSharedPointer<Promise> promise)
{
	if (promise->state() == Deferred::Pending)
	{
		QWriteLocker locker(&m_lock);
		Promise* rawPromise = promise.data();
		if (!m_promises.contains(rawPromise))
		{
			connect(rawPromise, &Promise::resolved, [this, rawPromise](const QVariant&) {
				this->remove(rawPromise);
			});
			connect(rawPromise, &Promise::rejected, [this, rawPromise](const QVariant&) {
				this->remove(rawPromise);
			});
			m_promises.insert(rawPromise, promise);
		}
	}
}

bool PromiseSitter::remove(const Promise* promise)
{
	QWriteLocker locker(&m_lock);
	return m_promises.remove(promise) > 0;
}

bool PromiseSitter::contains(QSharedPointer<Promise> promise) const
{
	QReadLocker locker(&m_lock);
	return m_promises.contains(promise.data());
}

} // namespace QtPromise
