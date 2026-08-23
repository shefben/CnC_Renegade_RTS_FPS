#
# One name, one built-in script.  Two registrations of the same name compile
# fine and then race during static initialization, so the catalog is checked
# here and generation fails rather than the collision being found at runtime.
#
find_package(Python3 COMPONENTS Interpreter)

if(Python3_FOUND)
    execute_process(
        COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/tools/check_script_catalog.py"
                "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE SCRIPT_CATALOG_RESULT
        OUTPUT_VARIABLE SCRIPT_CATALOG_OUTPUT
        ERROR_VARIABLE SCRIPT_CATALOG_ERROR)

    if(NOT SCRIPT_CATALOG_RESULT EQUAL 0)
        message(FATAL_ERROR "Built-in script catalog check failed:\n${SCRIPT_CATALOG_ERROR}")
    endif()

    string(STRIP "${SCRIPT_CATALOG_OUTPUT}" SCRIPT_CATALOG_OUTPUT)
    message(STATUS "Script catalog: ${SCRIPT_CATALOG_OUTPUT}")
else()
    message(WARNING "Python not found; the built-in script catalog was not checked for duplicate names.")
endif()

set(SCRIPTS_SRC
    CustomEvents.h
    DPrint.cpp
    DPrint.h
    DrMobius.cpp
    ScriptNode.h
    ScriptRegistrant.h
    Test_BMG.cpp
    Test_Cinematic.cpp
    Test_DAK.cpp
    Test_DAY.cpp
    Test_DLS.cpp
    Test_DME.cpp
    Test_GTH.cpp
    Test_JDG_EVA.cpp
    Test_PDS.cpp
    Test_RAD.cpp
    Test_RMV.cpp
    Test_RMV_Toolkit.cpp
    Toolkit.cpp
    Toolkit.h
    Toolkit_Actions.cpp
    Toolkit_Animations.cpp
    Toolkit_Broadcaster.cpp
    Toolkit_Explosions.cpp
    Toolkit_Objectives.cpp
    TT_Powerup.cpp
    TT_World.cpp
    TT_SSGM.cpp
    TT_Weapons.cpp
    TT_Scripts.cpp
    TT_Defenses.cpp
    TT_Customs.cpp
    TT_Hooks.cpp
    TT_Vehicles.cpp
    Toolkit_Objects.cpp
    Toolkit_Powerup.cpp
    Toolkit_Siege.cpp
    Toolkit_Sounds.cpp
    Toolkit_Spawners.cpp
    Toolkit_Triggers.cpp
    Mission00.cpp
    Mission00.h
    Mission01.cpp
    Mission02.cpp
    Mission03.cpp
    Mission04.cpp
    Mission05.cpp
    Mission06.cpp
    Mission07.cpp
    mission08.cpp
    Mission09.cpp
    Mission1.h
    Mission10.cpp
    Mission10.h
    Mission11.cpp
    Mission11.h
    Mission2.h
    Mission3.h
    Mission4.h
    Mission5.h
    Mission6.h
    Mission7.h
    Mission8.h
    Mission9.h
    MissionDemo.cpp
    MissionX0.cpp
    MissionX0.h
    scripts.cpp
    scripts.h
)

# Targets to build.

#
# An OBJECT library, not a static one.  Every built-in script registers itself
# from a file-scope object that nothing references, so a static library would
# let the linker drop whole scripts; object libraries are linked whole.
#
add_library(scripts OBJECT)

target_link_libraries(scripts PRIVATE
    wwcommon
)

target_sources(scripts PRIVATE ${SCRIPTS_SRC})

#
# The editor needs the same catalog the game runs, so it links the same
# sources.  Its combat library is built with PARAM_EDITING_ON, but the scripts
# themselves only ever call the engine interface declared in scriptcommands.h.
#
if (W3D_TOOLS)
    add_library(scriptse OBJECT)

    target_link_libraries(scriptse PRIVATE
        wwcommon
    )

    target_compile_definitions(scriptse PRIVATE PARAM_EDITING_ON)

    target_sources(scriptse PRIVATE ${SCRIPTS_SRC})
endif()
