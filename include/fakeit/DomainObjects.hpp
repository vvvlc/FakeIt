/*
 * Copyright (c) 2014 Eran Pe'er.
 *
 * This program is made available under the terms of the MIT License.
 *
 * Created on Mar 10, 2014
 */

#ifndef DomainObjects_h__
#define DomainObjects_h__

#include <string>

namespace fakeit {

struct MockObject {
};

struct Method {
	virtual ~Method() = default;
};

}

#endif // DomainObjects_h__