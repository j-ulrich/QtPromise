#include "FutureDeferred.h"

#ifndef QT_NO_QFUTURE

namespace QtPromise
{

void FutureDeferred::registerMetaTypes()
{
	qRegisterMetaType<Progress>();
	QMetaType::registerEqualsComparator<Progress>();
	qRegisterMetaType<Progress>("FutureDeferred::Progress");
	qRegisterMetaType<Progress>("QtPromise::FutureDeferred::Progress");
}

void FutureDeferred::futureFinished(const QVariantList& results)
{
	QMutexLocker locker(&m_lock);
	m_results = results;
	if (this->resolve(QVariant::fromValue(results)))
		Q_EMIT resolved(results);
}

void FutureDeferred::futureCanceled(const QVariantList& results)
{
	QMutexLocker locker(&m_lock);
	m_results = results;
	if (this->reject(QVariant::fromValue(results)))
		Q_EMIT rejected(results);
}

void FutureDeferred::futureProgressRangeChanged(int min, int max)
{
	QMutexLocker locker(&m_lock);
	m_progress.min = min;
	m_progress.max = max;
	if (this->notify(QVariant::fromValue(m_progress)))
		Q_EMIT notified(m_progress);
}

void FutureDeferred::futureProgressTextChanged(const QString& text)
{
	QMutexLocker locker(&m_lock);
	m_progress.text = text;
	if (this->notify(QVariant::fromValue(m_progress)))
		Q_EMIT notified(m_progress);
}

void FutureDeferred::futureProgressValueChanged(int value)
{
	QMutexLocker locker(&m_lock);
	m_progress.value = value;
	if (this->notify(QVariant::fromValue(m_progress)))
		Q_EMIT notified(m_progress);
}


} /* namespace QtPromise */

#endif /* QT_NO_QTFUTURE */
