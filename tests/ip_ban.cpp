/* Standalone tests for the proxy IP-ban cache. */

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>

#include "proxy/IpBan.h"


namespace {


void check(bool condition, const char *expression, const char *file, int line)
{
    if (!condition) {
        std::fprintf(stderr, "%s:%d: check failed: %s\n", file, line, expression);
        std::abort();
    }
}


#define CHECK(expression) check((expression), #expression, __FILE__, __LINE__)


void testExpiryBoundary()
{
    xmrig::IpBan bans;
    constexpr uint64_t now = 1000;

    CHECK(bans.add("192.0.2.1", now));
    CHECK(bans.isBanned("192.0.2.1", now));
    CHECK(bans.isBanned("192.0.2.1", now + xmrig::IpBan::kDurationMs - 1));
    CHECK(!bans.isBanned("192.0.2.1", now + xmrig::IpBan::kDurationMs));
    CHECK(bans.size() == 0);
}


void testDuplicateDoesNotExtend()
{
    xmrig::IpBan bans;
    constexpr uint64_t now = 2000;

    CHECK(bans.add("192.0.2.2", now));
    CHECK(!bans.add("192.0.2.2", now + 5000));
    CHECK(bans.isBanned("192.0.2.2", now + xmrig::IpBan::kDurationMs - 1));
    CHECK(!bans.isBanned("192.0.2.2", now + xmrig::IpBan::kDurationMs));
}


void testCustomDuration()
{
    xmrig::IpBan bans;
    constexpr uint64_t now = 2000;
    constexpr uint64_t duration = 3ULL * 60 * 60 * 1000;
    bans.setDurationMs(duration);

    CHECK(bans.durationMs() == duration);
    CHECK(bans.add("192.0.2.10", now));
    CHECK(!bans.add("192.0.2.10", now + 5000));
    CHECK(bans.isBanned("192.0.2.10", now + duration - 1));
    CHECK(!bans.isBanned("192.0.2.10", now + duration));
}


void testDurationChanges()
{
    xmrig::IpBan bans;
    constexpr uint64_t now = 3000;
    constexpr uint64_t duration = 2ULL * 60 * 60 * 1000;

    CHECK(bans.durationMs() == xmrig::IpBan::kDurationMs);
    CHECK(bans.add("192.0.2.11", now));
    bans.setDurationMs(xmrig::IpBan::kDurationMs);
    CHECK(bans.isBanned("192.0.2.11", now + 1));

    bans.setDurationMs(duration);
    CHECK(bans.size() == 0);
    CHECK(!bans.isBanned("192.0.2.11", now + 1));
    CHECK(bans.add("192.0.2.12", now + 1));
    bans.setDurationMs(duration);
    CHECK(bans.isBanned("192.0.2.12", now + duration));
    CHECK(!bans.isBanned("192.0.2.12", now + duration + 1));

    CHECK(bans.add("192.0.2.13", now + duration + 2));
    bans.setDurationMs(0);
    CHECK(bans.durationMs() == 0);
    CHECK(bans.size() == 0);
    CHECK(!bans.isBanned("192.0.2.13", now + duration + 3));
    CHECK(!bans.add("192.0.2.13", now + duration + 3));
    CHECK(bans.size() == 0);

    bans.setDurationMs(duration);
    CHECK(bans.add("192.0.2.13", now + duration + 4));
    CHECK(bans.isBanned("192.0.2.13", now + duration + 4));
}


void testExpiryDoesNotWrap()
{
    xmrig::IpBan bans;
    constexpr uint64_t maximum = std::numeric_limits<uint64_t>::max();
    constexpr uint64_t duration = static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) * 60 * 60 * 1000;
    constexpr uint64_t now = maximum - 5000;
    bans.setDurationMs(duration);

    CHECK(bans.add("192.0.2.14", now));
    CHECK(bans.isBanned("192.0.2.14", now));
    CHECK(bans.isBanned("192.0.2.14", maximum - 1));
    CHECK(!bans.isBanned("192.0.2.14", maximum));
    CHECK(bans.size() == 0);
}


void testExpiryPruneAndReadd()
{
    xmrig::IpBan bans;
    constexpr uint64_t first = 4000;

    CHECK(bans.add("192.0.2.3", first));
    CHECK(bans.add("192.0.2.4", first + 1));
    bans.prune(first + xmrig::IpBan::kDurationMs);
    CHECK(bans.size() == 1);
    CHECK(!bans.isBanned("192.0.2.3", first + xmrig::IpBan::kDurationMs));
    CHECK(bans.isBanned("192.0.2.4", first + xmrig::IpBan::kDurationMs));
    CHECK(bans.add("192.0.2.3", first + xmrig::IpBan::kDurationMs));
    CHECK(bans.size() == 2);
    CHECK(bans.isBanned("192.0.2.3", first + xmrig::IpBan::kDurationMs));
}


void testBoundedEviction()
{
    xmrig::IpBan bans;

    for (std::size_t i = 0; i < xmrig::IpBan::kCapacity; ++i) {
        CHECK(bans.add(("ip-" + std::to_string(i)).c_str(), 3000));
    }

    CHECK(bans.size() == xmrig::IpBan::kCapacity);
    CHECK(bans.isBanned("ip-0", 3001));
    CHECK(bans.add("ip-new", 3001));
    CHECK(bans.size() == xmrig::IpBan::kCapacity);
    CHECK(!bans.isBanned("ip-0", 3001));
    CHECK(bans.isBanned("ip-1", 3001));
    CHECK(bans.isBanned("ip-new", 3001));
    CHECK(bans.add("ip-0", 3002));
    CHECK(!bans.isBanned("ip-1", 3002));
    CHECK(bans.isBanned("ip-0", 3002));
}


void testRejectionMatching()
{
    CHECK(xmrig::IpBan::isLowDifficulty("Low difficulty share"));
    CHECK(xmrig::IpBan::isLowDifficulty("LOW DIFFICULTY SHARE"));
    CHECK(xmrig::IpBan::isLowDifficulty("lOw DiFfIcUlTy ShArE"));
    CHECK(!xmrig::IpBan::isLowDifficulty(""));
    CHECK(!xmrig::IpBan::isLowDifficulty("Low difficulty share "));
    CHECK(!xmrig::IpBan::isLowDifficulty("Low difficulty share\n"));
    CHECK(!xmrig::IpBan::isLowDifficulty("Low difficulty"));
    CHECK(!xmrig::IpBan::isLowDifficulty("Duplicate share"));
    CHECK(!xmrig::IpBan::isLowDifficulty(nullptr));
}


} // namespace


int main()
{
    testExpiryBoundary();
    testDuplicateDoesNotExtend();
    testCustomDuration();
    testDurationChanges();
    testExpiryDoesNotWrap();
    testExpiryPruneAndReadd();
    testBoundedEviction();
    testRejectionMatching();
    return 0;
}
