"""Builds the whole cover-shooter demo: StateTree asset + demo level.

Run from the editor console:
    py "<Project>/Scripts/build_cover_demo.py"

or headless:
    UnrealEditor-Cmd.exe "<Project>/StateTree.uproject" -run=pythonscript ^
        -script="<Project>/Scripts/build_cover_demo.py" -unattended -nullrhi -nosplash

Result:
    /Game/AI/StateTree/ST_Bot_CoverShooter   (StateTree AI Component schema)
    /Game/AI/Lvl_CoverShooting               (floor from the template + 5 covers + bot + navmesh)
"""

import unreal

LEVEL_PATH = "/Game/AI/Lvl_CoverShooting"

# The level is rebuilt from scratch every time, so remove the previous copy first.
if unreal.EditorAssetLibrary.does_asset_exist(LEVEL_PATH):
    unreal.EditorAssetLibrary.delete_asset(LEVEL_PATH)
    unreal.log("[CoverAI] removed old %s" % LEVEL_PATH)

state_tree_ok = unreal.CoverAIEditorTools.build_cover_state_tree()
unreal.log("[CoverAI] state tree built: %s" % state_tree_ok)

level_ok = unreal.CoverAIEditorTools.build_demo_level()
unreal.log("[CoverAI] demo level built: %s" % level_ok)

unreal.log("[CoverAI] done (state_tree=%s, level=%s)" % (state_tree_ok, level_ok))
