
#include <QtTest>
#include "Deferred.h"

namespace QtPromise
{
namespace Tests
{

class DeferredTest : public QObject
{
	Q_OBJECT

private slots:
	void constructorTest();
	void resolveTest();
	void rejectTest();
	void notifyTest();
	void destructorTest();

private:
	struct DeferredSpies
	{
		DeferredSpies(Deferred::Ptr deferred);

		QSignalSpy resolved;
		QSignalSpy rejected;
		QSignalSpy notified;
	};

};

DeferredTest::DeferredSpies::DeferredSpies(Deferred::Ptr deferred)
	: resolved(deferred.data(), &Deferred::resolved),
	  rejected(deferred.data(), &Deferred::rejected),
	  notified(deferred.data(), &Deferred::notified)
{
}

void DeferredTest::constructorTest()
{
	Deferred::Ptr deferred = Deferred::create();

	QVERIFY(!deferred.isNull());
	QCOMPARE(deferred->state(), Deferred::Pending);
	QVERIFY(deferred->data().isNull());
}

void DeferredTest::resolveTest()
{
	Deferred::Ptr deferred = Deferred::create();

	DeferredSpies spies(deferred);

	QString value("myValue");

	QVERIFY(deferred->resolve(value));
	QCOMPARE(deferred->data().toString(), value);
	QCOMPARE(deferred->state(), Deferred::Resolved);
	QCOMPARE(spies.resolved.count(), 1);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 0);
	QCOMPARE(spies.resolved.first().first().toString(), value);

	QVERIFY(!deferred->notify(QString("progress")));
	QVERIFY(!deferred->reject(QString("reason")));
	QCOMPARE(deferred->data().toString(), value);
	QCOMPARE(deferred->state(), Deferred::Resolved);
	QCOMPARE(spies.resolved.count(), 1);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 0);
}

void DeferredTest::rejectTest()
{
	Deferred::Ptr deferred = Deferred::create();

	DeferredSpies spies(deferred);

	QString value("myValue");

	QVERIFY(deferred->reject(value));
	QCOMPARE(deferred->data().toString(), value);
	QCOMPARE(deferred->state(), Deferred::Rejected);
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 1);
	QCOMPARE(spies.notified.count(), 0);
	QCOMPARE(spies.rejected.first().first().toString(), value);

	QVERIFY(!deferred->notify(QString("progress")));
	QVERIFY(!deferred->resolve(QString("value")));
	QCOMPARE(deferred->data().toString(), value);
	QCOMPARE(deferred->state(), Deferred::Rejected);
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 1);
	QCOMPARE(spies.notified.count(), 0);
}

void DeferredTest::notifyTest()
{
	Deferred::Ptr deferred = Deferred::create();

	DeferredSpies spies(deferred);

	QString firstValue("myValue");

	QVERIFY(deferred->notify(firstValue));
	QVERIFY(deferred->data().isNull());
	QCOMPARE(deferred->state(), Deferred::Pending);
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 1);
	QCOMPARE(spies.notified.at(0).first().toString(), firstValue);

	int secondValue(3);

	QVERIFY(deferred->notify(secondValue));
	QVERIFY(deferred->data().isNull());
	QCOMPARE(deferred->state(), Deferred::Pending);
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 2);
	QCOMPARE(spies.notified.at(1).first().toInt(), secondValue);
}

void DeferredTest::destructorTest()
{
	Deferred::Ptr deferred = Deferred::create();

	DeferredSpies spies(deferred);

	deferred.clear();

	QVERIFY(spies.rejected.wait()); // Need an event loop to execute deleteLater
	QCOMPARE(spies.rejected.count(), 1);
	QVERIFY(spies.rejected.first().first().canConvert<DeferredDestroyed>());
}


}  // namespace Tests
}  // namespace QtPromise



QTEST_MAIN(QtPromise::Tests::DeferredTest)
#include "DeferredTest.moc"


