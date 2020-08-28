
test_that("Mode is normal by default", {
  expect_equal(seerac.ellis::getMode(), 0)
})
test_that("Mode to TEST works", {
  seerac.ellis::testModeOn()
  expect_equal(seerac.ellis::getMode(), 1)
})
test_that("mode to NORMAL works", {
  seerac.ellis::testModeOff()
  expect_equal(seerac.ellis::getMode(), 0)
})
test_that("Unknown mode is rejected", {
  expect_error(seerac.ellis::setMode(5))
})