#include "NetworkPromise.h"

namespace QtPromise
{

NetworkPromise::NetworkPromise(QNetworkReply* reply)
	: Promise(NetworkDeferred::create(reply))
{
	connect(m_deferred.data(), &NetworkDeferred::resolved, this, &NetworkPromise::resolved);
	connect(m_deferred.data(), &NetworkDeferred::rejected, this, &NetworkPromise::rejected);
	connect(m_deferred.data(), &NetworkDeferred::notified, this, &NetworkPromise::notified);

}

NetworkPromise::Ptr NetworkPromise::create(QNetworkReply* reply)
{
	return Ptr(new NetworkPromise(reply), &QObject::deleteLater);
}

void NetworkPromise::reemitSignals() const
{
	Promise::reemitSignals();
	switch (state())
	{
	case Deferred::Resolved:
		emit resolved(m_deferred->data().toByteArray());
		break;
	case Deferred::Rejected:
		emit rejected(m_deferred->data().value<NetworkDeferred::Error>());
		break;
	default:
	case Deferred::Pending:
		break;
	}
}

} /* namespace QtPromise */
