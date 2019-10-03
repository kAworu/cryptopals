[![Build Status](https://travis-ci.org/kAworu/cryptopals.svg?branch=master)](https://travis-ci.org/kAworu/cryptopals)

# cryptopals

Just me going tough the cryptopals challenges. It is slow, insecure code *by
design*. So, don't use it.

## Building

Start by `git submodule init && git submodule update`. Then, simply `make`.
Oh, you'll need [Cmake](https://cmake.org/) (sorry) and a C11 compiler.

## ...and?

Type `make test`, sit back and enjoy the show. Alternatively -- if you're super
lazy -- just check the Travis-CI build output.

## ...aaaand?

That's it. It's only a bunch of functions in a lib wrapped by tests using the
data from the challenges. It's cool though, hopefully well commented and using
the [µnit Testing Framework](https://nemequ.github.io/munit/).

## coverage?

```sh
% BUILD_TYPE=COVERAGE make
% ./build/testrunner --no-fork --log-visible debug --show-stderr \
    --param srp_server ./python/srp_server.py \
    --param mac_server ./python/hmac_padding_oracle.py \
    --param mac_filepath ./README.md
% lcov --directory build/CMakeFiles/cryptopals.dir/src \
    --base-directory build/CMakeFiles/cryptopals.dir/src \
    --gcov-tool $PWD/llvm-gcov.sh \
    --capture \
    -o cov.info
% genhtml cov.info -o output
```
