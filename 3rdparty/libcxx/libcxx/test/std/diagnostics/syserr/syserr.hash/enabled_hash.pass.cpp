//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++98, c++03

// <system_error>

// Test that <system_error> provides all of the arithmetic, enum, and pointer
// hash specializations.

#include <system_error>

#include "poisoned_hash_helper.hpp"

int main() {
  test_library_hash_specializations_available();
  {
    test_hash_enabled_for_type<std::error_code>();
    test_hash_enabled_for_type<std::error_condition>();
  }
}
