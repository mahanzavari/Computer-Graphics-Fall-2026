#pragma once

#include "Geometry.h"

namespace team12 {

inline constexpr int seatingSectors = 4;
inline constexpr float seatSpacing = 0.55f;
inline constexpr float seatHeight = 0.42f;
inline constexpr float stairOverlap = 0.05f;

struct RowSpan {
    float start;
    float end;
};

using RowSpans = std::array<RowSpan, 2>;
RowSpans seatingSpans(const StadiumLayout& layout, int sector, glm::vec2 radii, float depth);

enum class SeatPattern { WhiteBands, SolidRed };
inline constexpr SeatPattern seatPattern = SeatPattern::WhiteBands;
inline constexpr bool useDetailedSeats = false;

// Build once in init(). Seating and Crowd both consume layout.seats.
std::vector<SeatPlacement> makeSeatPlacements(const StadiumLayout& layout);

struct SeatingRow {
    glm::vec3 origin;
    Mesh tread;
    std::vector<SeatPlacement> seats;
};

struct SeatingSection {
    glm::vec3 origin;
    int tier;
    std::vector<SeatingRow> rows;
};

struct SeatingBowl {
    std::vector<SeatingSection> sections;
    Mesh stairs;
    Mesh rails;
    Mesh detailedSeat;
    std::size_t detailedSeatCount = 0;
};

SeatingBowl makeSeatingBowl(const StadiumLayout& layout, citylab::Context& ctx);
void drawSeatingBowl(const SeatingBowl& bowl);

}  // namespace team12
