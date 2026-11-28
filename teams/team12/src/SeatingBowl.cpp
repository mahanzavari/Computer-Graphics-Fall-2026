#include "Seating.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <utility>
#include <glm/gtc/constants.hpp>

namespace team12 {
namespace {

// CityLab measures feedback polygons as triangles, including the four in the strip.
constexpr int compactSeatVertices = 14;

void registerTread(citylab::Context& ctx, const StadiumLayout& layout,
                   const std::array<glm::vec2, 4>& points, float height) {
    glm::vec2 low = points[0], high = points[0];
    for (const auto point : points) {
        low = glm::min(low, point);
        high = glm::max(high, point);
    }
    // A wedge's axis-aligned SDK collider can reach farther infield than its mesh.
    // Cut those corners back to keep the pitch, runoff and goal reservations walkable.
    struct Bounds {
        glm::vec2 low;
        glm::vec2 high;
    };
    constexpr float walkerMargin = 0.35f;
    const auto halfPitch = layout.pitch.size * 0.5f;
    const std::array<glm::vec2, 2> clearHalfSizes{{
        halfPitch + glm::vec2{0.0f, layout.sidelineClearance} + glm::vec2{walkerMargin},
        {halfPitch.x + layout.goalClearance.x + walkerMargin,
         layout.goalClearance.y * 0.5f + walkerMargin},
    }};
    std::vector<Bounds> boxes{{low, high}};
    for (const auto halfSize : clearHalfSizes) {
        const auto clearLow = layout.pitch.center - halfSize;
        const auto clearHigh = layout.pitch.center + halfSize;
        std::vector<Bounds> remainder;
        const auto add = [&](glm::vec2 lo, glm::vec2 hi) {
            if (lo.x < hi.x && lo.y < hi.y) remainder.push_back({lo, hi});
        };
        for (const auto& box : boxes) {
            const auto cutLow = glm::max(box.low, clearLow);
            const auto cutHigh = glm::min(box.high, clearHigh);
            if (cutLow.x >= cutHigh.x || cutLow.y >= cutHigh.y) {
                remainder.push_back(box);
                continue;
            }
            add(box.low, {cutLow.x, box.high.y});
            add({cutHigh.x, box.low.y}, box.high);
            add({cutLow.x, box.low.y}, {cutHigh.x, cutLow.y});
            add({cutLow.x, cutHigh.y}, {cutHigh.x, box.high.y});
        }
        boxes.swap(remainder);
    }
    const auto origin = layout.stadiumCenter;
    for (const auto& box : boxes) {
        ctx.addCollider({origin.x + box.low.x, 0.0f, origin.z + box.low.y},
                        {origin.x + box.high.x, height, origin.z + box.high.y});
    }
}

void buildDetailedSeat(Mesh& mesh) {
    constexpr glm::vec3 frontLeft{-0.18f, -0.01f, 0.20f};
    constexpr glm::vec3 frontRight{0.18f, -0.01f, 0.20f};
    constexpr glm::vec3 hingeLeft{-0.22f, 0.0f, -0.15f};
    constexpr glm::vec3 hingeRight{0.22f, 0.0f, -0.15f};
    constexpr glm::vec3 topLeft{-0.18f, 0.35f, -0.21f};
    constexpr glm::vec3 topRight{0.18f, 0.35f, -0.21f};
    addQuad(mesh, frontLeft, frontRight, hingeRight, hingeLeft);
    addQuad(mesh, hingeLeft, hingeRight, topRight, topLeft);
    addQuad(mesh, frontRight, frontLeft, frontLeft - glm::vec3{0.0f, 0.035f, 0.025f},
            frontRight - glm::vec3{0.0f, 0.035f, 0.025f});
    addQuad(mesh, topLeft, topRight, {0.16f, 0.33f, -0.24f}, {-0.16f, 0.33f, -0.24f});
}

void drawSeat(glm::vec3 color, const Mesh& detailedSeat, bool detailed) {
    glColor3f(palette::steel.r, palette::steel.g, palette::steel.b);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, -seatHeight, 0.0f);
    glVertex3f(0.0f, -0.006f, -0.035f);
    glEnd();

    if (detailed) {
        drawMesh(detailedSeat, color);
        return;
    }
    glColor3f(color.r, color.g, color.b);
    glBegin(GL_TRIANGLE_STRIP);
    glNormal3f(0.0f, 1.0f, 0.03f);
    glVertex3f(-0.20f, -0.01f, 0.20f);
    glVertex3f( 0.20f, -0.01f, 0.20f);
    glVertex3f(-0.22f, 0.0f, -0.15f);
    glVertex3f( 0.22f, 0.0f, -0.15f);
    glNormal3f(0.0f, 0.17f, 0.985f);
    glVertex3f(-0.19f, 0.35f, -0.21f);
    glVertex3f( 0.19f, 0.35f, -0.21f);
    glEnd();
}

void buildRow(SeatingRow& row, const StadiumLayout& layout, const SeatingSection& section,
              int rowIndex, int sector, citylab::Context& ctx) {
    const auto& tier = layout.tiers[section.tier];
    const auto radii = tier.firstRowRadii + glm::vec2{rowIndex * tier.rowDepth};
    const float height = tier.firstRowHeight + rowIndex * tier.risePerRow;
    const float tread = height - seatHeight;
    const auto center = ovalPoint(radii, (sector + 0.5f) * glm::half_pi<float>());
    const glm::vec3 origin{center.x, height, center.y};
    row.origin = origin - section.origin;
    for (const auto& seat : layout.seats) {
        if (seat.tier == section.tier && seat.sector == sector && seat.row == rowIndex) {
            auto local = seat;
            local.position -= origin;
            row.seats.push_back(local);
        }
    }

    const auto inner = radii - glm::vec2{tier.rowDepth * 0.5f};
    const auto outer = radii + glm::vec2{tier.rowDepth * 0.5f};
    for (const auto& span : seatingSpans(layout, sector, radii, tier.rowDepth)) {
        if (span.end <= span.start) continue;
        const int segments = static_cast<int>(std::ceil((span.end - span.start) / 0.18f));
        for (int i = 0; i < segments; ++i) {
            const float a = span.start + (span.end - span.start) * i / segments;
            const float b = span.start + (span.end - span.start) * (i + 1) / segments;
            const std::array<glm::vec2, 4> outline{
                ovalPoint(outer, a), ovalPoint(inner, a), ovalPoint(inner, b), ovalPoint(outer, b)};
            registerTread(ctx, layout, outline, tread);
            const auto vertex = [&](int corner, float y) {
                const auto p = outline[corner];
                return glm::vec3{p.x, y, p.y} - origin;
            };
            addQuad(row.tread, vertex(0, tread), vertex(1, tread), vertex(2, tread), vertex(3, tread));
            addQuad(row.tread, vertex(1, tread), vertex(1, 0.0f), vertex(2, 0.0f), vertex(2, tread));
            if (rowIndex == tier.rows - 1) {
                addQuad(row.tread, vertex(3, tread), vertex(3, 0.0f), vertex(0, 0.0f), vertex(0, tread));
            }
            if (i == 0) {
                addQuad(row.tread, vertex(0, tread), vertex(0, 0.0f), vertex(1, 0.0f), vertex(1, tread));
            }
            if (i == segments - 1) {
                addQuad(row.tread, vertex(2, tread), vertex(2, 0.0f), vertex(3, 0.0f), vertex(3, tread));
            }
        }
    }
}
// The stair takes one side of the 4 m aisle, leaving a continuous ground-level lane.
void buildStairs(SeatingBowl& bowl, const StadiumLayout& layout, citylab::Context& ctx) {
    constexpr float stairSide = 0.25f;
    constexpr float firstDistance = -1.25f;
    const auto inner = layout.innerSize * 0.5f;
    for (int aisle = 0; aisle < seatingSectors; ++aisle) {
        const auto corridor = layout.spectatorCorridors[aisle].size;
        const float stairOuterEdge = std::min(corridor.x, corridor.y) * 0.5f + stairOverlap;
        const float angle = aisle * glm::half_pi<float>();
        const glm::vec2 outward{std::round(std::cos(angle)), std::round(std::sin(angle))};
        const glm::vec2 across{-outward.y, outward.x};
        const auto foot = ovalPoint(inner, angle);
        const auto point = [&](float depth, float side) {
            return foot + outward * depth + across * side;
        };
        const auto railPoint = [&](float depth, float y, float side) {
            const auto p = point(depth, side);
            return glm::vec3{p.x, y, p.y};
        };
        float start = firstDistance;
        float base = 0.0f;
        for (const auto& tier : layout.tiers) {
            for (int row = 0; row < tier.rows; ++row) {
                const float landing = tier.firstRowRadii.x - inner.x + (row + 0.5f) * tier.rowDepth;
                const float height = tier.firstRowHeight + row * tier.risePerRow - seatHeight;
                constexpr float maxRiser = 0.30f;
                const int count = static_cast<int>(std::ceil((height - base) / maxRiser));
                const float run = (landing - start) / count;
                const float rise = (height - base) / count;
                for (int step = 0; step < count; ++step) {
                    const float a = start + run * step, b = a + run;
                    const float top = base + rise * (step + 1);
                    const std::array<glm::vec2, 4> outline{
                        point(a, stairSide), point(a, stairOuterEdge),
                        point(b, stairOuterEdge), point(b, stairSide)};
                    addWedge(bowl.stairs, outline, 0.0f, top);
                    registerTread(ctx, layout, outline, top);
                }
                addQuad(bowl.rails, railPoint(start, base + 0.87f, stairSide),
                        railPoint(landing, height + 0.87f, stairSide),
                        railPoint(landing, height + 0.94f, stairSide),
                        railPoint(start, base + 0.94f, stairSide));
                addQuad(bowl.rails, railPoint(landing - 0.06f, height, stairSide),
                        railPoint(landing, height, stairSide),
                        railPoint(landing, height + 0.9f, stairSide),
                        railPoint(landing - 0.06f, height + 0.9f, stairSide));
                start = landing;
                base = height;
            }
        }
    }
}

}  // namespace

SeatingBowl makeSeatingBowl(const StadiumLayout& layout, citylab::Context& ctx) {
    SeatingBowl bowl;

    for (int tier = 0; tier < static_cast<int>(layout.tiers.size()); ++tier) {
        for (int sector = 0; sector < seatingSectors; ++sector) {
            const auto center = ovalPoint(layout.tiers[tier].firstRowRadii,
                                          (sector + 0.5f) * glm::half_pi<float>());
            SeatingSection section{{center.x, 0.0f, center.y}, tier, {}};
            for (int row = 0; row < layout.tiers[tier].rows; ++row) {
                buildRow(section.rows.emplace_back(), layout, section, row, sector, ctx);
            }
            bowl.sections.push_back(std::move(section));
        }
    }
    buildStairs(bowl, layout, ctx);
    std::size_t triangles = bowl.stairs.size() + bowl.rails.size();
    for (const auto& section : bowl.sections) {
        for (const auto& row : section.rows) triangles += row.tread.size();
    }
    const std::size_t compactCost = 3 * triangles + compactSeatVertices * layout.seats.size();
    assert(compactCost <= vertexBudget::seating);
    if constexpr (useDetailedSeats) {
        buildDetailedSeat(bowl.detailedSeat);
        const std::size_t extraPerSeat = 3 * bowl.detailedSeat.size() - 12;
        if (compactCost < vertexBudget::seating) {
            bowl.detailedSeatCount = std::min(layout.seats.size(),
                (vertexBudget::seating - compactCost) / extraPerSeat);
        }
    }
    return bowl;
}

void drawSeatingBowl(const SeatingBowl& bowl) {
    drawMesh(bowl.stairs, palette::concrete);
    drawMesh(bowl.rails, palette::steel);
    glPushAttrib(GL_LINE_BIT);
    glLineWidth(2.0f);
    for (const auto& section : bowl.sections) {
        const ScopedMatrix sectionFrame;
        glTranslatef(section.origin.x, section.origin.y, section.origin.z);
        const auto treadColor = section.tier == 0 ? glm::vec3{0.38f, 0.38f, 0.40f} : glm::vec3{0.44f, 0.42f, 0.44f};
        for (const auto& row : section.rows) {
            const ScopedMatrix rowFrame;
            glTranslatef(row.origin.x, row.origin.y, row.origin.z);
            drawMesh(row.tread, treadColor);
            for (const auto& seat : row.seats) {
                const ScopedMatrix seatFrame;
                glTranslatef(seat.position.x, seat.position.y, seat.position.z);
                glRotatef(seat.yawDegrees, 0.0f, 1.0f, 0.0f);
                constexpr glm::vec3 red{0.70f, 0.035f, 0.09f};
                constexpr glm::vec3 darkRed{0.48f, 0.025f, 0.07f};
                const bool whiteBand = seatPattern == SeatPattern::WhiteBands && seat.tier == 1 && seat.row == 1;
                const auto color = whiteBand ? palette::facadeWhite : (seat.row == 0 ? darkRed : red);
                drawSeat(color, bowl.detailedSeat, static_cast<std::size_t>(seat.id) < bowl.detailedSeatCount);
            }
        }
    }
    glPopAttrib();
}

}  // namespace team12
