"""The donor-name -> canonical-name map the file conversion will use.

Some 4.8.4 SDK functions are a second name for something the canonical engine
already does.  Directive 0.4 forbids keeping both, so they are not ported --
they are renamed at the call site.  This records which, so the conversion is
mechanical and the API-gap count stops reporting them as work.
"""
import io

TAB = chr(9)

RENAMES = [
    # donor name, canonical name, note
    ('Get_Object_Type', 'Get_Player_Type',
     'the same field; Get_Player_Type answers a DamageableGameObj since P04-C1'),
    ('Set_Object_Type', 'Set_Player_Type', 'ditto'),
    ('Find_Closest_Preset_By_Team', 'Find_Nearest_Preset',
     'argument order differs: (position, preset, team)'),
    ('Send_Custom_Event_To_Object', 'Send_Custom_To_Preset',
     'the donor name says object; it sends to every object of a preset'),
    ('Get_Vehicle_Return', 'Get_Vehicle',
     'canonical Get_Vehicle already answers the object itself when it is not a rider'),
    ('Attach_Script_Preset_Once', 'Attach_Script_Preset', 'once=true'),
    ('Attach_Script_Type_Once', 'Attach_Script_Type', 'once=true'),
    ('Attach_Script_Is_Preset', 'Attach_Script_Preset',
     'the single-object form; test the preset at the call site'),
    ('Attach_Script_Is_Type', 'Attach_Script_Type',
     'the single-object form; test the class id at the call site'),
    ('Change_Team_2', 'Change_Team', 'destroy_object=false'),
    ('Is_Harvester_Preset', 'Is_Harvester', 'canonical asks the base, not the preset table'),
    ('Get_Team_Color', 'Get_Color_For_Team',
     'canonical returns a Vector3 of 0..1 rather than three 0..255 ints'),
    ('Find_Definition', 'Is_Valid_Preset_ID / Get_Definition_Name',
     'a script has no use for the DefinitionClass itself; ask what it wanted to know'),
    ('Get_GameObj', 'Find_Object_By_Player_ID', 'see TTScriptApiGap.tsv: still to port'),
]

rows = ['donor_name' + TAB + 'canonical' + TAB + 'note']
rows += [TAB.join(r) for r in RENAMES]
io.open('docs/tt484/TTScriptApiRenames.tsv', 'w', encoding='utf-8',
        newline='').write('\n'.join(rows) + '\n')
print('%d renames' % len(RENAMES))
