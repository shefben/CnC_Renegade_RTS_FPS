"""Where each of the 25 mandatory Zero Hour systems actually lives in the donor.

Roadmap Section 3 names 25 macro-systems and Section 13.3 says the capability
matrix must map all of them to real `Core`/`GeneralsMD` sources.  The mapping
has to come from the donor rather than from memory of the EA-era layout,
because it has moved: at the pinned SHA most of the render and terrain systems
sit in shared `Core/GameEngineDevice`, with `GeneralsMD` holding the Zero
Hour-specific ones.  Writing that down by hand would be wrong within a week.

So each system is described here by the file names that implement it, and the
tool resolves those names against the checked-out donor.  A system that
resolves to nothing is an error, which is what keeps Section 13.3's "no
relevant Zero Hour engine area may remain UNREVIEWED" honest: the list cannot
quietly go stale.

Writes `docs/zerohour/ZeroHourCapabilitySources.tsv`.
"""

import io
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
DONOR = os.path.join(ROOT, 'GeneralsGameCode')
OUT = os.path.join(ROOT, 'docs', 'zerohour', 'ZeroHourCapabilitySources.tsv')

#	Only these two trees.  `Generals/` is not a donor stage and never enters
#	the inventory; see ZeroHourDonorBaseline.md.
TREES = ('Core', 'GeneralsMD')

#	(id, roadmap item, title, [basename patterns])
#
#	Patterns match the file's base name, anchored at both ends, case
#	insensitively.  They are deliberately narrow: a pattern that swept in
#	everything would make the count meaningless.
SYSTEMS = [
    ('ZH-01', 1, 'Asset exclusion / residency', [
        r'W3DAssetManager\..*', r'W3DAssetManagerExposed\..*', r'assetcull\..*',
        r'ArchiveFile.*', r'GameMemory\..*']),
    ('ZH-02', 2, 'Sectional bridge renderer', [
        r'W3DBridgeBuffer\..*', r'Bridge.*', r'TerrainRoads\..*']),
    ('ZH-03', 3, 'Runtime road renderer', [
        r'W3DRoadBuffer\..*', r'W3DBibBuffer\..*']),
    ('ZH-04', 4, 'Heightmap / terrain framework', [
        r'BaseHeightMap\..*', r'HeightMap\..*', r'FlatHeightMap\..*',
        r'WorldHeightMap\..*', r'W3DTerrainVisual\..*', r'TerrainVisual\..*',
        r'TerrainLogic\..*', r'W3DTerrainLogic\..*']),
    ('ZH-05', 5, 'Terrain texture system', [
        r'TerrainTex\..*', r'TileData\..*', r'TerrainTypes\..*']),
    ('ZH-06', 6, 'Tree / foliage buffer', [
        r'W3DTreeBuffer\..*', r'W3DTreeDraw\..*', r'W3DPropBuffer\..*',
        r'W3DPropDraw\..*']),
    ('ZH-07', 7, 'Reflective river / water', [
        r'W3DWater\..*', r'W3DWaterTracks\..*', r'Water.*Options.*',
        r'WaterTracksObj\..*']),
    ('ZH-08', 8, 'Projected / cached shadows', [
        r'W3DShadow\..*', r'W3DProjectedShadow\..*', r'W3DBufferManager\..*',
        r'W3DVolumetricShadow\..*', r'Shadow\..*']),
    ('ZH-09', 9, 'Terrain tracks / surface ribbons', [
        r'W3DTerrainTracks\..*', r'W3DScorch\..*']),
    ('ZH-10', 10, 'Particle batching', [
        r'W3DParticleSys\..*', r'ParticleSys\..*', r'ParticleSysInfo\..*',
        r'ParticleSysManager\..*', r'ParticleSysTemplate\..*']),
    ('ZH-11', 11, 'Dynamic-light filtering', [
        r'W3DDynamicLight\..*']),
    ('ZH-12', 12, 'Tracer / beam / projectile render modules', [
        r'W3DTracerDraw\..*', r'W3DLaserDraw\..*',
        r'W3DProjectileStreamDraw\..*', r'W3DRopeDraw\..*']),
    ('ZH-13', 13, 'Debris rendering', [
        r'W3DDebrisDraw\..*', r'Debris\..*']),
    ('ZH-14', 14, 'Shader-manager architecture', [
        r'W3DShaderManager\..*', r'.*Shader\.h', r'.*Shader\.cpp']),
    ('ZH-15', 15, 'RTS shroud / fog-of-war', [
        r'W3DShroud\..*', r'PartitionManager\..*', r'ShroudUpdate\..*']),
    ('ZH-16', 16, 'Radar system', [
        r'W3DRadar\..*', r'Radar\..*', r'RadarUpgrade\..*']),
    ('ZH-17', 17, 'Waypoint / status-circle renderers', [
        r'W3dWaypointBuffer\..*', r'W3DStatusCircle\..*']),
    ('ZH-18', 18, 'Modular draw-component architecture', [
        r'W3DModelDraw\..*', r'W3DDefaultDraw\..*', r'W3DDependencyModelDraw\..*',
        r'W3DScienceModelDraw\..*', r'W3DOverlord.*Draw\..*',
        r'W3DTankDraw\..*', r'W3DTankTruckDraw\..*', r'W3DTruckDraw\..*',
        r'W3DPoliceCarDraw\..*', r'W3DSupplyDraw\..*', r'Draw\.h',
        r'DrawModule\..*', r'Drawable\..*', r'DrawableInfo\..*']),
    ('ZH-19', 19, 'Spatial partitioning', [
        r'PartitionManager\..*', r'W3DScene\..*', r'CullSystem\..*',
        r'AABTree.*', r'.*Culling.*']),
    ('ZH-20', 20, 'Debug rendering tools', [
        r'W3DDebugIcons\..*', r'W3DDebugDisplay\..*', r'DebugDisplay\..*',
        r'GraphDraw\..*', r'W3DProfilerFrameCapture\..*']),
    ('ZH-21', 21, 'Far/background terrain LOD', [
        r'W3DTerrainBackground\..*']),
    ('ZH-22', 22, 'Surface smudge / decal manager', [
        r'W3DSmudge\..*', r'.*DecalSystem.*', r'.*Decal\..*']),
    ('ZH-23', 23, 'Weather / environment particles', [
        r'W3DSnow\..*', r'Snow\..*', r'Weather\..*']),
    ('ZH-24', 24, 'AI state machine / pathfinding / guard', [
        r'AI\..*', r'AIPathfind\..*', r'AIStateMachine\..*', r'AIGuard\..*',
        r'AIGuardRetaliate\..*', r'AIPlayer\..*', r'StateMachine\..*',
        r'AITNGuard\..*', r'AIStates\..*', r'AISkirmishPlayer\..*']),
    ('ZH-25', 25, 'RTS telemetry / statistics instrumentation', [
        r'AcademyStats\..*', r'ScoreKeeper\..*', r'StatsCollector\..*',
        r'Statistics\..*']),
]

SOURCE_EXT = ('.cpp', '.c', '.h', '.hpp', '.inl')


def donor_files():
    """Every source file in the two donor trees, as donor-relative paths."""
    found = []
    for tree in TREES:
        base = os.path.join(DONOR, tree)
        if not os.path.isdir(base):
            raise SystemExit('donor tree missing: %s' % base)
        for walk_root, dirs, files in os.walk(base):
            dirs[:] = [d for d in dirs if d != '.git']
            for name in files:
                if name.lower().endswith(SOURCE_EXT):
                    full = os.path.join(walk_root, name)
                    found.append(os.path.relpath(full, DONOR).replace('\\', '/'))
    return sorted(found)


def main():
    files = donor_files()
    if not files:
        raise SystemExit('donor submodule is not checked out')

    by_base = {}
    for path in files:
        by_base.setdefault(os.path.basename(path).lower(), []).append(path)

    rows = []
    empty = []
    for ident, item, title, patterns in SYSTEMS:
        matched = set()
        for pattern in patterns:
            compiled = re.compile(pattern + '$', re.IGNORECASE)
            for base, paths in by_base.items():
                if compiled.match(base):
                    matched.update(paths)
        if not matched:
            empty.append('%s %s' % (ident, title))
            continue
        core = sorted(p for p in matched if p.startswith('Core/'))
        zh = sorted(p for p in matched if p.startswith('GeneralsMD/'))
        rows.append((ident, item, title, core, zh))

    if empty:
        raise SystemExit('these systems resolved to no donor source, so the '
                'patterns are stale: %s' % '; '.join(empty))

    directory = os.path.dirname(OUT)
    if not os.path.isdir(directory):
        os.makedirs(directory)
    with io.open(OUT, 'w', encoding='utf-8', newline='') as handle:
        handle.write('ID\tRoadmapItem\tSystem\tShared\tCoreFiles\tGeneralsMDFiles'
                '\tCoreSources\tGeneralsMDSources\n')
        for ident, item, title, core, zh in rows:
            if core and zh:
                shared = 'Core+GeneralsMD'
            elif core:
                shared = 'Core'
            else:
                shared = 'GeneralsMD'
            handle.write('%s\t%d\t%s\t%s\t%d\t%d\t%s\t%s\n'
                    % (ident, item, title, shared, len(core), len(zh),
                       ';'.join(core), ';'.join(zh)))

    core_only = sum(1 for row in rows if row[3] and not row[4])
    zh_only = sum(1 for row in rows if row[4] and not row[3])
    both = len(rows) - core_only - zh_only
    sys.stdout.write('%s: %d systems mapped (%d Core only, %d GeneralsMD only, '
            '%d both), %d donor files scanned\n'
            % (os.path.relpath(OUT, ROOT).replace('\\', '/'), len(rows),
               core_only, zh_only, both, len(files)))


if __name__ == '__main__':
    main()
