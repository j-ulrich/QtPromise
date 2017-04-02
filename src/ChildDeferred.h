/*! \file
 *
 * \date Created on: 21.02.2017
 * \author jochen.ulrich
 */

#ifndef QTPROMISE_CHILDDEFERRED_H_
#define QTPROMISE_CHILDDEFERRED_H_

#include "Deferred.h"
#include <QList>

namespace QtPromise
{

class ChildDeferred : public Deferred
{
	Q_OBJECT

public:

	typedef QSharedPointer<ChildDeferred> Ptr;

	static Ptr create(Deferred::Ptr parent, bool trackResults = false);
	static Ptr create(QList<Deferred::Ptr> parents, bool trackResults = false);


signals:
	/*! Emitted when one of the parent Deferreds is resolved.
	 *
	 * @param value The result of the resolved Deferred.
	 *
	 * \sa Deferred::resolved()
	 */
	void parentResolved(const QVariant& value) const;
	/*! Emitted when all parent Deferreds are resolved.
	 *
	 * @param results List of the results of the Deferreds in
	 * the order of the Deferreds as provided to create().
	 */
	void parentsResolved(QList<QVariant> results) const;
	/*! Emitted when one of the parent Deferreds is rejected.
	 *
	 * @param reason The reason why the Deferred was rejected.
	 *
	 * \sa Deferred::rejected()
	 */
	void parentRejected(const QVariant& reason) const;
	/*! Emitted when all parent Deferreds are rejected.
	 *
	 * @param reasons List of the reasons why the Deferreds were
	 * rejected in the order of the Deferreds as provided to create().
	 */
	void parentsRejected(QList<QVariant> reasons) const;

protected:
	ChildDeferred(QList<Deferred::Ptr> parents, bool trackResults);

private slots:
	void onParentDestroyed(QObject* parent) const;
	void onParentResolved(const QVariant& value);
	void onParentRejected(const QVariant& reason);

private:
	mutable QMutex m_lock;
	QList<Deferred::Ptr> m_parents;
	int m_resolvedCount;
	int m_rejectedCount;
};

} /* namespace QtPromise */

#endif /* QTPROMISE_CHILDDEFERRED_H_ */
