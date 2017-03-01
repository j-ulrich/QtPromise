# Qt Promise #

> Promise Pattern for Qt

![Build status](https://gitlab.com/julrich/QtPromise/badges/master/build.svg)

## Motivation ##
Working with asynchronous data (e.g. data fetched from web servers) can be cumbersome in Qt applications. Especially if the data needs to be processed before being usable by the application (think extracting specific information out of a JSON object) or if multiple data sources are involved.
Typically, it leads to splitting the processing across multiple signals and slots and having member variables only to remember the state of the processing between the processing steps.

The [promise pattern](https://en.wikipedia.org/wiki/Futures_and_promises) solves these problems by providing an object which represents the state of an asynchronous operation and allowing to chain processing steps to that object.
For more information about the promise pattern, see the links in the [Further Reading](#further-reading) section.

> **Note:** There are different namings for the pattern and it's concepts.
> Some use "future" and "promise" (especially the C++ implementations), others use "deferred" and "promise" (especially the JavaScript implementations), others use just "promise".
> The chaining is also called "pipelining" or "continuation".

### Other Implementations ###
[Boost::Future](http://www.boost.org/doc/libs/1_63_0/doc/html/thread/synchronization.html#thread.synchronization.futures.then) provides an implementation of the pattern.
The C++ Standard Library contains an experimental specification of the pattern: [std::experimental::future::then](http://en.cppreference.com/w/cpp/experimental/future/then).
Both do not integrate with Qt's signal & slot mechanism out of the box but provide support for exceptions.

Qt's [QFuture](http://doc.qt.io/qt-5.6/qfuture.html) and [QFutureWatcher](http://doc.qt.io/qt-5.6/qfuturewatcher.html) provide a similar functionality.
However, they do not allow chaining, enforce using threads and do not support custom asynchronous operations wells (e.g. no progress reporting).

### Further reading ###
- [Wikipedia: Futures and Promises](https://en.wikipedia.org/wiki/Futures_and_promises)
- [Promises/A+](https://promisesaplus.com)
- [JavaScript Promise (MDN)](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Promise)
- [AngularJS: \$q](https://docs.angularjs.org/api/ng/service/$q)
- [jQuery: Deferred Object](https://api.jquery.com/category/deferred-object)
- [Boost::Future](http://www.boost.org/doc/libs/1_63_0/doc/html/thread/synchronization.html#thread.synchronization.futures.then)
- [std::experimental::future::then](http://en.cppreference.com/w/cpp/experimental/future/then)


## Features ##
- Promise chaining
- Error handling and progress reporting
**Note:** QtPromise does **not** support exceptions since Qt's signal & slot mechanism does not support exceptions.
- Can be used with or without threads
- Can be used with any asynchronous operation
- Supports signals & slots
- Supports `QNetworkReply`

> **Important Note:** The library currently does **not** provide binary compatibility between versions. Only source compatibility is guaranteed between minor and patch versions.

## Example ##
```cpp
using QtPromise;
Promise::Ptr fetchJson(QNetworkReply* reply)
{
	return QtPromise::NetworkPromise::create(reply)
	->then([](const QVariant& data) -> QVariant {
		/* We could do more pre-processing here like
		 * removing comments from the JSON etc.
		 */
		return QJsonDocument::fromJson(data.toByteArray());
	});
}

// ...

QNetworkAccessManager qnam;
QNetworkRequest request("http://www.example.com/getData");
QNetworkReply* reply = qnam.get(request);

this->promise = fetchJson(reply)
->then([](const QVariant& data) {
	// Do something with JSON document
	this->print(data.toJsonDocument().toObject());
}, [](const QVariant& error) {
	this->logError("Error fetching JSON document: "+error.value<NetworkDeferred::Error>().message());
});
```

## Requirements ##
 - Qt 5
 - Compiler supporting C++11 (tested with Microsoft Visual Studio 2015 and GCC 5.x and 6.x)

## Documentation ##
- [Changelog](CHANGELOG.md)

## License ##
QtPromise is licensed under [MIT license](LICENSE).