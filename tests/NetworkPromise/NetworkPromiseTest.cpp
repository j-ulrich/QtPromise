
#include <QtTest>
#include <QtDebug>
#include <QSignalSpy>
#include <QNetworkAccessManager>
#include "NetworkPromise.h"


namespace QtPromise
{
namespace Tests
{

class NetworkPromiseTest : public QObject
{
	Q_OBJECT

private slots:
	void successTest();
	void failTest();
	void reemitSignalsTest_data();
	void reemitSignalsTest();

private:
	struct PromiseSpies
	{
		PromiseSpies(NetworkPromise::Ptr promise);

		QSignalSpy resolved;
		QSignalSpy baseResolved;
		QSignalSpy rejected;
		QSignalSpy baseRejected;
		QSignalSpy notified;
		QSignalSpy baseNotified;
	};
};


//####### Helpers #######
NetworkPromiseTest::PromiseSpies::PromiseSpies(NetworkPromise::Ptr promise)
	: resolved(promise.data(), &NetworkPromise::resolved),
	  rejected(promise.data(), &NetworkPromise::rejected),
	  notified(promise.data(), &NetworkPromise::notified),
	  baseResolved(promise.data(), &Promise::resolved),
	  baseRejected(promise.data(), &Promise::rejected),
	  baseNotified(promise.data(), &Promise::notified)
{
}

//####### Tests #######
void NetworkPromiseTest::successTest()
{
	QString dataPath = QFINDTESTDATA("data/DummyData.txt");
	QFile dataFile(dataPath);
	dataFile.open(QIODevice::ReadOnly);
	QByteArray expectedData = dataFile.readAll();
	dataFile.close();

	QNetworkAccessManager qnam;
	QNetworkRequest request(QUrl::fromLocalFile(dataPath));
	QNetworkReply* reply = qnam.get(request);

	NetworkPromise::Ptr promise = NetworkPromise::create(reply);

	PromiseSpies spies(promise);

	QVERIFY(spies.resolved.wait());
	QCOMPARE(spies.resolved.count(), 1);
	NetworkDeferred::ReplyData replyData = spies.resolved.first().first().value<NetworkDeferred::ReplyData>();
	QCOMPARE(replyData.data, expectedData);
	QCOMPARE(spies.baseResolved.count(), 1);
	QCOMPARE(spies.baseResolved.first().first(), QVariant::fromValue(replyData));
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.baseRejected.count(), 0);
	QVERIFY(spies.notified.count() > 0);
	NetworkDeferred::ReplyProgress progress = spies.notified.first().first().value<NetworkDeferred::ReplyProgress>();
	QVERIFY(progress.download.current > 0);
	QVERIFY(progress.download.total > 0);
	QVERIFY(spies.baseNotified.count() > 0);
	QCOMPARE(spies.baseNotified.first().first(), QVariant::fromValue(progress));
}

void NetworkPromiseTest::failTest()
{
	QString dataPath("A_File_that_doesnt_exist_9831874375377535764532134848337483.txt");

	QNetworkAccessManager qnam;
	QNetworkRequest request(QUrl::fromLocalFile(dataPath));
	QNetworkReply* reply = qnam.get(request);

	NetworkPromise::Ptr promise = NetworkPromise::create(reply);

	PromiseSpies spies(promise);

	QVERIFY(spies.rejected.wait());
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.baseResolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 1);
	NetworkDeferred::Error error = spies.rejected.first().first().value<NetworkDeferred::Error>();
	QCOMPARE(error.error, QNetworkReply::ContentNotFoundError);
	qDebug() << error.errorString;
	QVERIFY(!error.errorString.isEmpty());
	QCOMPARE(spies.baseRejected.count(), 1);
	QCOMPARE(spies.baseRejected.first().first(), QVariant::fromValue(error));
	QCOMPARE(spies.notified.count(), 0);
	QCOMPARE(spies.baseNotified.count(), 0);
}

void NetworkPromiseTest::reemitSignalsTest_data()
{
	QTest::addColumn<QString>("dataPath");

	QTest::newRow("success") << QFINDTESTDATA("data/DummyData.txt");
	QTest::newRow("fail") << "Non_existent_File_45389045765.txt";
}

void NetworkPromiseTest::reemitSignalsTest()
{
	QFETCH(QString, dataPath);

	QNetworkAccessManager qnam;
	QNetworkRequest request(QUrl::fromLocalFile(dataPath));
	QNetworkReply* reply = qnam.get(request);

	NetworkPromise::Ptr promise = NetworkPromise::create(reply);

	PromiseSpies expectedSpies(promise);
	QTest::qWait(1000);

	// Slots connected after promise has been resolved/rejected/notified
	PromiseSpies spies(promise);

	QVERIFY(spies.resolved.isEmpty());
	QVERIFY(spies.baseResolved.isEmpty());
	QVERIFY(spies.rejected.isEmpty());
	QVERIFY(spies.baseRejected.isEmpty());
	QVERIFY(spies.notified.isEmpty());
	QVERIFY(spies.baseNotified.isEmpty());

	promise->reemitSignals();

	QCOMPARE(spies.resolved, expectedSpies.resolved);
	QCOMPARE(spies.baseResolved, expectedSpies.baseResolved);
	QCOMPARE(spies.rejected, expectedSpies.rejected);
	QCOMPARE(spies.baseRejected, expectedSpies.baseRejected);
	// Notified signals are not reemitted.
	QVERIFY(spies.notified.isEmpty());
	QVERIFY(spies.baseNotified.isEmpty());
}


}  // namespace Tests
}  // namespace QtPromise



QTEST_MAIN(QtPromise::Tests::NetworkPromiseTest)
#include "NetworkPromiseTest.moc"


