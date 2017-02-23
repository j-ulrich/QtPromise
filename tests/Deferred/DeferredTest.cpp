
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
	QVERIFY(resolved.isValid());
	QVERIFY(rejected.isValid());
	QVERIFY(notified.isValid());
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
}

void DeferredTest::rejectTest()
{

}

void DeferredTest::notifyTest()
{

}


}  // namespace Tests
}  // namespace QtPromise



QTEST_MAIN(QtPromise::Tests::DeferredTest)
#include "DeferredTest.moc"


