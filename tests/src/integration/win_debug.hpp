/*
  Copyright (c) 2016 DataStax, Inc.

  This software can be used solely with DataStax Enterprise. Please consult the
  license at http://www.datastax.com/terms/datastax-dse-driver-license-terms
*/
#ifndef __WIN_DEBUG_HPP__
#define __WIN_DEBUG_HPP__
#include <gtest/gtest.h>

#if defined(_WIN32) && defined(_DEBUG)
# ifdef USE_VISUAL_LEAK_DETECTOR
#   include <vld.h>
# else
// Enable memory leak detection
# define _CRTDBG_MAP_ALLOC
# include <stdlib.h>
# include <crtdbg.h>

// Enable memory leak detection for new operator
# ifndef DBG_NEW
#   define DBG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#   define new DBG_NEW
# endif

/**
 * Output the memory leak results to the console
 *
 * @param report_type Type of report (warn, error, or assert)
 * @param message message
 * @param error_code Error code
 * @return Result to return to CRT processing (1 will stop processing report)
 */
inline int __cdecl output_memory_leak_results(int report_type, char* message, int* error_code) {
  std::cerr << message;
  return 1;
}
# endif

/**
 * Memory leak listener for detecting memory leaks on Windows more efficiently.
 */
class MemoryLeakListener : public testing::EmptyTestEventListener {
public:

# ifndef USE_VISUAL_LEAK_DETECTOR
  void OnTestProgramStart(const testing::UnitTest& unit_test) {
    // Install the memory leak reporting
    _CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, &output_memory_leak_results);
  }

  void OnTestProgramEnd(const testing::UnitTest& unit_test) {
    // Uninstall/Remove the memory leak reporting
    _CrtSetReportHook2(_CRT_RPTHOOK_REMOVE, &output_memory_leak_results);
  }
# endif

  void OnTestStart(const testing::TestInfo& test_information) {
# ifdef USE_VISUAL_LEAK_DETECTOR
    VLDMarkAllLeaksAsReported();
    VLDEnable();
# else
    // Get the starting memory state
    _CrtMemCheckpoint(&memory_start_state_);
# endif
  }

  void OnTestEnd(const testing::TestInfo& test_information) {
    // Check for memory leaks if the test was successful
    if(test_information.result()->Passed()) {
      check_leaks(test_information);
    }
}

private:
# ifndef USE_VISUAL_LEAK_DETECTOR
  /**
   * Starting memory state (before start of test)
   */
  _CrtMemState memory_start_state_;
# endif

  /**
   * Check for memory leaks based on the starting memory state
   *
   * @param test_information Information about the test
   */
  void check_leaks(const testing::TestInfo& test_information) {
# ifdef USE_VISUAL_LEAK_DETECTOR
    // Determine if a difference exists (e.g. leak)
    VLDDisable();
    if (VLDGetLeaksCount() > 0) {
      VLDReportLeaks();
      VLDMarkAllLeaksAsReported();
# else
    // Get the ending memory state for the test
    _CrtMemState memory_end_state;
    _CrtMemCheckpoint(&memory_end_state);
    _CrtMemState memory_state_difference;

    // Determine if a difference exists (e.g. leak)
    if (_CrtMemDifference(&memory_state_difference, &memory_start_state_, &memory_end_state)) {
      _CrtMemDumpAllObjectsSince(&memory_start_state_);
      _CrtMemDumpStatistics(&memory_state_difference);
# endif
      FAIL() << "Memory leaks detected in "
             << test_information.test_case_name()
             << "." << test_information.name();
    }
  }
};
#endif

#endif // __WIN_DEBUG_HPP__
