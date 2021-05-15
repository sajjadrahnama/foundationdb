/*
 * String.h
 *
 * This source file is part of the FoundationDB open source project
 *
 * Copyright 2013-2021 Apple Inc. and the FoundationDB project authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FLOW_STRING_H
#define FLOW_STRING_H

#include <sstream>
#include <string>

namespace {

template <typename Arg>
std::stringstream& _concatHelper(std::stringstream&& ss, const Arg& arg) {
	ss << arg;
	return ss;
}

template <typename First, typename... Args>
std::stringstream& _concatHelper(std::stringstream&& ss, const First& first, const Args&... args) {
	ss << first;
	return _concatHelper(std::move(ss), std::forward<const Args&>(args)...);
}

} // anonymous namespace

// Concatencate a list of objects to string
template <typename... Args>
std::string concatToString(const Args&... args) {
	return _concatHelper(std::stringstream(), args...).str();
}

#endif // FLOW_STRING_H