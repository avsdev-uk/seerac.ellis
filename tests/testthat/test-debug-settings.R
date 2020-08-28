
test_that("Debug off by default", {
  print(seerac.ellis::getDebug())
  expect_equal(seerac.ellis::getDebug(), 0)
})
test_that("Debug on works", {
  seerac.ellis::debugOn()
  expect_equal(seerac.ellis::getDebug(), 1)
})
test_that("Custom debug level works", {
  seerac.ellis::setDebug(2)
  expect_equal(seerac.ellis::getDebug(), 2)
})
test_that("Debug off works", {
  seerac.ellis::debugOff()
  expect_equal(seerac.ellis::getDebug(), 0)
})
test_that("Negative debug level handled", {
  seerac.ellis::debugOn()
  expect_warning(seerac.ellis::setDebug(-1))
  expect_equal(seerac.ellis::getDebug(), 0)
})
