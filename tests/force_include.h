// SPDX-License-Identifier: Unlicense
//
// single force-include for every test/benchmark tu (see tests/cmakelists.txt):
// placement new first, then the umbrella header. one wrapper header instead
// of two -include/-fi flags because cmake drops a repeated -include token
// when generating (observed with the ninja generator)

#pragma once

#include <new>
#include <ystl/ystl.h>
