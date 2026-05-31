// Minimal zero-dependency test harness.
// Wire in GoogleTest / Catch2 once a dependency-management story is chosen.

#ifndef FORKLIFT_TESTS_TEST_HARNESS_H_
#define FORKLIFT_TESTS_TEST_HARNESS_H_

#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace fss::test {

struct Case {
    std::string                name;
    std::function<void(bool&)> body;   // body sets ok=false on first failure
};

inline std::vector<Case>& registry() { static std::vector<Case> r; return r; }

struct Registrar {
    Registrar(std::string name, std::function<void(bool&)> body) {
        registry().push_back({std::move(name), std::move(body)});
    }
};

inline int run_all() {
    int failed = 0;
    for (auto& c : registry()) {
        bool ok = true;
        try { c.body(ok); }
        catch (const std::exception& e) {
            ok = false;
            std::fprintf(stderr, "  exception in %s: %s\n", c.name.c_str(), e.what());
        }
        std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", c.name.c_str());
        if (!ok) ++failed;
    }
    std::printf("---\n%zu cases, %d failed\n", registry().size(), failed);
    return failed == 0 ? 0 : 1;
}

}  // namespace fss::test

#define FSS_CONCAT2(a, b) a##b
#define FSS_CONCAT(a, b) FSS_CONCAT2(a, b)

#define TEST_CASE(NAME)                                                                  \
    static void FSS_CONCAT(fss_case_, __LINE__)(bool& fss_ok);                           \
    static ::fss::test::Registrar FSS_CONCAT(fss_reg_, __LINE__)(                        \
        NAME, &FSS_CONCAT(fss_case_, __LINE__));                                         \
    static void FSS_CONCAT(fss_case_, __LINE__)([[maybe_unused]] bool& fss_ok)

#define EXPECT_TRUE(EXPR)                                                                \
    do {                                                                                 \
        if (!(EXPR)) {                                                                   \
            std::fprintf(stderr, "  EXPECT_TRUE failed: %s @ %s:%d\n",                   \
                         #EXPR, __FILE__, __LINE__);                                     \
            fss_ok = false;                                                              \
        }                                                                                \
    } while (0)

#define EXPECT_EQ(A, B)                                                                  \
    do {                                                                                 \
        if (!((A) == (B))) {                                                             \
            std::fprintf(stderr, "  EXPECT_EQ failed: %s == %s @ %s:%d\n",               \
                         #A, #B, __FILE__, __LINE__);                                    \
            fss_ok = false;                                                              \
        }                                                                                \
    } while (0)

#define EXPECT_NEAR(A, B, EPS)                                                           \
    do {                                                                                 \
        const auto _a = (A);                                                             \
        const auto _b = (B);                                                             \
        if (!(((_a) - (_b) < (EPS)) && ((_b) - (_a) < (EPS)))) {                         \
            std::fprintf(stderr, "  EXPECT_NEAR failed: %s ≈ %s @ %s:%d\n",              \
                         #A, #B, __FILE__, __LINE__);                                    \
            fss_ok = false;                                                              \
        }                                                                                \
    } while (0)

#endif  // FORKLIFT_TESTS_TEST_HARNESS_H_
