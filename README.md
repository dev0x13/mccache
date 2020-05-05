# Markov chain cache

This repository contains a reference implementation for time-homogeneous Markov chain based cache,
which was created as a part of my master degree dissertation.

**Algorithm description will be published soon.**

## Software specific notes

* This implementation is not intended to be efficient enough for production usage. Its purpose is to
  set up experiments with a decent balance between code readability and performance, and to be an extension
  for the algorithm description from the programming point of view.
* This implementation basically does not provided error handling and reporting. All the expected preconditions
  are represented by asserts. 
* Since this implementation employs several linear algebra routines, it will try to use Intel MKL by default,
  which should be installed to the host system. If MKL is not installed, then naive BLAS implementation is used
  (this is not preferred since these routines are really slow comparing to any BLAS library).
  However, you may use any BLAS library of you choice since the code, which utilizes these routines, is isolated
  to `src/math/linalg_impl.cpp`.

## Building

CMake 3.15 is required for building as there is apparently a bug in FindBLAS module in the earlier CMake versions,
which makes Intel MKL usage very problematic.

The build was tested on Ubuntu 16.04 GCC 5.5.0, no guarantees are given regarding to other environments
(especially Windows).

## Testing

This repository provides two options to evaluate the algorithm. These options refer to the two different caching system purposes:
* Static, when all the items are not cached at the beginning and the items set is determined, i.e. no new items 
  can be stored at the runtime.
* Dynamic, when the storage system contains no items at the beginning and reading operations (retrieving existing items)
  are combined with writing operations (storing new items) at the runtime.
  
This algorithm was implemented for the dynamic scenario, however preliminary testing results show that it can be
successfully employed for both scenarios.

There are two utilities provided for evaluating these scenarios:
* `mccache_evaluation_test_static` for static scenario. Usage example is the following:
```bash
./mccache_evaluation_test_static ../sample_traces/static/1999-011-usertrace-98-webcachesim.tr 13963100 transitions 10 1
```
Traces are expected in the [webcachesim](https://github.com/dasebe/webcachesim) space-separated format with three columns (timestamp, object ID, size) and a separate request on each line:

| time |  id | size |
| ---- | --- | ---- |
|   1  |  1  |  120 |
|   2  |  2  |   64 |
|   3  |  1  |  120 |
|   4  |  3  |  14  |
|   4  |  1  |  120 |

Sample trace can be found at `sample_traces/static`.

* `mccache_evaluation_test_dynamic` for dynamic scenario. Usage example is the following:
```bash
./mccache_evaluation_test_dynamic ../sample_traces/dynamic/pattern_mixed_fixed_size.tr 6291456 transitions 10 1
```
Traces are expected in the extended webcachesim format. The extension consist of first column representing the type of request
(`s` for set request, `g` for get request):

| type | time |  id | size |
| ---- | ---- | --- | ---- |
|   s  |   1  |  1  |  120 |
|   s  |   2  |  2  |   64 |
|   g  |   3  |  1  |  120 |
|   g  |   4  |  2  |  64  |
|   s  |   4  |  3  |  120 |

Sample traces can be found at `sample_traces/dynamic`.