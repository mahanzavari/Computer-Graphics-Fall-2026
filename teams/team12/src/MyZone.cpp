#include <citylab/CityLab.h>
#include "Architecture.h"
#include "Facilities.h"
#include "Geometry.h"
#include "Interior.h"
#include "Landscape.h"
#include "Pitch.h"
#include "Roof.h"
#include "Seating.h"

namespace team12 {
namespace {

void enablePitchSpotlight(glm::vec3 position, float nightLight) {
    const GLfloat location[] = {position.x, position.y, position.z, 1.0f};
    const auto aim = glm::normalize(-position);
    const GLfloat direction[] = {aim.x, aim.y, aim.z};
    const GLfloat color[] = {1.55f * nightLight, 1.55f * nightLight, 1.48f * nightLight, 1.0f};
    const GLfloat ambient[] = {0.09f * nightLight, 0.09f * nightLight, 0.09f * nightLight, 1.0f};
    glLightfv(GL_LIGHT1, GL_POSITION, location);
    glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, direction);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, color);
    glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
    glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 70.0f);
    glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 0.65f);
    glEnable(GL_LIGHT1);
}

}  // namespace

class StadiumZone final : public citylab::Zone {
public:
    void init(citylab::Context& ctx) override {
        layout_ = makeStadiumLayout(ctx.plot());
        pitch_ = makePitch(layout_);
        shell_ = makeStadiumShell(layout_, ctx);
        roof_ = makeStadiumRoof(layout_);
        interior_ = makeStadiumInterior(layout_);
        bowl_ = makeSeatingBowl(layout_, ctx);
        grounds_ = makeGrounds(layout_, ctx);
        floodlight_ = {0.0f, layout_.roofHeight - 3.0f, layout_.innerSize.y * 0.45f};
    }

    void draw(const citylab::Context& ctx) const override {
        drawLandscape(layout_, grounds_);
        drawFacilities(layout_);

        const ScopedMatrix stadiumFrame;
        const auto& center = layout_.stadiumCenter;
        glTranslatef(center.x, center.y, center.z);
        glRotatef(layout_.stadiumYawDegrees, 0.0f, 1.0f, 0.0f);
        const float nightLight = 1.0f - glm::smoothstep(0.04f, 0.26f, ctx.frame().daylight);
        drawArchitecture(shell_, nightLight);
        drawStadiumRoof(roof_, nightLight);
        drawSeatingBowl(bowl_);
        drawStadiumInterior(interior_);
        // Pitch is last, so the floodlight affects the field and goals only.
        // CityLab restores the light state when it exits the zone.
        if (nightLight > 0.0f) {
            enablePitchSpotlight(floodlight_, nightLight);
        }
        drawPitch(layout_, pitch_);
    }

private:
    StadiumLayout layout_{};
    PitchScene pitch_;
    StadiumShell shell_;
    StadiumRoof roof_;
    StadiumInterior interior_;
    SeatingBowl bowl_;
    StadiumGrounds grounds_;
    glm::vec3 floodlight_{};
};

}  // namespace team12

CITYLAB_REGISTER_ZONE(team12::StadiumZone);
