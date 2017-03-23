/*! \file
 *
 * \date Created on: 21.02.2017
 * \author jochen.ulrich
 */

#ifndef CHILDDEFERRED_H_
#define CHILDDEFERRED_H_

#include "Deferred.h"

namespace QtPromise
{

/*!
 * \cond INTERNAL
 */

/*!
 * \brief A Deferred holding a pointer to another (parent) Deferred.
 *
 * This class is used internally to realize the Promise chaining which is in fact
 * rather a Deferred chain:
 * when Promise::then() is called, a ChildDeferred is created which holds a QSharedPointer
 * to the original Promise's Deferred to prevent its destruction.
 *
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
	 * \return QSharedPointer to a new, pending ChildDeferred.
	 */
	static Ptr create(Deferred::Ptr parent);

protected slots:
	void onParentDestroyed() const;

protected:
	/*! Creates a pending ChildDeferred object holding a pointer to a parent Deferred.
	 *
	 * \param parent The Deferred which should exist as long as this ChildDeferred exists.
	 */
	ChildDeferred(Deferred::Ptr parent);
	Deferred::Ptr m_parent;
};

/*!
 * \endcond
 */

} /* namespace QtPromise */

#endif /* CHILDDEFERRED_H_ */
