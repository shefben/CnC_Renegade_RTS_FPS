-- Command & Conquer Renegade - multiplayer building batch W3D exporter
-- Target: GMax 1.2 with the repository's GMax2w3d.dle exporter.
--
-- The rebuilt exporter recognizes a .__mpb_<preset>.w3d filename marker on
-- unattended exports. It applies the preset natively and writes the normal,
-- unmarked .w3d filename. MAXScript only calls the W3D export plug-in.
--
-- Export types are read from models\GMAX_W3D_EXPORT_GUIDE.txt so this script
-- has one source of truth instead of maintaining a second 183-file list.
--
-- GMax disables MaxScript file-output functions such as createFile and
-- deleteFile. Progress is written to the MAXScript Listener and retained in
-- MPB_LOG_LINES instead of being written to a separate text log.


-- USER SETTINGS -------------------------------------------------------------------------------

global MPB_MODELS_ROOT = "F:\\development\\steam\\emulator_bot\\CnC_Renegade_new\\models\\"
global MPB_OVERWRITE_EXISTING = true
global MPB_EXPORT_AUTHORING_HELPERS = true
global MPB_DRY_RUN = false


-- INTERNAL PATHS ------------------------------------------------------------------------------

global MPB_SOURCE_ROOT = MPB_MODELS_ROOT + "mp buildings\\"
global MPB_EXPORT_GUIDE = MPB_MODELS_ROOT + "GMAX_W3D_EXPORT_GUIDE.txt"
global MPB_LOG_LINES = #()


fn MPB_fileExists filename =
(
	(getFiles filename).count > 0
)


fn MPB_startsWith value prefix =
(
	if value == undefined or prefix == undefined then
		false
	else if value.count < prefix.count then
		false
	else
		(substring value 1 prefix.count) == prefix
)


fn MPB_toW3DPath sourcePath =
(
	if sourcePath.count < 5 then
		sourcePath + ".w3d"
	else
		replace sourcePath (sourcePath.count - 4) 5 ".w3d"
)


fn MPB_log text =
(
append MPB_LOG_LINES text
format "%\n" text
)


fn MPB_showLog =
(
for line in MPB_LOG_LINES do format "%\n" line
)


fn MPB_parsePreset typeText =
(
	if MPB_startsWith typeText "DO NOT EXPORT" then
		#skip
	else if MPB_startsWith typeText "Hierarchical Animated Model" then
		#hierarchical_animated
	else if MPB_startsWith typeText "Hierarchical Model" then
		#hierarchical_model
	else if MPB_startsWith typeText "Renegade Terrain" then
		#terrain
	else if MPB_startsWith typeText "Pure Animation" then
		#pure_animation
	else if MPB_startsWith typeText "Skeleton" then
		#skeleton
	else if MPB_startsWith typeText "Simple Mesh" then
		#simple_mesh
	else
		#unknown
)


fn MPB_presetArgument preset =
(
	case preset of
	(
		#hierarchical_animated: "hierarchical_animated"
		#hierarchical_model: "hierarchical_model"
		#terrain: "terrain"
		#pure_animation: "pure_animation"
		#skeleton: "skeleton"
		#simple_mesh: "simple_mesh"
		default: undefined
	)
)


fn MPB_toNativePresetPath outputPath preset =
(
	local presetArgument = MPB_presetArgument preset

	if presetArgument == undefined then
		undefined
	else if outputPath.count < 4 then
		outputPath + ".__mpb_" + presetArgument + ".w3d"
	else
		replace outputPath (outputPath.count - 3) 4 \
			(".__mpb_" + presetArgument + ".w3d")
)


fn MPB_isAuthoringHelper typeText =
(
	(findString typeText "authoring/VIS helper" != undefined) or
	(findString typeText "collision/blocker helper" != undefined)
)


-- Each returned job is #(relative_path_below_mp_buildings, preset, full_type_text).
fn MPB_readJobs guidePath =
(
	local jobs = #()
	local guide = openFile guidePath mode:"rt"
	local pathPrefix = "mp buildings\\"

	if guide == undefined then return jobs

	while not eof guide do
	(
		local line = readLine guide

		if line != undefined and MPB_startsWith line pathPrefix do
		(
			local arrow = findString line " -> "

			if arrow != undefined do
			(
				local relativeStart = pathPrefix.count + 1
				local relativeLength = arrow - relativeStart
				local typeStart = arrow + 4
				local typeLength = line.count - typeStart + 1
				local relativePath = substring line relativeStart relativeLength
				local typeText = substring line typeStart typeLength
				local preset = MPB_parsePreset typeText

				append jobs #(relativePath, preset, typeText)
			)
		)
	)

	close guide
	jobs
)


fn MPB_restoreScene originalScene =
(
if originalScene != undefined and originalScene != "" and MPB_fileExists originalScene then
(
 local restoreResult = undefined
 try (restoreResult = loadMaxFile originalScene) catch (restoreResult = undefined)
 ((maxFilePath + maxFileName) == originalScene)
)
else
(
 -- resetMaxFile #noPrompt can raise an unknown-system exception in GMax 1.2.
 -- If the batch started with an untitled scene, safely leave the last export
 -- scene open instead of trying to reset it from MAXScript.
 true
)
)


fn MPB_runBatch =
(
	if not MPB_fileExists MPB_EXPORT_GUIDE then
	(
		messageBox ("Export guide not found:\n\n" + MPB_EXPORT_GUIDE) \
			title:"W3D Batch Export"
		return false
	)

	local jobs = MPB_readJobs MPB_EXPORT_GUIDE
	if jobs.count == 0 then
	(
		messageBox "No mp buildings entries were found in the export guide." \
			title:"W3D Batch Export"
		return false
	)

	if wwGetLastExportError == undefined then
	(
		messageBox ("This batch needs the rebuilt GMax2w3d.dle exporter.\n\n" + \
			"wwGetLastExportError was not found, so the preset filename marker " + \
			"and per-model error capture are unavailable.\n\n" + \
			"Install the rebuilt plugin and restart GMax.") \
			title:"W3D Batch Export"
		return false
	)

	if checkForSave() == false then return false

local originalScene = maxFilePath + maxFileName
local exportedCount = 0
local skippedCount = 0
local failedCount = 0
	local missingCount = 0
local helperCount = 0
local abortBatch = false
local failedJobs = #()

MPB_LOG_LINES = #()
MPB_log "C&C Renegade multiplayer-building W3D batch export"
MPB_log ("Models root: " + MPB_MODELS_ROOT)
MPB_log ("Guide: " + MPB_EXPORT_GUIDE)
MPB_log ("Jobs read: " + jobs.count as string)
MPB_log ("Dry run: " + MPB_DRY_RUN as string)
MPB_log ""

	for job in jobs do
	(
		if abortBatch == false do
		(
			local relativePath = job[1]
			local preset = job[2]
			local typeText = job[3]
			local sourcePath = MPB_SOURCE_ROOT + relativePath
			local outputPath = MPB_toW3DPath sourcePath
			local isHelper = MPB_isAuthoringHelper typeText

			if preset == #skip then
 (
  skippedCount += 1
  MPB_log ("SKIP (guide): " + relativePath)
 )
 else if preset == #unknown then
 (
  failedCount += 1
  MPB_log ("ERROR (unknown export type): " + relativePath + " -> " + typeText)
 )
 else if isHelper and not MPB_EXPORT_AUTHORING_HELPERS then
 (
  helperCount += 1
  MPB_log ("SKIP (authoring helper): " + relativePath)
 )
 else if not MPB_fileExists sourcePath then
 (
  missingCount += 1
  MPB_log ("ERROR (missing source): " + sourcePath)
 )
 else if (not MPB_OVERWRITE_EXISTING) and MPB_fileExists outputPath then
 (
  skippedCount += 1
  MPB_log ("SKIP (output exists): " + outputPath)
 )
 else if MPB_DRY_RUN then
 (
  MPB_log ("DRY RUN: " + relativePath + " -> " + preset as string)
 )
 else
 (
  local loadResult = undefined
  try (loadResult = loadMaxFile sourcePath) catch (loadResult = undefined)

  -- GMax 1.2 can return false/undefined (and sometimes raise a late system
  -- exception) even though the requested .gmax scene was loaded. Verify the
  -- active scene path instead of trusting the loadMaxFile return value.
  local activeScene = maxFilePath + maxFileName
  local loaded = (activeScene == sourcePath)

  if not loaded then
  (
  failedCount += 1
  MPB_log ("ERROR (GMax could not load source): " + sourcePath)
  MPB_log ("  Active scene after load attempt: " + activeScene)
  )
				else
				(
					local exporterOutputPath = MPB_toNativePresetPath outputPath preset
					local exportCalled = false
					local exportException = undefined

					try
						(
							exportFile exporterOutputPath #noPrompt
							exportCalled = true
						)
						catch
						(
							exportCalled = false
							try (exportException = getCurrentException()) \
								catch (exportException = undefined)
						)

					-- The rebuilt exporter captures every export error instead of
					-- raising modal dialogs, and deletes its output on failure.
					local exporterError = undefined
					try (exporterError = wwGetLastExportError()) \
						catch (exporterError = undefined)

					local outputSize = 0
					if MPB_fileExists outputPath do
						try (outputSize = getFileSize outputPath) catch (outputSize = -1)

					if exportCalled and exporterError == undefined and outputSize != 0 then
					(
						exportedCount += 1
						MPB_log ("OK: " + relativePath + " -> " + preset as string)
					)
					else
					(
						failedCount += 1
						append failedJobs relativePath
						MPB_log ("ERROR (W3D export failed): " + outputPath)
						if exportException != undefined do
							MPB_log ("  exportFile exception: " + exportException as string)
						if exporterError != undefined do
							MPB_log ("  Exporter: " + exporterError)
						if exportCalled and exporterError == undefined and outputSize == 0 do
							MPB_log "  Output file missing or empty, but no exporter error was captured."
						if MPB_fileExists exporterOutputPath do
							MPB_log "  The exporter did not consume its native preset marker."
					)
				)
			)
		)
	)

MPB_log ""
MPB_log ("Exported: " + exportedCount as string)
MPB_log ("Skipped: " + skippedCount as string)
MPB_log ("Helpers skipped: " + helperCount as string)
MPB_log ("Missing: " + missingCount as string)
MPB_log ("Failed: " + failedCount as string)

if failedJobs.count > 0 do
(
	MPB_log ""
	MPB_log "Failed scenes:"
	for failedJob in failedJobs do MPB_log ("  " + failedJob)
)

local restoredOriginal = MPB_restoreScene originalScene
if not restoredOriginal do
 MPB_log ("WARNING: Could not restore original scene: " + originalScene)

	messageBox ("W3D batch export complete.\n\nExported: " + exportedCount as string + \
	"\nSkipped: " + skippedCount as string + \
	"\nMissing: " + missingCount as string + \
	"\nFailed: " + failedCount as string + \
	"\n\nSee the MAXScript Listener for details.") title:"W3D Batch Export"
	(failedCount == 0 and missingCount == 0)
)


macroScript Batch_Export_MP_Buildings_W3D
category:"Westwood Scripts"
buttonText:"Batch MP Buildings"
toolTip:"Export all multiplayer building GMax scenes to W3D"
icon:#("GameTools", 2)
(
	if queryBox ("Export every listed multiplayer building scene to W3D?\n\n" + \
		"Existing W3D files will be replaced when overwrite is enabled.") \
		title:"W3D Batch Export" then
	(
		MPB_runBatch()
	)
)


if queryBox ("The batch exporter script is loaded.\n\nRun it now?") title:"W3D Batch Export" then
(
	MPB_runBatch()
)
