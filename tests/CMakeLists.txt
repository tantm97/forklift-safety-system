# Minimal hand-rolled test harness (no external test framework required).
add_executable(forklift_tests
    test_main.cpp
    domain/bounding_box_test.cpp
    domain/safety_zone_test.cpp
    application/safety_zone_service_test.cpp
    application/risk_detection_service_test.cpp
)
target_link_libraries(forklift_tests PRIVATE forklift_core)

add_test(NAME forklift_tests COMMAND forklift_tests)
