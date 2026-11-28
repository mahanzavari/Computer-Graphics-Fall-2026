# team12 stadium architecture and checklist

The updated Allianz Arena plan governs team12. The official Phase 1 PDF and original CityLab SDK take precedence. Only the team directory is editable. CMake discovers sources under `src/`. `MyZone.cpp` registers one zone. Its `init(Context&)` reads plot dimensions, builds the shared layout and precomputes meshes and placement data. The const draw callback renders grounds and auxiliary buildings in plot coordinates, then applies the stadium transform once for the bowl, roof, shell, and pitch.

## Shared geometry contract

Plot origin is at ground center. +Y is up, +Z faces the street. The SDK reports 160 × 100 m for SP-1 + SP-2, with a 10 m gate. The first 6 m inside the gate remain clear. Dimensions below describe the confirmed plot in metres.

| Element | Center or reference | Dimensions |
| --- | --- | --- |
| Stadium | Plot (-18, -5), yaw 0° | 112 × 76 outer ellipse, 96 × 60 inner ellipse. Axes along X and Z |
| Shell and roof | Stadium origin | Plinth band to y = 4.4. Shell spring y = 20. Roof ridge y = 24, with an 82 × 51 rounded-rectangle opening |
| Pitch | Stadium (0, 0), yaw 0° | 70 × 36. Goal lines at x = ±35 and sidelines at z = ±18 |
| Goal and sideline reservations | Stadium-local | 3 m behind each goal line and 10 m wide around z = 0. 2 m runoff beside both sidelines within the ends of the pitch |
| Lower tier | First row radii (48.55, 30.55) | Four rows at 0.82 radial spacing. Seat surface y = 1.00, 1.43, 1.86, 2.29. Treads y = 0.58, 1.01, 1.44, 1.87 |
| Upper tier | First row radii (52.25, 34.25) | Three rows at 0.82 radial spacing. Seat surface y = 4.00, 4.44, 4.88. Treads y = 3.58, 4.02, 4.46 |
| Spectator portals | Four cardinal points of the outer ellipse | 4.8 m nominal opening width. Main +Z portal 4 m high, others 3.5 m. Interior aisles 4 m wide. Stair runs occupy one side |
| Player tunnel | Stadium z = -18, plot z = -23 | 3 m wide, 3.2 m high shell opening. Reserved stadium x = 36 to about 49.32. Exterior route continues to plot x = 47 |
| Player facilities | Plot (59, -23) | 24 × 18. Public +Z approach and west-side player approach |
| Ticket office | Plot (59, 3) | 22 × 12. Public +Z approach |
| Fan shop | Plot (59, 25) | 24 × 16. Public +Z approach |
| Football monument | Plot (15, 40) | 5 × 5 reservation beside the gate approach |

The 76 × 44 m pitch proposed in the plan crosses the chosen inner ellipse at its corners. The 70 × 36 m pitch fits the goal and sideline reservations above. Actual goals, nets, technical areas, tunnel finishing, and door colliders must be checked when Mahan integrates his geometry. The eastern 4 m spine joins the entrance plaza and all public approaches. The shell portal and clear bowl opening are available for the player route.

## Seat placement and ownership

Four aisle cuts and the player tunnel define the usable ellipse spans for both the concrete treads and the **3,116** shared seat placements. Seat counts come from the measured arc length of each span at 0.55 m nominal spacing. Each record carries a deterministic ID, stadium-local seat-surface center, inward yaw, zero-based tier/row/sector, and occupancy eligibility. Rendering retains the Stadium, Section, Row, and Seat transforms. Spectator rendering can reuse these placements and must cap occupancy using the separate 10,000-vertex allocation.

| Owner | Module | Working cap | Stage vertices |
| --- | --- | ---: | ---: |
| Amirabbas | Shell and façade | 14,000 | 13,752 |
| Amirabbas | Roof, supports, interior banners and their lights | 7,000 | 6,690 |
| Amirabbas | Seating, stairs and railings | 50,960 | 50,374 |
| Amirabbas | Grounds and monument | 8,000 | 7,836 |
| Mahan | Pitch, goals and equipment, including scoreboard | 6,000 | 3,796 |
| Mahan | Spectators and flags | 10,000 | Pending |
| Mahan | Player facilities and tunnel | 8,000 | Placeholder |
| Mahan | Ticket office and fan shop | 7,000 | Placeholder |

The seating revision baseline was **89,414** checker-counted vertices and 379 of 400 colliders. The compact seat submits eight vertices per instance, which CityLab counts as 14 after triangulating the strip. The optional beveled mesh and white-band color pattern are configured in Seating.h. Detailed seats are capped by the remaining bowl budget.

The interior decor revision brings the scene to **97,676** checker-counted vertices with the compact seats. It adds 1,500 for pitchside LEDs, 1,296 for wall ribbons, 1,866 for banners, 1,020 for flags, 960 for roof-mounted lamp assemblies, and 1,728 for steel trusses and the connecting ring. The original 108-vertex visible lamp was replaced by the roof assemblies. CityLab reported the same day and night light counts. The 2,324-vertex margin is shared by future crowd and facility work.

## Implementation checklist and measured checkpoints

- [x] Bootstrap. Official template 180 vertices, revised preview 468. SDK lifecycle, double plot, and baseline setup verified.
- [x] A1. Closed oval shell segments, 192 staggered cushions, structural portal jambs, and five ground-level openings. Shell checkpoint 13,284 vertices in the scene. Street view inspected. Final cushions use 12 noncoplanar polygon faces, represented by 16 triangles per cushion. CPU edge-pair checks passed for the cushions.
- [x] A2. Closed annular roof, columns, inclined nonuniformly scaled rafters, open pitch view, and entrance canopy hinge. Roof checkpoint 17,424 vertices. Aerial and interior views inspected. The canopy uses explicit T(p) Rₓ(12°) T(-p), with p = (0, 4.8, 37.6) in stadium coordinates.
- [x] A3. Two distinct tiers using closed handmade tread wedges, 520 composite seats, aisles with stairs and handrails, and section/row/seat transforms. Seating checkpoint 36,516 vertices. Interior capture inspected. CPU checks verified matched directed edges on the tread wedges and reconstructed all seat positions.
- [x] A4 stage. Full plot ground, plaza and path reservations, planting beds, trees, lamps, benches, bollards, and the street-visible football monument. Landscaping checkpoint 44,460 vertices. Aerial, street and monument views inspected. The monument has five separately drawn pedestal/trim pieces plus a black-pentagon and white-hexagon ball, and exceeds 4 m. Lamps, benches, trees, and bollards each have at least five placed instances.
- [x] Integrated Amirabbas stage. CityLab build, lint, and check passed in the isolated build directory. Final check 44,652 scene vertices, zero checker failures or warnings. The CPU walk used CityLab collision against the current zone and reached the stadium interior, player tunnel, and all three auxiliary sites. It also traversed both reserved sideline and goal boundaries and reached the upper stair landing at y = 4.50 m. The facilities are currently pads.
- [x] Interior revision. Red molded seats with grouped white accents, increased roof coverage, and curved FC BAYERN and MIA SAN MIA wall banners. The 64-segment opening leaves about 2.31 m at the pitch corner and 0.77 m at the sideline runoff corner in plan view. CityLab check and lint passed at 46,734 scene vertices. Visual reference is the [official Allianz Arena interior gallery](https://allianz-arena.com/de/galleries/2018/bauarbeiten-sommer-2018_3).
- [x] M1 pitch. Striped grass, touchlines and goal lines, center line/circle/spot, penalty and goal areas, penalty spots and arcs, corner arcs, and two mirrored goals with fixed grid nets. The goal post center spacing is 5.8 m and rear depth is 2.4 m. Check and lint passed at 48,850 scene vertices. Inspected the close-up goal and overview.
- [x] Night lighting. Red and white facade cushions gain emission as CityLab daylight fades. One roof-mounted spotlight (GL_LIGHT1) illuminates the pitch and goals only. Day and night captures inspected. Official check and lint passed at 48,958 vertices, with zero extra lights by day and one at night.
- [x] Scoreboard and banner lighting. The static display reads BAYERN 8-2 BARCELONA with a full-time match clock at 90:00. Each banner has three wall-mounted fixtures. GL_LIGHT2 and GL_LIGHT3 are scoped to the banner draw, and GL_LIGHT1 still illuminates the pitch. Night captures inspected. Check and lint passed at 51,100 vertices.
- [ ] M1 equipment. Dugouts and flags.
- [ ] M2. Spectator selection, seated models and static banners.
- [ ] M3. Player facilities, dressing rooms, finished tunnel and locker-door pivot.
- [ ] M4. Ticket office and fan shop with accessible frontages and eight or more components each.
- [ ] Verify complete plot coverage, all four structures, goal and tunnel geometry, final access and collider count, root-transform demonstration, and final visual/rubric evidence after integration.
- [ ] Complete the README report and images, including student IDs, matrix and vertex experiments, contributions and AI use. Pack and submit after final verification.
