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
			/* Need to use QueuedConnection since we may not delete the promise
			 * in a slot connected to its signal.
			 */
			connect(rawPromise, &Promise::resolved, this, [this, rawPromise](const QVariant&) {
				this->remove(rawPromise);
			}, Qt::QueuedConnection);
			connect(rawPromise, &Promise::rejected, this, [this, rawPromise](const QVariant&) {
				this->remove(rawPromise);
			}, Qt::QueuedConnection);
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
