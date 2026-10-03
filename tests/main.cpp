#include <ystl/test.h>

thread_local SectionTracker g_sections;
thread_local TestResults g_results;

int main (int argc, char **argv) {
  return ystl::test::run_main (argc, argv);
}
