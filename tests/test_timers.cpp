// test_timers.cpp tests for ystl timers.h with simulated time
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

// global simulated time value and helper
static float g_time = 0.0f;

static void reset_time (float t = 0.0f) {
  g_time = t;
  timer_source.set_time_address (&g_time);
}

// timersource

TEST_CASE ("TimerSource::value returns pointed-to float [timers]") {
  reset_time (3.5f);
  REQUIRE (timer_source.value () == Approx (3.5f));

  g_time = 7.0f;
  REQUIRE (timer_source.value () == Approx (7.0f));
}

// countdowntimer

TEST_CASE ("CountdownTimer default construction is not started [timers]") {
  reset_time (0.0f);
  CountdownTimer ct;
  REQUIRE_FALSE (ct.started ());
}

TEST_CASE ("CountdownTimer start makes it started [timers]") {
  reset_time (1.0f);
  CountdownTimer ct;
  ct.start (2.0f);
  REQUIRE (ct.started ());
}

TEST_CASE ("CountdownTimer elapsed returns false before duration expires [timers]") {
  reset_time (0.0f);
  CountdownTimer ct;
  ct.start (5.0f); // expires at t=5

  g_time = 3.0f;
  REQUIRE_FALSE (ct.elapsed ());
}

TEST_CASE ("CountdownTimer elapsed returns true after duration expires [timers]") {
  reset_time (0.0f);
  CountdownTimer ct;
  ct.start (2.0f); // expires at t=2

  g_time = 3.0f;
  REQUIRE (ct.elapsed ());
}

TEST_CASE ("CountdownTimer remainingTime is positive before expiry [timers]") {
  reset_time (0.0f);
  CountdownTimer ct;
  ct.start (10.0f);

  g_time = 3.0f;
  REQUIRE (ct.remaining_time () == Approx (7.0f).epsilon (0.01f));
}

TEST_CASE ("CountdownTimer elapsedTime works after expiry [timers]") {
  reset_time (0.0f);
  CountdownTimer ct;
  ct.start (5.0f);

  g_time = 8.0f; // 3 seconds past expiry
  REQUIRE (ct.elapsed_time () == Approx (8.0f).epsilon (0.01f));
}

TEST_CASE ("CountdownTimer countdownDuration returns duration when started [timers]") {
  reset_time (0.0f);
  CountdownTimer ct;
  ct.start (3.0f);
  REQUIRE (ct.countdown_duration () == Approx (3.0f));
}

TEST_CASE ("CountdownTimer countdownDuration returns 0 when not started [timers]") {
  reset_time (0.0f);
  CountdownTimer ct;
  REQUIRE (ct.countdown_duration () == Approx (0.0f));
}

TEST_CASE ("CountdownTimer reset restarts with same duration [timers]") {
  reset_time (0.0f);
  CountdownTimer ct;
  ct.start (5.0f);

  g_time = 3.0f;
  ct.reset (); // new expiry at t=3+5=8

  g_time = 7.0f;
  REQUIRE_FALSE (ct.elapsed ());

  g_time = 9.0f;
  REQUIRE (ct.elapsed ());
}

TEST_CASE ("CountdownTimer invalidate stops the timer [timers]") {
  reset_time (0.0f);
  CountdownTimer ct;
  ct.start (5.0f);
  ct.invalidate ();
  REQUIRE_FALSE (ct.started ());
}

TEST_CASE ("CountdownTimer constructed with duration starts immediately [timers]") {
  reset_time (0.0f);
  CountdownTimer ct (4.0f);
  REQUIRE (ct.started ());
  REQUIRE_FALSE (ct.elapsed ());

  g_time = 5.0f;
  REQUIRE (ct.elapsed ());
}

// intervaltimer

TEST_CASE ("IntervalTimer default construction is not started [timers]") {
  reset_time (0.0f);
  IntervalTimer it;
  REQUIRE_FALSE (it.started ());
}

TEST_CASE ("IntervalTimer start marks the current time [timers]") {
  reset_time (5.0f);
  IntervalTimer it;
  it.start ();
  REQUIRE (it.started ());
}

TEST_CASE ("IntervalTimer elapsedTime measures since start [timers]") {
  reset_time (2.0f);
  IntervalTimer it;
  it.start ();

  g_time = 7.0f;
  REQUIRE (it.elapsed_time () == Approx (5.0f).epsilon (0.01f));
}

TEST_CASE ("IntervalTimer elapsedTime returns max when not started [timers]") {
  reset_time (0.0f);
  IntervalTimer it;
  REQUIRE (it.elapsed_time () == Approx (detail::kMaxTimerValue).epsilon (1.0f));
}

TEST_CASE ("IntervalTimer lessThan returns true when within duration [timers]") {
  reset_time (0.0f);
  IntervalTimer it;
  it.start ();

  g_time = 3.0f;
  REQUIRE (it.less_than (5.0f));
  REQUIRE_FALSE (it.less_than (2.0f));
}

TEST_CASE ("IntervalTimer greaterThan returns true after duration [timers]") {
  reset_time (0.0f);
  IntervalTimer it;
  it.start ();

  g_time = 10.0f;
  REQUIRE (it.greater_than (5.0f));
  REQUIRE_FALSE (it.greater_than (15.0f));
}

TEST_CASE ("IntervalTimer reset re-anchors to current time [timers]") {
  reset_time (0.0f);
  IntervalTimer it;
  it.start ();

  g_time = 10.0f;
  it.reset (); // now anchored at t=10

  g_time = 12.0f;
  REQUIRE (it.elapsed_time () == Approx (2.0f).epsilon (0.01f));
}

TEST_CASE ("IntervalTimer invalidate resets to invalid timestamp [timers]") {
  reset_time (10.0f);
  IntervalTimer it;
  it.start ();

  it.invalidate ();
  // invalidate must restore negative sentinel like countdowntimer
  REQUIRE_FALSE (it.started ());
  REQUIRE (it.elapsed_time () == Approx (detail::kMaxTimerValue).epsilon (1.0f));
}

TEST_CASE ("IntervalTimer invalidated timer behaves as never-started in comparisons [timers]") {
  reset_time (10.0f);
  IntervalTimer it;
  it.start ();
  it.invalidate ();

  // invalid timer counts as elapsed long ago
  REQUIRE_FALSE (it.less_than (5.0f));
  REQUIRE (it.greater_than (5.0f));
}

TEST_CASE ("IntervalTimer restarted after invalidate tracks time again [timers]") {
  reset_time (10.0f);
  IntervalTimer it;
  it.start ();
  it.invalidate ();
  it.start ();

  g_time = 12.0f;
  REQUIRE (it.started ());
  REQUIRE (it.elapsed_time () == Approx (2.0f).epsilon (0.01f));
}

TEST_CASE ("IntervalTimer started at time zero is started [timers]") {
  reset_time (0.0f);
  IntervalTimer it;
  it.start (); // timestamp becomes 0.0, sentinel is -1.0

  g_time = 4.0f;
  REQUIRE (it.started ());
  REQUIRE (it.elapsed_time () == Approx (4.0f).epsilon (0.01f));
}

TEST_CASE ("IntervalTimer never-started comparisons delegate to elapsedTime [timers]") {
  reset_time (10.0f);
  IntervalTimer it;

  REQUIRE_FALSE (it.less_than (5.0f));
  REQUIRE (it.greater_than (5.0f));
}

// countdowntimer guards

TEST_CASE ("CountdownTimer never-started query methods report inactive state [timers]") {
  reset_time (10.0f);
  CountdownTimer ct;

  REQUIRE_FALSE (ct.started ());
  REQUIRE (ct.elapsed ());
  REQUIRE (ct.elapsed_time () == Approx (detail::kMaxTimerValue).epsilon (1.0f));
  REQUIRE (ct.remaining_time () == Approx (-detail::kMaxTimerValue).epsilon (1.0f));
}

TEST_CASE ("CountdownTimer invalidated query methods report inactive state [timers]") {
  reset_time (10.0f);
  CountdownTimer ct;
  ct.start (5.0f);
  ct.invalidate ();

  g_time = 12.0f;
  REQUIRE_FALSE (ct.started ());
  REQUIRE (ct.elapsed ());
  REQUIRE (ct.elapsed_time () == Approx (detail::kMaxTimerValue).epsilon (1.0f));
  REQUIRE (ct.remaining_time () == Approx (-detail::kMaxTimerValue).epsilon (1.0f));
}

TEST_CASE ("CountdownTimer started at time zero is started [timers]") {
  reset_time (0.0f);
  CountdownTimer ct;
  ct.start (5.0f);

  g_time = 2.0f;
  REQUIRE (ct.started ());
  REQUIRE_FALSE (ct.elapsed ());
  REQUIRE (ct.remaining_time () == Approx (3.0f).epsilon (0.01f));
}
