// Copyright (c) 2017-2018 Nguyen Viet Giang. All rights reserved.
#ifndef _AVM_VALUE_H_
#define _AVM_VALUE_H_

#include <avm/prereq.h>

/// NaN boxed value.
typedef f64 avalue_t;
AALIGNAS(avalue_t, 8);

/// Value size is fixed 8 bytes.
ASTATIC_ASSERT(sizeof(avalue_t) == 8);

#endif // !_AVM_VALUE_H_