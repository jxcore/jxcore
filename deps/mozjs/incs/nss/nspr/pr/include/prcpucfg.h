/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if defined(_WIN32)
#include "md/_win95.cfg"
#elif defined(__APPLE__)
#include "md/_darwin.cfg"
#elif defined(__linux__)
#include "md/_linux.cfg"
#else
#error Add a case for your platform
#endif
