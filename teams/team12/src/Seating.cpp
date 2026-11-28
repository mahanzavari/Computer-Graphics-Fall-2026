#include "Seating.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <glm/gtc/constants.hpp>

namespace team12 {

RowSpans seatingSpans(const StadiumLayout& layout, int sector, glm::vec2 radii, float depth) {
    const auto inner = radii - glm::vec2{depth * 0.5f};
    const auto outer = radii + glm::vec2{depth * 0.5f};
    const auto corridorWidth = [&](int aisle) {
        const auto size = layout.spectatorCorridors[aisle % seatingSectors].size;
        return std::min(size.x, size.y) * 0.5f + stairOverlap;
    };
    const bool alongX = sector % 2 == 0;
    const float quarter = glm::half_pi<float>();
    const float start = sector * quarter + std::asin(corridorWidth(sector) / (alongX ? inner.y : inner.x));
    const float end = (sector + 1) * quarter - std::asin(corridorWidth(sector + 1) / (alongX ? inner.x : inner.y));
    RowSpans spans{{{start, end}, {}}};
    if (sector == seatingSectors - 1) {
        const auto& tunnel = layout.playerTunnel;
        const float halfWidth = tunnel.size.y * 0.5f + 0.12f;
        const float low = glm::two_pi<float>() + std::asin((tunnel.center.y - halfWidth) / inner.y);
        const float high = glm::two_pi<float>() + std::asin((tunnel.center.y + halfWidth) / outer.y);
        spans = {{{start, std::min(end, low)}, {std::max(start, high), end}}};
    }
    return spans;
}

std::vector<SeatPlacement> makeSeatPlacements(const StadiumLayout& layout) {
    constexpr int samples = 128;
    std::vector<SeatPlacement> seats;
    for (int tierIndex = 0; tierIndex < static_cast<int>(layout.tiers.size()); ++tierIndex) {
        const auto& tier = layout.tiers[tierIndex];
        for (int row = 0; row < tier.rows; ++row) {
            const auto radii = tier.firstRowRadii + glm::vec2{row * tier.rowDepth};
            const float height = tier.firstRowHeight + row * tier.risePerRow;
            for (int sector = 0; sector < seatingSectors; ++sector) {
                for (const auto& span : seatingSpans(layout, sector, radii, tier.rowDepth)) {
                    if (span.end <= span.start) continue;
                    const float step = (span.end - span.start) / samples;
                    std::array<float, samples + 1> lengths{};
                    auto previous = ovalPoint(radii, span.start);
                    for (int i = 1; i <= samples; ++i) {
                        const auto point = ovalPoint(radii, span.start + i * step);
                        lengths[i] = lengths[i - 1] + glm::length(point - previous);
                        previous = point;
                    }
                    const int count = static_cast<int>(lengths.back() / seatSpacing);
                    for (int i = 0; i < count; ++i) {
                        const float distance = lengths.back() * (i + 0.5f) / count;
                        const int upper = static_cast<int>(std::lower_bound(lengths.begin(), lengths.end(), distance) - lengths.begin());
                        const float fraction = (distance - lengths[upper - 1]) / (lengths[upper] - lengths[upper - 1]);
                        const float angle = span.start + (upper - 1 + fraction) * step;
                        const auto position = ovalPoint(radii, angle);
                        const auto inward = -position / (radii * radii);
                        const float yaw = glm::degrees(std::atan2(inward.x, inward.y));
                        seats.push_back({static_cast<int>(seats.size()), {position.x, height, position.y},
                                         yaw, tierIndex, row, sector, true});
                    }
                }
            }
        }
    }
    return seats;
}

}  // namespace team12
