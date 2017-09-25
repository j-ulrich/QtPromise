/*! \file
 *
 * \date Created on: 21.02.2017
 * \author jochen.ulrich
 */

#ifndef QTPROMISE_CHILDDEFERRED_H_
#define QTPROMISE_CHILDDEFERRED_H_

#include "Deferred.h"
#include <QList>
#include <QVector>

namespace QtPromise
{

/*!
 * \cond INTERNAL
 */

/*!
 * \brief A Deferred holding pointers to other (parent) Deferreds.
 *
 * This class is used internally to realize the Promise chaining which is in fact
 * rather a Deferred chain:
 * when Promise::then(), Promise::all() or Promise::any() is called, a ChildDeferred
 * is created which holds QSharedPointers to the original Promises' Deferreds to prevent
 * their destruction.
 *
 * \threadsafeClass
 * \author jochen.ulrich
 */
class ChildDeferred : public Deferred
{
	Q_OBJECT

public:

	/*! Smart pointer to ChildDeferred. */
	typedef QSharedPointer<ChildDeferred> Ptr;

	/*! Creates a ChildDeferred which holds a pointer to a parent Deferred.
	 *
	 * \param parent The Deferred which acts as parent to this ChildDeferred.
	 * \param trackResults If \c true, the ChildDeferred listens to the signals
	 * of the \p parent and emits parentResolved(), parentRejected(), parentsResolved()
	 * and parentsResolved() signals.
	 * \return QSharedPointer to a new, pending ChildDeferred.
	 */
	static Ptr create(Deferred::Ptr parent, bool trackResults = false);
	/*! Creates a ChildDeferred which holds pointers to multiple parent Deferreds.
	 *
	 * \param parents List of parent Deferreds.
	 * \param trackResults If \c true, the ChildDeferred listens to the signals
	 * of the \p parents and emits parentResolved(), parentRejected(), parentsResolved()
	 * and parentsResolved() signals.
	 * \return QSharedPointer to a new, pending ChildDeferred.
	 */
	static Ptr create(const QVector<Deferred::Ptr>& parents, bool trackResults = false);

	/*! Sets the parent of this ChildDeferred.
	 *
	 * \param parent The new parent for this ChildDeferred.
	 * \param trackResults If \c true, the ChildDeferred listens to the signals
	 * of the \p parent and emits parentResolved(), parentRejected(), parentsResolved()
	 * and parentsResolved() signals.
	 */
	void setParent(Deferred::Ptr parent, bool trackResults = false);
	/*! Sets the parents of this ChildDeferred.
	 *
	 * \param parents The new parents of this ChildDeferred.
	 * \param trackResults If \c true, the ChildDeferred listens to the signals
	 * of the \p parents and emits parentResolved(), parentRejected(), parentsResolved()
	 * and parentsResolved() signals.
	 */
	void setParents(const QVector<Deferred::Ptr>& parents, bool trackResults = false);

	/*! \return The parents of this ChildDeferred.
	 */
	QVector<Deferred::Ptr> parents() const { return m_parents; }

Q_SIGNALS:
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
	/*! Creates a pending ChildDeferred object holding a pointer to a parent Deferred.
	 *
	 * \param parent The Deferred which should exist as long as this ChildDeferred exists.
	 */
	ChildDeferred(const QVector<Deferred::Ptr>& parents, bool trackResults);

private Q_SLOTS:
	void onParentDestroyed(QObject* parent) const;
	void onParentResolved(const QVariant& value);
	void onParentRejected(const QVariant& reason);

private:
	mutable QMutex m_lock;
	QVector<Deferred::Ptr> m_parents;
	int m_resolvedCount;
	int m_rejectedCount;
};

/*!
 * \endcond
 */

} /* namespace QtPromise */

#endif /* QTPROMISE_CHILDDEFERRED_H_ */
