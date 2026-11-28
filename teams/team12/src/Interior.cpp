#include "Interior.h"

#include <algorithm>
#include <cmath>
#include <string_view>
#include <utility>
#include <glm/gtc/constants.hpp>

#include "Roof.h"

namespace team12 {
namespace {

constexpr glm::vec3 red{0.86f, 0.02f, 0.18f};
constexpr glm::vec3 deepRed{0.60f, 0.0f, 0.13f};
constexpr glm::vec3 white{1.0f, 1.0f, 1.0f};
constexpr glm::vec3 graphite{0.145f, 0.16f, 0.20f};
constexpr glm::vec3 blue{0.0f, 0.40f, 0.70f};

struct Advert {
    std::string_view text;
    glm::vec3 background;
};

constexpr Advert allianz{"ALLIANZ", {0.025f, 0.25f, 0.58f}};
constexpr Advert adidas{"ADIDAS", {0.055f, 0.06f, 0.085f}};
constexpr Advert audi{"AUDI", {0.63f, 0.045f, 0.09f}};
constexpr Advert telekom{"TELEKOM", {0.81f, 0.02f, 0.40f}};
constexpr Advert bayern{"FC BAYERN", deepRed};
constexpr Advert mia{"MIA SAN MIA", {0.41f, 0.018f, 0.093f}};

template<class Point>
void patch(Mesh& mesh, const Point& point, float left, float right, float bottom, float top, float depth) {
    addQuad(mesh, point(left, bottom, depth), point(right, bottom, depth),
            point(right, top, depth), point(left, top, depth));
}

template<class Point>
void lettering(Mesh& mesh, const Point& point, std::string_view text,
               float width, float height, float middle, float depth) {
    const float pixel = width / (6 * text.size() - 1);
    const float rowHeight = height / 7.0f;
    float cursor = -width * 0.5f;
    for (const char letter : text) {
        auto rows = stadiumGlyph(letter);
        for (int y = 0; y < 7; ++y) {
            for (int x = 0; x < 5; ++x) {
                const unsigned bit = 16u >> x;
                if (!(rows[y] & bit)) continue;
                int right = x;
                while (right + 1 < 5 && (rows[y] & (16u >> (right + 1)))) ++right;
                unsigned mask = 0;
                for (int col = x; col <= right; ++col) mask |= 16u >> col;
                int bottom = y;
                while (bottom + 1 < 7 && (rows[bottom + 1] & mask) == mask) ++bottom;
                int vertical = y;
                while (vertical + 1 < 7 && (rows[vertical + 1] & bit)) ++vertical;
                if (vertical - y + 1 > (bottom - y + 1) * (right - x + 1)) {
                    right = x;
                    bottom = vertical;
                    mask = bit;
                }
                for (int row = y; row <= bottom; ++row) rows[row] &= ~mask;
                const float topY = middle + height * 0.5f;
                patch(mesh, point, cursor + x * pixel, cursor + (right + 1) * pixel,
                      topY - (bottom + 1) * rowHeight, topY - y * rowHeight, depth);
            }
        }
        cursor += 6 * pixel;
    }
}

void addPitchBoards(StadiumInterior& interior, const StadiumLayout& layout) {
    struct Run {
        glm::vec2 center;
        glm::vec2 inward;
        float length;
        int modules;
        Advert advert;
    };
    const auto half = layout.pitch.size * 0.5f;
    const float scale = half.x / 35.0f;
    const float touchline = half.y + layout.sidelineClearance + 1.7f * scale;
    const float goalLine = half.x + layout.goalClearance.x + 1.4f * scale;
    const std::array<Run, 8> runs{{
        {{-17, touchline}, {0,-1}, 30, 5, allianz}, {{17, touchline}, {0,-1}, 30, 5, adidas},
        {{-26,-touchline}, {0,1}, 12, 2, bayern}, {{26,-touchline}, {0,1}, 12, 2, mia},
        {{goalLine, 10.5f}, {-1,0}, 12, 2, audi}, {{goalLine,-10}, {-1,0}, 11, 2, telekom},
        {{-goalLine,10.5f}, {1,0}, 12, 2, {"", telekom.background}},
        {{-goalLine,-10.5f}, {1,0}, 12, 2, {"", allianz.background}}
    }};
    for (const auto& run : runs) {
        const auto pitchCenter = layout.pitch.center;
        const glm::vec3 center{pitchCenter.x + run.center.x, 0, pitchCenter.y + run.center.y};
        const glm::vec3 inward{run.inward.x, 0, run.inward.y};
        const glm::vec3 right{inward.z, 0, -inward.x};
        const auto point = [&](float u, float y, float d) {
            return center + right * (u * scale) + glm::vec3{0,y,0} + inward * (d - 0.12f*y);
        };
        Mesh frame, screen, text;
        const float segment = run.length / run.modules;
        // A shared closed chassis carries the individual LED tiles.
        const float a = -run.length * .5f;
        const float b = run.length * .5f;
        patch(frame, point, a, b, 0.02f, 0.84f, 0.0f);
        addQuad(frame, point(b,.02f,-.16f), point(a,.02f,-.16f),
                point(a,.84f,-.16f), point(b,.84f,-.16f));
        addQuad(frame, point(a,.84f,0), point(b,.84f,0), point(b,.84f,-.16f), point(a,.84f,-.16f));
        addQuad(frame, point(a,.02f,0), point(a,.84f,0), point(a,.84f,-.16f), point(a,.02f,-.16f));
        addQuad(frame, point(b,.02f,-.16f), point(b,.84f,-.16f), point(b,.84f,0), point(b,.02f,0));
        addQuad(frame, point(a,.02f,-.16f), point(b,.02f,-.16f), point(b,.02f,0), point(a,.02f,0));
        for (int i = 0; i < run.modules; ++i) {
            const float start = a + i * segment;
            patch(screen, point, start+.06f, start+segment-.06f, .09f, .78f, .01f);
        }
        if (run.advert.text.empty()) {
            patch(text, point, a+.35f, b-.35f, .21f, .27f, .022f);
            patch(text, point, a+.35f, b-.35f, .57f, .63f, .022f);
        } else {
            lettering(text, point, run.advert.text, .58f * run.advert.text.size(), .50f, .44f, .022f);
        }
        interior.pitchBoards.push_back({std::move(frame), graphite});
        interior.pitchBoards.push_back({std::move(screen), run.advert.background, true});
        interior.pitchBoards.push_back({std::move(text), white, true});
    }
}

void addRibbons(StadiumInterior& interior, glm::vec2 outer) {
    constexpr float quarter = glm::half_pi<float>();
    struct Ribbon { float angle; Advert advert; };
    constexpr std::array<Ribbon, 4> ribbons{{
        {0.75f, allianz}, {quarter, bayern}, {glm::pi<float>(), telekom}, {3*quarter, mia}
    }};
    for (const auto& ribbon : ribbons) {
        const auto point = [&](float u, float y, float d) {
            const auto p = ovalPoint(outer - glm::vec2{1.56f+d}, ribbon.angle + u);
            return glm::vec3{p.x, y, p.y};
        };
        Mesh frame, screen, text;
        constexpr int modules = 8;
        for (int i=0; i<modules; ++i) {
            const float a = -.24f + .06f*i;
            patch(frame, point, a, a+.0594f, 7.10f, 7.87f, 0.0f);
            patch(screen, point, a+.001f, a+.0584f, 7.17f, 7.79f, .022f);
            if (i % 2 == 0) {
                addQuad(frame, point(a,7.75f,0), point(a+.008f,7.75f,0),
                        point(a+.008f,7.75f,-1.25f), point(a,7.75f,-1.25f));
            }
        }
        lettering(text, point, ribbon.advert.text, .017f*ribbon.advert.text.size(), .45f, 7.48f, .068f);
        interior.ribbons.push_back({std::move(frame), graphite});
        interior.ribbons.push_back({std::move(screen), ribbon.advert.background, true});
        interior.ribbons.push_back({std::move(text), white, true});
    }
}

enum class FabricPattern { Lettered, Horizontal, Vertical, Checker, Diamonds, Roundel };

struct Fabric {
    float angle;
    float top;
    float width;
    float height;
    float wave;
    FabricPattern pattern;
    std::string_view text{};
};

void addFabric(std::vector<InteriorSurface>& surfaces, glm::vec2 outer, const Fabric& design, bool flag) {
    const float setback = flag ? 2.25f : 1.57f;
    const auto radii = outer - glm::vec2{setback};
    const float metresPerRadian = std::hypot(radii.x * std::sin(design.angle), radii.y * std::cos(design.angle));
    const auto angleAt = [&](float x) { return design.angle + x / metresPerRadian; };
    const auto point = [&](float x, float y, float depth) {
        const float u = x / design.width + 0.5f;
        const float v = -y / design.height;
        const float fold = design.wave * std::sin(3.0f * glm::pi<float>() * u + design.angle) * (flag ? 0.25f + 0.75f*u : 0.3f + 0.7f*v);
        const auto p = ovalPoint(radii - glm::vec2{fold + depth}, angleAt(x));
        const float hem = 0.10f * std::sin(5.0f*u + design.angle) * v;
        return glm::vec3{p.x, design.top + y + hem, p.y};
    };

    const float half = design.width * .5f;
    Mesh primary, alternate, ink, mounts;
    constexpr int columns = 6;
    const int rows = (design.pattern == FabricPattern::Horizontal || design.pattern == FabricPattern::Checker) ? 3
                   : (design.pattern == FabricPattern::Lettered ? 1 : 2);
    for (int x=0; x<columns; ++x) {
        const float a = -half + design.width * x / columns;
        const float b = -half + design.width * (x+1) / columns;
        for (int y=0; y<rows; ++y) {
            const float bottom = -design.height*(y+1)/rows;
            const float top = -design.height*y/rows;
            const bool whiteCell = (design.pattern == FabricPattern::Horizontal && y%2 == 1)
                || (design.pattern == FabricPattern::Vertical && (x/2)%2 == 1)
                || (design.pattern == FabricPattern::Checker && (x/2+y)%2 == 1);
            patch(whiteCell ? alternate : primary, point, a, b, bottom, top, 0);
            if (design.pattern == FabricPattern::Diamonds) {
                const auto middle = (a+b)*.5f;
                const auto midY = (bottom+top)*.5f;
                addQuad(alternate, point(a,midY,.025f), point(middle,bottom,.025f),
                        point(b,midY,.025f), point(middle,top,.025f));
            }
        }
    }
    if (design.pattern == FabricPattern::Roundel) {
        const float r = design.height*.32f;
        for (int i=0; i<12; ++i) {
            const auto a = ovalPoint({r,r}, glm::two_pi<float>()*i/12.0f);
            const auto b = ovalPoint({r,r}, glm::two_pi<float>()*(i+1)/12.0f);
            addTriangle(alternate, point(0,-design.height*.5f,.045f),
                        point(a.x,a.y-design.height*.5f,.045f),
                        point(b.x,b.y-design.height*.5f,.045f));
        }
    }
    if (!design.text.empty()) {
        const float width = design.width * .84f;
        lettering(ink, point, design.text, width,
                  std::min(design.height * .52f, 1.4f * width / design.text.size()),
                  -design.height * .50f, .085f);
    }

    for (const float x : {-half, half}) {
        if (flag && x > 0) continue;
        const auto a = point(x, 0, 0);
        const auto b = ovalPoint(outer - glm::vec2{0.37f}, angleAt(x));
        const glm::vec3 wallAnchor{b.x,a.y+.20f,b.y};
        constexpr glm::vec3 bar{0,.055f,0};
        addQuad(mounts, a, wallAnchor, wallAnchor + bar, a + bar);
    }
    if (flag) {
        patch(mounts, point, -half-.055f, -half+.025f, -design.height-.17f, .13f, .01f);
    } else {
        for (int i=0; i<columns; ++i) {
            const float a = -half + design.width*i/columns;
            const float b = -half + design.width*(i+1)/columns;
            patch(mounts, point, a, b, -.055f, .04f, .019f);
        }
    }
    const auto base = design.pattern == FabricPattern::Diamonds ? blue : (flag ? red : deepRed);
    surfaces.push_back({std::move(primary), base});
    surfaces.push_back({std::move(alternate), white});
    surfaces.push_back({std::move(ink), white});
    surfaces.push_back({std::move(mounts), palette::steel});
}

void addSupporterDecor(StadiumInterior& interior, glm::vec2 outer) {
    using Pattern = FabricPattern;
    constexpr Fabric banners[] = {
        {3.14f, 12.8f, 13.0f, 2.1f, .13f, Pattern::Lettered, "S@DKURVE M@NCHEN"},
        {2.76f, 12.1f, 6.2f, 1.45f, .09f, Pattern::Lettered, "SEIT 1900"},
        {3.48f, 11.4f, 7.7f, 1.65f, .12f, Pattern::Lettered, "ROT UND WEISS"},
        {0.68f, 12.6f, 3.1f, 3.0f, .10f, Pattern::Diamonds},
        {5.63f, 11.9f, 6.7f, 1.4f, .10f, Pattern::Lettered, "NUR DER FCB"},
        {2.88f, 6.34f, 4.9f, .83f, .05f, Pattern::Horizontal}
    };
    constexpr Fabric flags[] = {
        {2.68f, 15.9f, 3.8f, 2.3f, .19f, Pattern::Horizontal},
        {3.04f, 16.5f, 4.5f, 2.4f, .22f, Pattern::Lettered, "FCB"},
        {3.38f, 15.8f, 3.9f, 2.1f, .23f, Pattern::Checker},
        {3.58f, 14.7f, 3.1f, 1.8f, .16f, Pattern::Roundel},
        {0.52f, 17.5f, 3.2f, 2.1f, .19f, Pattern::Vertical},
        {5.75f, 16.8f, 4.0f, 1.9f, .20f, Pattern::Lettered, "MIA SAN MIA"},
        {2.06f, 7.0f, 2.6f, 1.3f, .11f, Pattern::Diamonds}
    };
    for (const auto& banner : banners) addFabric(interior.banners, outer, banner, false);
    for (const auto& flag : flags) addFabric(interior.flags, outer, flag, true);
}

}  // namespace

StadiumInterior makeStadiumInterior(const StadiumLayout& layout) {
    StadiumInterior interior;
    addPitchBoards(interior, layout);
    addRibbons(interior, layout.outerSize * 0.5f);
    addSupporterDecor(interior, layout.outerSize * 0.5f);
    return interior;
}

void drawStadiumInterior(const StadiumInterior& interior) {
    glPushAttrib(GL_ENABLE_BIT);
    for (const auto* group : {&interior.pitchBoards, &interior.ribbons, &interior.banners, &interior.flags}) {
        for (const auto& surface : *group) {
            if (surface.display) glDisable(GL_LIGHTING);
            drawMesh(surface.mesh, surface.color);
            if (surface.display) glEnable(GL_LIGHTING);
        }
    }
    glPopAttrib();
}

}  // namespace team12
