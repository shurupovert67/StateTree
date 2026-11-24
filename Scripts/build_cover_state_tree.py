"""Creates / rebuilds /Game/AI/StateTree/ST_Bot_CoverShooter.

Run from the editor console:
    py "<Project>/Scripts/build_cover_state_tree.py"

or headless:
    UnrealEditor-Cmd.exe "<Project>/StateTree.uproject" -run=pythonscript ^
        -script="<Project>/Scripts/build_cover_state_tree.py" -unattended -nullrhi -nosplash
"""

import unreal

ok = unreal.CoverAIEditorTools.build_cover_state_tree()
unreal.log("[CoverAI] build_cover_state_tree -> %s" % ok)
