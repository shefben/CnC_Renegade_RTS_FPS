"""Colour targets derived from retail Renegade reference textures.

Every stop below cites the `always.dat` reference it was pulled from and,
where the reference was analysed numerically (see
`docs/assets/GeneratedTextureReport.md` for the full table), the measured
mean RGB of that reference. Stops are picked to sit close to the measured
mean while leaving room for the generator's own light/dark variation -- the
goal is "clearly related to the stock texture's colour family", not a
pixel-identical clone (`MissingAssets.md` / art direction brief explicitly
asks for more detail and resolution, not the same pixels).

All RGB tuples are 0-255 int8 sRGB-ish values, hand-sampled against the
extracted reference PNGs (`art/procedural_textures/reference_notes.md`
records which stock file backs which family).
"""

TERRAIN = {
    # cc_grass1 measured mean (91.6, 104.4, 50.8): olive dry-grass green.
    "ground": dict(
        soil=(58, 48, 32),
        dry_grass=(138, 122, 58),
        green_grass=(84, 100, 48),
        stone=(150, 142, 124),
    ),
    # l05_grass measured mean (60.7, 69.1, 33.5): darker, broader green than
    # ground -- macro is deliberately low-detail so only two stops are used.
    "macro": dict(
        dark=(44, 56, 27),
        light=(104, 114, 64),
    ),
    # l01-rock2 measured mean (67.1, 59.5, 49.0): worn grey-brown rock.
    "rock": dict(
        dark=(42, 38, 32),
        mid=(76, 68, 56),
        light=(114, 102, 84),
    ),
    # l03_mossrock measured mean (108.1, 99.6, 81.3) plus l10_cliff1 (tan
    # sedimentary strata, visual match) for banding and a moss-green accent.
    "cliff": dict(
        dark=(56, 50, 42),
        mid=(102, 90, 72),
        light=(152, 132, 98),
        moss=(88, 98, 58),
    ),
    # lv8_sand3 measured mean (192.3, 157.1, 110.1) for the dry end; l03_mud /
    # l03_wetdrt (visual match, near-black wet soil) for the water's-edge end.
    "shore": dict(
        dry=(150, 124, 86),
        wet=(94, 72, 50),
        dark_wet=(56, 42, 30),
    ),
    # l11_gravel measured mean (130.5, 102.9, 77.5); l01-harddirt (visual
    # match, pale hardpan) lightens the mix.
    "road_bed": dict(
        dirt=(108, 88, 64),
        gravel_light=(150, 128, 102),
        gravel_dark=(78, 64, 50),
    ),
}

# 0201tarmak / stp_tarmak (visual match, grey tarmac panels) for asphalt tone;
# cc_road3 (visual match, pale road with painted lines) for marking colour.
ROADS = {
    "highway": dict(
        asphalt=(56, 56, 58),
        asphalt_light=(78, 78, 80),
        line=(198, 184, 122),
        wear=(94, 86, 70),
    ),
    "street": dict(
        asphalt=(62, 60, 56),
        patch=(86, 78, 64),
        oil=(36, 34, 32),
        line=(188, 176, 148),
    ),
    "track": dict(
        dirt=(112, 90, 64),
        dirt_light=(150, 124, 90),
        rut=(68, 52, 38),
    ),
}

# water_texture measured mean (13.7, 44.5, 51.0): dark teal. Other five kinds
# are built as variations around it per WaterModels.md's flow/murk guidance.
WATER = {
    "river": dict(deep=(10, 42, 50), mid=(24, 68, 76), foam=(182, 210, 205)),
    "stream": dict(deep=(18, 60, 66), mid=(42, 92, 92), foam=(202, 222, 212)),
    "lake": dict(deep=(8, 36, 46), mid=(20, 58, 64), foam=(160, 195, 195)),
    "pond": dict(deep=(20, 40, 28), mid=(40, 60, 38), foam=(140, 160, 120)),
    "coast": dict(deep=(10, 46, 58), mid=(34, 86, 94), foam=(212, 226, 216)),
    "ocean": dict(deep=(6, 30, 44), mid=(16, 56, 70), foam=(202, 220, 220)),
}

MARKS = {
    # hnd_pad measured mean (197.3, 192.5, 185.3): pale worn concrete.
    "construction": dict(pad=(150, 144, 132), line=(196, 168, 60), dark=(88, 84, 76)),
    # oil-2 measured mean (5.8, 5.0, 7.1): near-black fluid.
    "stain": dict(oil=(9, 8, 11), rim=(32, 28, 24)),
    "scorch": dict(soot=(18, 16, 14), ring=(70, 54, 40)),
    "blast": dict(center=(20, 17, 15), mid=(68, 54, 40), dirt=(118, 96, 68)),
    "impact": dict(hole=(22, 19, 17), rim=(88, 72, 52)),
    "dirt": dict(loose=(116, 94, 64), dark=(78, 60, 40)),
    "world": dict(mark=(72, 62, 48), edge=(112, 96, 72)),
}

# d_tk_dirt measured mean (113.6, 83.1, 59.0): stock tread-print colour.
RIBBONS = {
    "tank_track": dict(dark=(44, 34, 26), light=(92, 74, 54)),
    "harvester_track": dict(dark=(38, 30, 24), light=(84, 66, 48)),
    "tire_track": dict(dark=(50, 40, 30), light=(96, 78, 58)),
    "scorch_trail": dict(soot=(16, 14, 12), ring=(56, 44, 32)),
    "drag_mark": dict(dark=(52, 42, 32), light=(90, 74, 54)),
}

# tib_bullet01-3 / l01-tib-water1 / tib_scorch01 (visual match, saturated
# glowing green) for Tiberium; WeatherParticles measured mean
# (209, 208.9, 209) for the neutral rain/falling tone.
WEATHER = {
    "rain": dict(core=(222, 226, 232)),
    "dust": dict(tan=(150, 128, 96)),
    "tiberium": dict(green=(58, 214, 110), bright=(160, 255, 176)),
    "falling": dict(grey=(212, 212, 212)),
}

# bark_pine / pine_green (visual match) for conifer; dsp_deadbush (visual
# match, bare grey-white branching) for the dead tree; leaf_yello / cc_grass1
# for grass clump; rock family reused for the boulder so it reads as the
# same geology as ow_ter_rock / ow_ter_cliff.
FOLIAGE = {
    "conifer_bark": dict(dark=(50, 40, 32), light=(96, 82, 64)),
    "conifer_needle": dict(dark=(26, 46, 32), light=(68, 98, 64)),
    "scrub": dict(dark=(56, 64, 32), light=(118, 122, 68), stem=(78, 62, 42)),
    "dead_tree": dict(bark=(92, 86, 76), dark=(48, 44, 38)),
    "grass_clump": dict(dry=(150, 132, 70), green=(94, 106, 52)),
    "boulder": dict(dark=(56, 50, 42), light=(116, 104, 84)),
}
