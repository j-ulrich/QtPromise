# Changelog # {#changelog}

\brief The changelog of the QtPromise library.

This project adheres to [Semantic Versioning](http://semver.org/).

This changelog follows the [Keep a Changelog](http://keepachangelog.com) format.


---


## Unreleased ##


### Breaking Changes ###
- [!30] Improves passing of parameters.
This is a breaking change because the signature of Promise::all() and Promise::any() changes.
However, as long as you do not rely on the exact signature, the break will not affect you since it
is just changing call-by-value to call-by-reference.
- [!31] Switched from using `QObject::deleteLater()` to using "immediate" delete.
This has proven necessary because with a deferred delete, a callback can still be invoked between
the destruction of one of its captured dependencies and its scheduled deletion.


---


## [1.2.0] - 2017-09-17 ##
Feature addition: PromiseSitter context objects

### Added ###
- [#9] Support for context objects in PromiseSitter
- Guide for handling object lifetime in capturing lambda expressions

### Changed ###
- Fixed unnecessary container copying/detaching


---


## [1.1.0] - 2017-08-28 ##
Feature addition: QFuture support

### Added ###
- [#6] Support for QFuture: FuturePromise and FutureDeferred
- [#7] Coverage report

### Changed ###
- Replaced Qt custom keywords (signals, slots, emit) with the corresponding preprocessor macros
  to be compatible with the "no_keywords" configuration.
- NetworkPromise now filters an unexpected 0/0 upload progress signal from QNetworkReply


---


## [1.0.0] - 2017-06-21 ##
Initial release.

### Added ###
- Basic Promise and Deferred functionality
- Combining promises using Promise::all() and Promise::any()
- Support for QNetworkReply: NetworkPromise and NetworkDeferred
- Documentation
- [#3] PromiseSitter: stores Promises to ensure attached actions are executed


---


[2.0.0]: https://gitlab.com/julrich/QtPromise/tags/2.0.0
[1.2.0]: https://gitlab.com/julrich/QtPromise/tags/1.2.0
[1.1.0]: https://gitlab.com/julrich/QtPromise/tags/1.1.0
[1.0.0]: https://gitlab.com/julrich/QtPromise/tags/1.0.0