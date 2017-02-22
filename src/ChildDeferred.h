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

class ChildDeferred : public Deferred
{
	Q_OBJECT

public:

	typedef QSharedPointer<ChildDeferred> Ptr;

	virtual ~ChildDeferred();

	static Ptr create(Deferred::Ptr parent);

protected slots:
	void onParentDestroyed() const;

protected:
	ChildDeferred(Deferred::Ptr parent);
	Deferred::Ptr m_parent;
};

} /* namespace QtPromise */

#endif /* CHILDDEFERRED_H_ */
