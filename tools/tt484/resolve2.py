import io, os, re
from collections import Counter, defaultdict

BS = chr(92)
CODE = 'Code'

qual_def = defaultdict(list)
qual_any = defaultdict(list)
free_def = defaultdict(list)
class_decl = {}
file_index = defaultdict(list)      # basename lower -> [path]
hdr_class = {}                      # header path -> first class name

QUAL = re.compile(r'(?<![A-Za-z0-9_:])([A-Za-z_][A-Za-z0-9_]*)::(~?[A-Za-z_][A-Za-z0-9_]*)\s*\(')
CLASSD = re.compile(r'^\s*(?:class|struct)\s+([A-Za-z_][A-Za-z0-9_]*)\s*(?::|\{|$)')
FREE = re.compile(r'^[A-Za-z_][A-Za-z0-9_:\*&<>,\s]*?(?<![A-Za-z0-9_])([A-Za-z_][A-Za-z0-9_]*)\s*\([^;]*$')

for dirpath, _d, files in os.walk(CODE):
    for fn in files:
        low = fn.lower()
        if not low.endswith(('.cpp', '.h', '.hpp', '.c')):
            continue
        p = os.path.join(dirpath, fn).replace(BS, '/')
        file_index[low].append(p)
        try:
            lines = io.open(p, encoding='utf-8', errors='replace').read().splitlines()
        except Exception:
            continue
        is_src = low.endswith(('.cpp', '.c'))
        for i, ln in enumerate(lines, 1):
            s = ln.strip()
            if not s or s.startswith('//') or s.startswith('*') or s.startswith('#'):
                continue
            mc = CLASSD.match(ln)
            if mc:
                if mc.group(1) not in class_decl:
                    class_decl[mc.group(1)] = '%s:%d' % (p, i)
                if not is_src and p not in hdr_class:
                    hdr_class[p] = (mc.group(1), i)
            if is_src:
                rs = ln.rstrip()
                is_defn = (ln[:1] not in (' ', '	')) and not rs.endswith(';')                     and not rs.endswith(',') and '(' in ln
                for mq in QUAL.finditer(ln):
                    k = mq.group(1) + '::' + mq.group(2)
                    tgt = qual_def if is_defn else qual_any
                    if len(tgt[k]) < 3:
                        tgt[k].append('%s:%d' % (p, i))
                mf = FREE.match(ln)
                if mf and '::' not in ln.split('(')[0]:
                    nm = mf.group(1)
                    if nm not in ('if', 'for', 'while', 'switch', 'return', 'sizeof'):
                        if len(free_def[nm]) < 3:
                            free_def[nm].append('%s:%d' % (p, i))

# ---- manual tables ---------------------------------------------------------
# TT free function / renamed-class -> canonical OpenW3D owner
FREE_MAP = {
    'Building_Msg_Check':                 ('Code/Commando/dlgcncpurchasemenu.cpp', 'PT "building"/unavailable message text'),
    'Purchase_Vendor_Item':               ('Code/Commando/purchaserequestevent.cpp', 'PT purchase request path'),
    'VendorGrantSupplies':                ('Code/Combat/purchasesettings.cpp', 'PT supply grant'),
    'OnPtChatListInitPatch':              ('Code/Commando/dlgcncpurchasemenu.cpp', 'PT chat list init'),
    'OnPtChatListUpdatePatch':            ('Code/Commando/dlgcncpurchasemenu.cpp', 'PT chat list update'),
    'OnUnitPtKeyDownPatch':               ('Code/Commando/dlgcncpurchasemenu.cpp', 'PT key-down handling'),
    'OnUnitPtMerchandiseItemUpdatePatch': ('Code/Commando/dlgcncpurchasemenu.cpp', 'PT merchandise item update'),
    'List_Column_Patch':                  ('Code/wwui/listctrl.cpp', 'list column colouring'),
    'Enable_Radar_Patch':                 ('Code/Combat/radar.cpp', 'radar enable state'),
    'Cleanup_HUD':                        ('Code/Combat/hud.cpp', 'HUD teardown ordering'),
    'Make_Screen_Shot':                   ('Code/ww3d2/ww3d.cpp', 'screenshot capture'),
    'KeyboardUpdateHook':                 ('Code/Combat/input.cpp', 'keyboard poll'),
    'DoCombatKeyboard':                   ('Code/Combat/input.cpp', 'combat keyboard dispatch'),
    'Get_Key_ID':                         ('Code/Combat/input.cpp', 'key id lookup (side mouse buttons)'),
    'Load_Mix_Files':                     ('Code/Commando/gameinitmgr.cpp', 'startup mix-file loading'),
    'Level_Loader_Thread':                ('Code/Commando/gameinitmgr.cpp', 'threaded level load'),
    'DoUnloadLevel':                      ('Code/Combat/combat.cpp', 'level unload'),
    'Do_Powerup_Sound':                   ('Code/Combat/powerup.cpp', 'powerup pickup sound'),
    'Weapon_Patch':                       ('Code/Combat/weapons.cpp', 'weapon powerup grant replication'),
    'Print_Hook':                         ('Code/Commando/ConsoleMode.cpp', 'console output capture'),
    'Printf_Hook':                        ('Code/Commando/ConsoleMode.cpp', 'console output capture (RenLogMon)'),
    'ResetGeometryPatch':                 ('Code/ww3d2/meshmdl.cpp', 'MeshModelClass geometry reset'),
    'Release_Hook':                       ('Code/ww3d2/dx8wrapper.cpp', 'DX8Wrapper shutdown release'),
    'Peek_Stealth_Texture':               ('Code/wwphys/stealtheffect.cpp', 'stealth texture lookup'),
    'VehicleOwnershipPatch':              ('Code/Combat/vehiclefactorygameobj.cpp', 'vehicle ownership enforcement'),
    'QuickMatchDisable':                  ('Code/Commando/dlgmpwolquickmatchoptions.cpp', 'disable WOL quickmatch'),
    'Bluehell_Fix':                       ('Code/wwphys/pscene.cpp', 'out-of-world ("blue hell") recovery'),
    'ExceptionHandler':                   ('Code/Commando/WINMAIN.CPP', 'top-level exception handler'),
    'PATCH_Get_OS_Info':                  ('Code/Commando/WINMAIN.CPP', 'OS version reporting'),
    'RenegadeAllocate':                   ('Code/wwdebug/wwmemlog.cpp', 'allocator (TT MemoryManager)'),
    'RenegadeFree':                       ('Code/wwdebug/wwmemlog.cpp', 'allocator (TT MemoryManager)'),
    'EmptyFunc':                          ('', 'DROP-stub'),
    'AdvancedGameListingsMenu::sortList':    ('Code/Commando/dlgmpwolgamelist.cpp', 'WOL game list sort'),
    'AdvancedGameListingsMenu::addFavorite': ('Code/Commando/dlgmpwolgamelist.cpp', 'WOL favourites'),
    'IngameSendMessageDialog::sendMessage':  ('Code/Commando/dlgmpingamechat.cpp', 'in-game chat send'),
    'IngameSendMessageDialog::On_EditCtrl_Change':   ('Code/Commando/dlgmpingamechat.cpp', 'in-game chat edit-change (TT adds autocomplete)'),
    'IngameSendMessageDialog::On_EditCtrl_Key_Down': ('Code/Commando/dlgmpingamechat.cpp', 'in-game chat key-down (TT adds autocomplete)'),
    'DlgMPConnecting::On_Periodic':          ('Code/Commando/DlgMPConnect.cpp', 'connecting popup periodic tick'),
    'DlgCnCWinScreen::Update_List':          ('Code/Commando/dlgcncwinscreen.cpp', 'win screen list'),
    'OptionsDlg::New_Start_Dialog':          ('Code/wwui/menudialog.cpp', 'options dialog start'),
    'OptionsDlg::New_On_Command':            ('Code/wwui/menudialog.cpp', 'options dialog command'),
}

# byte-patch (no named replacement) -> owner, keyed by (file-basename, line)
BYTE_MAP = {
    'Do not load all .mix files at startup': 'Code/Commando/gameinitmgr.cpp',
    'enable secret PT pages':            'Code/Commando/dlgcncpurchasemainmenu.cpp',
    'new unpurchasable logic':           'Code/Commando/dlgcncpurchasemenu.cpp',
    'sidebar sounds patch to disable normal sound': 'Code/wwui/stylemgr.cpp',
    'version registry string change':    'Code/wwlib/registry.cpp',
    'WOL ingame URL string change':      'Code/WWOnline/WOLProduct.cpp',
    'kill message disable':              'Code/Combat/messagewindow.cpp',
    'nop out WWPhys::Shutdown call':     'Code/wwphys/wwphys.cpp',
    'emoticons fix':                     'Code/Combat/messagewindow.cpp',
    'dead powerplant 2x cost message change': 'Code/Combat/basecontroller.cpp',
    'dead powerplant 2x cost disable':   'Code/Combat/basecontroller.cpp',
    'PT chatbox fix':                    'Code/Commando/dlgcncpurchasemenu.cpp',
    'PT keypress fix':                   'Code/Commando/dlgcncpurchasemenu.cpp',
    'team information and battlefield information scroll fix': 'Code/Commando/dlgcncbattleinfo.cpp;Code/Commando/dlgcncteaminfo.cpp;Code/Commando/dlgcncserverinfo.cpp',
    'remove Toggle_Sorting console command': 'Code/Commando/consolefunction.cpp',
    'remove MSG console command':        'Code/Commando/consolefunction.cpp',
    'WOL advanced game listings dialog default sort order': 'Code/Commando/dlgmpwolgamelist.cpp',
    'video config dialog':               'Code/Commando/dlgconfigvideotab.cpp',
    'performance config dialog':         'Code/Commando/dlgconfigperformancetab.cpp',
    'RenLogMon hook':                    'Code/Commando/ConsoleMode.cpp',
    'stop FDS from loading at startup':  'Code/Commando/ConsoleMode.cpp',
    'make vehicles not die when they flip over': 'Code/wwphys/vehiclephys.cpp',
    'disable cDonateEvent':              'Code/Commando/donateevent.cpp',
    'disable cConsoleCommandEvent':      'Code/Commando/consolecommandevent.cpp',
    'disable cCsConsoleCommandEvent':    'Code/Commando/consolecommandevent.cpp',
    'disable cMoneyEvent':               'Code/Commando/moneyevent.cpp',
    'disable cRequestKillEvent':         'Code/Commando/requestkillevent.cpp',
    'disable cScoreEvent':               'Code/Commando/scoreevent.cpp',
    'disable cWarpEvent':                'Code/Commando/warpevent.cpp',
    'faster map load change':            'Code/Combat/combat.cpp',
    'logcopy change':                    'Code/Commando/ConsoleMode.cpp',
    'UDP fixes':                         'Code/wwnet/connect.cpp',
    'start button bug fix':              'Code/Commando/dlgmplanhostoptions.cpp',
    'wall lag fix':                      'Code/wwphys/humanphys.cpp',
    'weather manager hooks':             'Code/Combat/WeatherMgr.cpp',
    'remove calls to ToolTipMgrClass::Initialize and MenuDialogClass::Initialize': 'Code/wwui/tooltipmgr.cpp;Code/wwui/menudialog.cpp',
    'remove call to cGameData::Onetime_Init': 'Code/Commando/gamedata.cpp',
    'remove call to StyleMgrClass::Initialize_From_INI': 'Code/wwui/stylemgr.cpp',
    'Fix issue where beam manager is shut down then objects get added to the beam manager, causing a crash on exit': 'Code/Combat/combat.cpp',
}

PER_SITE = {
    ('tt_4.8.4/ttinit/PacketManagerClass.cpp', None): (
        'Code/wwnet/packetmgr.cpp',
        'singleton-redirect: every stock "mov ecx, PacketManager" repointed at TT PacketManager()'),
    ('tt_4.8.4/tt/WeaponClass.cpp', 61): ('Code/Combat/weapons.cpp', 'WeaponClass vtable patch (raw VirtualProtect)'),
    ('tt_4.8.4/tt/WeaponClass.cpp', 62): ('Code/Combat/weapons.cpp', 'WeaponClass vtable patch (raw VirtualProtect)'),
    ('tt_4.8.4/tt/cNetwork.cpp', 108):   ('Code/Commando/cnetwork.cpp', 'cNetwork jump-hook registrant'),
    ('tt_4.8.4/tt/tt.cpp', 994):         ('Code/Combat/input.cpp', 'side mouse buttons in default config dialog'),
}


def exists(path):
    return os.path.isfile(path.replace('/', os.sep))


def by_filename(cls):
    """TT class name -> OpenW3D file of the same name, plus its real class."""
    for ext in ('.cpp', '.h'):
        cands = file_index.get(cls.lower() + ext, [])
        if cands:
            p = cands[0]
            h = p[:-4] + '.h' if ext == '.cpp' else p
            if h in hdr_class:
                nm, ln = hdr_class[h]
                return (p, nm)
            return (p, '')
    return (None, None)


def resolve(sym):
    if not sym:
        return ('', '')
    if sym in FREE_MAP:
        owner, note = FREE_MAP[sym]
        if owner and not exists(owner):
            return (owner, 'manual (path unverified): ' + note)
        return (owner, 'manual: ' + note)
    if '::' in sym:
        if sym in qual_def:
            return (';'.join(qual_def[sym]), 'exact-def')
        cls, meth = sym.split('::', 1)
        for k in qual_def:
            if k.lower() == sym.lower():
                return (';'.join(qual_def[k]), 'case-insensitive-def')
        if sym in qual_any:
            return (';'.join(qual_any[sym]),
                    'call-site-only: declared in a header, no out-of-line body')
        p, real = by_filename(cls)
        if p:
            if real and real + '::' + meth in qual_def:
                return (';'.join(qual_def[real + '::' + meth]),
                        'renamed-class: TT %s == OpenW3D %s' % (cls, real))
            return (p, 'file-match: TT %s -> %s%s (method %s absent)'
                    % (cls, p, ' [' + real + ']' if real else '', meth))
        if cls in class_decl:
            return (class_decl[cls], 'class-only: TT adds method %s' % meth)
        return ('', 'ABSENT: TT-only symbol')
    if sym in free_def:
        return (';'.join(free_def[sym]), 'exact-free-def')
    for k in free_def:
        if k.lower() == sym.lower():
            return (';'.join(free_def[k]), 'case-insensitive-free-def')
    if sym in class_decl:
        return (class_decl[sym], 'class-decl')
    return ('', 'ABSENT: TT-only symbol')


# ---- rewrite TSV -----------------------------------------------------------
src = io.open('docs/tt484/TTHookSites.tsv', encoding='utf-8').read().splitlines()
out = []
stats = Counter()
for i, ln in enumerate(src):
    if i == 0:
        continue
    p = ln.split('\t')
    while len(p) < 9:
        p.append('')
    f, line, prim, addrs, repl, comment = p[:6]
    # repair address-shaped "symbols" produced by the earlier cast strip
    if re.match(r'^x?0{1,2}[0-9a-fA-F]{6}$', repl):
        repl = ''
    if 'shaderhooks' in f:
        owner, how = ('', 'EXCLUDED: D3D8 shader layer (directive 0.6)')
    elif repl:
        owner, how = resolve(repl)
    else:
        key = (f, int(line))
        if 'PacketManagerClass' in f:
            owner, how = PER_SITE[('tt_4.8.4/ttinit/PacketManagerClass.cpp', None)]
            how = 'byte-patch cluster: ' + how
        elif key in PER_SITE:
            owner, how = PER_SITE[key]
            how = 'byte-patch: ' + how
        elif comment in BYTE_MAP:
            owner, how = BYTE_MAP[comment], 'byte-patch by intent'
        else:
            owner, how = ('', 'UNMAPPED: byte patch, intent not recovered')
    if how == 'manual: DROP-stub':
        how = 'DROP: stock debug/no-op function removed outright; nothing to port'
    if how.startswith(('exact-def', 'exact-free-def', 'renamed-class')):
        conf = 'high'
    elif how.startswith(('case-insensitive', 'class-only', 'file-match', 'byte-patch cluster', 'call-site-only')) or (how.startswith('manual:') and owner):
        conf = 'medium'
    elif how.startswith(('manual (path unverified)', 'byte-patch by intent', 'byte-patch:')):
        conf = 'low'
    else:
        conf = 'none'
    stats[how.split(':')[0]] += 1
    out.append([f, line, prim, addrs, repl, comment, owner, how, conf])

with io.open('docs/tt484/TTHookSites.tsv', 'w', encoding='utf-8', newline='\n') as fh:
    fh.write('file\tline\tprimitive\taddresses\ttt_replacement\tintent_comment\topenw3d_owner\tmapping_method\tconfidence\n')
    for r in out:
        fh.write('\t'.join(r) + '\n')

for k, v in stats.most_common():
    print('  %-40s %d' % (k, v))
print('')
inscope = [r for r in out if not r[7].startswith('EXCLUDED')]
print('in-scope sites: %d' % len(inscope))
print('  with an OpenW3D owner: %d' % sum(1 for r in inscope if r[6]))
print('  DROP (nothing to port): %d' % sum(1 for r in inscope if r[7].startswith('DROP')))
print('  no owner yet:          %d' % sum(1 for r in inscope if not r[6] and not r[7].startswith('DROP')))
print('')
for c in ('high', 'medium', 'low', 'none'):
    print('  confidence %-7s %d' % (c, sum(1 for r in inscope if r[8] == c)))
