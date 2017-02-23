
#include <QtTest>
#include "Promise.h"

namespace QtPromise
{
namespace Tests
{

class PromiseTest : public QObject
{
	Q_OBJECT

private slots:
	void createResolvedPromiseTest();
	void createRejectedPromiseTest();

};


void PromiseTest::createResolvedPromiseTest()
{
	QString myString = "String";
	Promise::Ptr promise = Promise::createResolved(myString);

	QCOMPARE(promise->state(), Deferred::Resolved);
}

void PromiseTest::createRejectedPromiseTest()
{
	QString myString = "String";
	Promise::Ptr promise = Promise::createRejected(myString);

	QCOMPARE(promise->state(), Deferred::Rejected);
}

}  // namespace Tests
}  // namespace QtPromise


QTEST_MAIN(QtPromise::Tests::PromiseTest)
#include "PromiseTest.moc"


