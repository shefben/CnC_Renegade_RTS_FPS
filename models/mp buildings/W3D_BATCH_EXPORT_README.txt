MULTIPLAYER BUILDING GMAX -> W3D BATCH EXPORT
==============================================

Files
-----
BatchExportMPBuildings.ms
  GMax 1.2 MaxScript that loads each multiplayer-building .gmax scene, selects
  its required W3D preset, and exports a same-directory .w3d file.

..\GMAX_W3D_EXPORT_GUIDE.txt
  Source of truth for the per-file export type. The script reads the guide at
  runtime; the 183 mp buildings entries are not duplicated in the script.

Code\Tools\max2w3d\w3dexp.cpp
  Adds the preset filename marker and unattended error capture.

Code\Tools\max2w3d\AppData.cpp
  Adds wwGetLastExportError to the GMax W3D exporter plugin.


Why the exporter plugin needs a small change
--------------------------------------------
The stock Westwood Export.ms script proves that unattended export is invoked
with:

  exportFile output_filename #noPrompt

However, #noPrompt does not accept an export type. The exporter reads a binary
W3dExportOptionsStruct stored as scene AppData. If a scene has no saved export
settings, the default is hierarchy + animation + geometry. Therefore a stock
MaxScript can batch export scenes only with each scene's previously saved
settings; it cannot reliably choose Renegade Terrain versus Hierarchical Model
versus Hierarchical Animated Model.

GMax also disables the MAXScript primitives that would let a script push new
export options into the scene, so the preset has to travel through the one
argument #noPrompt does accept: the output filename.

1. Preset selection by filename marker
   The script asks for an output file whose root name ends in a marker:

     mgpwr_ext.__mpb_terrain.w3d
     mncon_lt_c.__mpb_hierarchical_model.w3d
     mgagd_doors.__mpb_hierarchical_animated.w3d

   Markers also exist for pure_animation, skeleton, and simple_mesh. On a
   suppressed-prompt export the plugin strips the marker, applies the matching
   preset to the scene's export options, refreshes the animation start/end
   frames from the current scene, and writes the normal unmarked .w3d name.
   The marker is ignored for interactive exports, so the dialog is unaffected.
   The script never saves the changed .gmax source.

2. Unattended error capture
   A modal MessageBox in an unattended export stops the whole batch until
   somebody clicks it, which is what made long runs appear to hang. During a
   suppressed-prompt export the plugin now routes every error into a buffer
   instead of showing a dialog, and the batch script reads it back with:

     wwGetLastExportError()

   It returns undefined when the export was clean, or the collected error text
   otherwise. The plugin also deletes its own output file when the export
   fails, so a leftover empty .w3d can never be mistaken for a success.


Build and installation
----------------------
1. Build Code\Tools\max2w3d\max2w3d.dsp with the configuration:

     max2w3d - Win32 GMax Release

   This is the legacy Visual C++ 6 project/configuration for GMax. It requires
   the matching GMax 1.2/3ds Max 4-era SDK and libraries expected by the project.

2. The configured output is GMax_Release\GMax2w3d.dle.

3. Close GMax, back up the currently installed GMax2w3d.dle, then replace it
   with the rebuilt plugin in the same GMax plugin directory.

4. Start GMax and confirm that the normal W3D exporter loads. In the MaxScript
   listener, this command should return undefined rather than an
   unknown-function error:

     wwGetLastExportError()

   The batch script performs this same check before it starts and refuses to
   run against a stock plugin.


Running the script
------------------
1. Open BatchExportMPBuildings.ms and verify MPB_MODELS_ROOT at the top. It is
   currently configured as:

     F:\development\steam\emulator_bot\CnC_Renegade_new\models\

2. In GMax 1.2, choose MaxScript -> Run Script and select
   BatchExportMPBuildings.ms.

3. Confirm the prompt. The script will:

   - confirm that the rebuilt plugin is installed and stop if it is not;
   - read all mp buildings entries from GMAX_W3D_EXPORT_GUIDE.txt;
   - skip buildings-setup.gmax and flying_buildings_setup.gmax;
   - load each remaining source scene directly;
   - verify loading from maxFilePath + maxFileName rather than trusting the
     unreliable GMax 1.2 loadMaxFile return value;
   - export a .w3d beside the source .gmax with #noPrompt, requesting the
     required preset through the output filename marker;
   - treat a scene as exported only when exportFile raised no exception,
     wwGetLastExportError() returned undefined, and the output file is
     non-empty;
   - restore the scene that was open before the batch began; and
   - write progress to the MAXScript Listener and retain the lines in the
     global MPB_LOG_LINES array, ending with a list of every failed scene.

   GMax deliberately disables MaxScript file-output functions such as
   createFile and deleteFile, so the script cannot create a separate text log.
   Use MaxScript -> MAXScript Listener to review progress. Run MPB_showLog() in
   the Listener to print the most recent batch log again.

   If the batch starts with an untitled scene, the final exported scene remains
   open. This avoids resetMaxFile #noPrompt, which can raise an unknown-system
   exception in GMax 1.2. A previously saved scene is restored normally.

Current expected workload
-------------------------
183 guide entries total:

  88  Renegade Terrain
  88  Hierarchical Animated Model
   6  Hierarchical Model
   1  DO NOT EXPORT setup scene

With MPB_EXPORT_AUTHORING_HELPERS=true, the script attempts 182 W3D exports.
buildings-setup.gmax is not compiled at all. flying_buildings_setup.gmax is
included as Hierarchical Model; it is an authoring scene, so do not add its
output to the runtime building component list.

It is deliberately not animated. Animation export allocates a matrix per bone
per frame, and these assembly scenes carry a timeline far longer than any real
animation, so exporting them with animation never finishes. The exporter now
refuses any unattended animation export longer than 10000 frames and reports
the range rather than hanging the batch.
VIS, VISX, TEMP, TMP, and blocker helper files are included because the request
is to compile every exportable .gmax scene. Set the option to false if only
normal runtime components should be generated.


Safety/settings
---------------
MPB_OVERWRITE_EXISTING=true
  Allows the W3D exporter to replace the exact same-directory .w3d target.
  The exporter opens its output in write mode; no MaxScript deleteFile call is
  used because GMax disables that function. Set false to skip existing outputs.

MPB_EXPORT_AUTHORING_HELPERS=true
  Exports VIS/TEMP/blocker helper scenes. Set false to skip them.

MPB_DRY_RUN=false
  Set true to parse and log the complete job list without loading scenes,
  deleting outputs, or invoking the exporter.

The batch never saves over a source .gmax file.


Important limitations
---------------------
- This script requires the rebuilt GMax2w3d.dle. The stock binary cannot choose
  a different W3D preset programmatically, and it stops an unattended batch on
  the first bad scene by raising a modal error dialog.
- GMax must be able to resolve all source textures and Westwood material/plugin
  classes. A missing dependency can still make an individual export fail. Such
  a scene is now logged and skipped instead of halting the run; the reported
  reason comes straight from the exporter.
- The script must run inside GMax 1.2; .gmax is a proprietary scene format and
  cannot be compiled correctly by an external file converter.
- Review the MAXScript Listener after every run. A successful batch has zero
  missing and zero failed entries. MPB_showLog() reprints the latest results.
