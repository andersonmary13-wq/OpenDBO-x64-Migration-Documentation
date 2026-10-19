"""I bind a portable native build to explicit source and external dependency pins."""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import os
import shutil
import struct
import xml.etree.ElementTree as ET

from apply_migration import Refusal, require, check_pin, digest, relative, no_links, load_manifest

PLUGINS = ('rtdict', 'rtcharse', 'rtfsyst', 'rtbmp', 'rtpng', 'rtintsec', 'rtpick', 'rttoc',
           'rtanim', 'rphanim', 'rtquat', 'rplogo', 'rpuserdat', 'rpid', 'rpmatfx', 'rpuvanim',
           'rpprtstd', 'rpprtadv', 'rpptank', 'rpskintoon', 'rpskin2', 'rprandom', 'rtimport',
           'rtgcond', 'rtwing', 'rtray', 'rplodatm', 'rppvs', 'rpltmap', 'rtltmap', 'rpmipkl',
           'rtpitexd', 'rtbary')
TOOLS = ('apply_migration.py', 'Build-Client64.ps1', 'build_support.py',
         'Client64.Output.props', 'RenderWare64.Output.props', 'Dependency64.props')


def file_pin(path):
    data = path.read_bytes()
    return dict(bytes=len(data), sha256=digest(data))


def image_machine(path):
    with path.open('rb') as stream:
        raw = stream.read(64)
        require(len(raw) == 64 and raw[:2] == b'MZ', 'I require a PE image.')
        stream.seek(struct.unpack_from('<I', raw, 60)[0])
        raw = stream.read(6)
        require(raw[:4] == b'PE\0\0', 'I require a valid PE signature.')
        return struct.unpack_from('<H', raw, 4)[0]


def archive64(path):
    raw = path.read_bytes()
    require(raw.startswith(b'!<arch>\n'), 'I require a COFF archive.')
    position, count = 8, 0
    while position < len(raw):
        header = raw[position:position + 60]
        require(len(header) == 60 and header[58:] == b'`\n', 'I refuse a malformed archive.')
        size = int(header[48:58]); body = raw[position + 60:position + 60 + size]
        require(len(body) == size, 'I refuse a truncated archive.')
        name = header[:16].strip()
        if name not in (b'/', b'//', b'/SYM64/'):
            require(len(body) >= 8, 'I refuse a short archive object.')
            machine = struct.unpack_from('<H', body, 6 if body[:4] == b'\0\0\xff\xff' else 0)[0]
            require(machine == 0x8664, 'I refuse an archive containing a non-AMD64 object.')
            count += 1
        position += 60 + size + (size & 1)
    require(count > 0, 'I require archive objects.')
    return count


def freeze(core):
    rows = []
    for directory, dirs, files in os.walk(core, followlinks=False):
        folder = Path(directory)
        dirs[:] = [d for d in dirs if d != '.git']
        for d in dirs:
            p = folder / d
            require(not p.is_symlink() and not getattr(p, 'is_junction', lambda: False)(),
                    'I refuse source directory links.')
        for name in files:
            p = folder / name
            rel = p.relative_to(core).as_posix()
            no_links(core, rel)
            rows.append(dict(path=rel, **file_pin(p)))
    return sorted(rows, key=lambda r: r['path'])


def graph(core, project, source_names, seen=None):
    seen = {} if seen is None else seen
    project = no_links(core, project.relative_to(core).as_posix())
    if project in seen:
        return list(seen.values())
    rel = project.relative_to(core).as_posix()
    require(rel in source_names, 'I refuse an unfrozen project.')
    tree = ET.parse(project)
    require('Release|x64' in {n.get('Include') for n in tree.findall('.//{*}ProjectConfiguration')},
            'I require native Release|x64 project configurations.')
    seen[project] = {'path': rel}
    for node in tree.iter():
        tag = node.tag.rsplit('}', 1)[-1]
        include = node.get('Include')
        if tag not in ('ProjectReference', 'ClCompile', 'ClInclude', 'ResourceCompile') or not include:
            continue
        require('$(' not in include, 'I require review of an unresolved source include.')
        path = (project.parent / include.replace('\\', '/')).resolve()
        require(path.is_relative_to(core), 'I refuse an external project/source include outside the explicit tree.')
        name = path.relative_to(core).as_posix()
        no_links(core, name)
        require(name in source_names, 'I refuse an unfrozen compile/header/resource input: ' + name)
        if tag == 'ProjectReference':
            graph(core, path, source_names, seen)
    return list(seen.values())


def dependency_check(root, path, sha):
    raw = path.read_bytes()
    require(digest(raw) == sha, 'I require the external contract SHA-256.')
    data = json.loads(raw)
    require(data.get('schema') == 'opendbo-client64-dependencies/v1' and
            data.get('architecture') == 'AMD64' and data.get('native_build_ready') is True,
            'I require an explicitly qualified AMD64 dependency contract.')
    require(data.get('files'), 'I require individual dependency pins.')
    for row in data['files']:
        check_pin(no_links(root, row['path']).read_bytes(), row, 'external dependency')
    require(all(data.get('features', {}).get(x) is True for x in ('Discord', 'BugTrap', 'DevIL', 'FMOD', 'BandiVideo')),
            'I refuse a disabled or stubbed required feature.')
    gfx = data['gfx']
    require(gfx.get('guards_preserved') is True, 'I require the two GFx guard changes.')
    archive = no_links(root, gfx['protected_archive']['path'])
    check_pin(archive.read_bytes(), gfx['protected_archive'], 'guarded GFx archive')
    require(archive == root / 'link/libgfx.lib' and archive64(archive) == 188,
            'I require the guarded 188-object archive at the explicit link input.')
    proof = no_links(root, gfx['qualification_receipt']['path'])
    check_pin(proof.read_bytes(), gfx['qualification_receipt'], 'external GFx proof receipt')
    require(gfx.get('headless_checks') == 48 and gfx.get('headless_failures') == 0,
            'I require the external 48-check guard proof; I do not rerun it in preflight.')
    names = set()
    for row in data['runtime_files']:
        name = row['destination_name']
        require(relative(name).name == name and name.lower().endswith('.dll') and name.casefold() not in names,
                'I require unique bounded DLL basenames.')
        names.add(name.casefold())
        source = no_links(root, row['source']['path'])
        check_pin(source.read_bytes(), row['source'], 'runtime dependency')
        require(image_machine(source) == 0x8664, 'I require actual AMD64 DLLs.')
    require(names == {'devil.dll', 'bugtrap-x64.dll', 'bugtrapu-x64.dll', 'discord-rpc.dll', 'fmod.dll', 'bdvid64.dll'},
            'I require the six real runtime dependencies, without a renamed BVL or shim.')
    require(data['BandiVideo'].get('actual_export') == 'CreateBandiVideo2' and
            data['BandiVideo'].get('verification_preserved') is True,
            'I require the genuine SDK2 interface and verification path.')
    for key in ('gfx_sdk', 'bandi_sdk2_include'):
        require(no_links(root, data['paths'][key]).is_dir(), 'I require explicit external SDK include directories.')
    return data


def prepare(args):
    core = args.source_root.resolve(strict=True)
    output = args.output_root.absolute()
    require(all(not p.is_symlink() and not getattr(p, 'is_junction', lambda: False)()
                for p in (output, *output.parents)), 'I refuse linked build-output parents.')
    output = output.resolve()
    require(not output.exists() and not output.is_relative_to(core), 'I require a new build root outside source.')
    source, source_raw = load_manifest(args.source_manifest.resolve(strict=True), args.source_manifest_sha256)
    for row in source['final_union_files']:
        check_pin(no_links(core, row['path']).read_bytes(), row, 'migrated source')
    dependencies = dependency_check(args.dependency_root.resolve(strict=True), args.dependency_contract.resolve(strict=True), args.dependency_contract_sha256)
    dx = args.directx_sdk.resolve(strict=True)
    require((dx / 'Include/d3d9.h').is_file() and (dx / 'Lib/x64/d3dx9.lib').is_file(),
            'I require the official DirectX SDK headers and x64 libraries.')
    rows = freeze(core); names = {r['path'] for r in rows}
    projects = graph(core, core / 'DboClient/Client/Client.vcxproj', names)
    for plugin in PLUGINS:
        graph(core, core / ('DboClient/Renderware/Include/%s/%s.vcxproj' % (plugin, plugin)), names)
    require('DboClient/Lib/Discord/Discord.vcxproj' in {r['path'] for r in projects}, 'I require the Discord project.')
    project_text = (core / 'DboClient/Lib/Discord/Discord.vcxproj').read_text(encoding='utf-8-sig')
    header_text = (core / 'DboClient/Lib/Discord/Discord.h').read_text(encoding='utf-8-sig')
    require('Discord.cpp' in project_text and 'DiscordStub.cpp' not in project_text and
            'discord_rpc.h' in header_text and 'discord_rpc_minimal.h' not in header_text,
            'I require the full Discord implementation and declarations.')
    movie = ET.parse(core / 'DboClient/Lib/NtlMovie/NtlMovie.vcxproj')
    for name in ('LegacyVideo21Adapter.cpp', 'Backend21.cpp', 'Sound/Sdk2Sound.cpp', 'Video21Integration.cpp'):
        found = [n for n in movie.findall('.//{*}ClCompile') if n.get('Include', '').replace('\\', '/') == 'Video21Bridge/' + name]
        require(len(found) == 1, 'I require each bridge compile item.')
        item = found[0]
        only64 = "=='x64'" in item.get('Condition', '').replace(' ', '')
        excluded32 = any(n.text == 'true' and "!='x64'" in n.get('Condition', '').replace(' ', '') for n in item.findall('{*}ExcludedFromBuild'))
        require((only64 or excluded32) and any(n.text == 'NotUsing' for n in item.findall('{*}PrecompiledHeader')),
                'I require isolated x64-only bridge translation units without inherited PCH.')
    tools = Path(__file__).resolve().parent
    output.mkdir(parents=True)
    value = {'state': 'PLAN_ONLY_NOT_BUILT', 'utc': datetime.now(timezone.utc).isoformat(),
             'source_root': str(core), 'output_root': str(output), 'source_manifest': str(args.source_manifest.resolve()),
             'source_manifest_sha256': digest(source_raw), 'source_files': rows,
             'dependency_root': str(args.dependency_root.resolve()), 'dependency_contract': str(args.dependency_contract.resolve()),
             'dependency_contract_sha256': args.dependency_contract_sha256,
             'directx_sdk': str(dx), 'directx_pins': [{'path': p, **file_pin(dx / p)} for p in ('Include/d3d9.h', 'Lib/x64/d3dx9.lib')],
             'tools': [{'path': n, **file_pin(tools / n)} for n in TOOLS], 'plugins': list(PLUGINS),
             'gfx_sdk_root': str(args.dependency_root.resolve() / dependencies['paths']['gfx_sdk']),
             'bandi_sdk2_include': str(args.dependency_root.resolve() / dependencies['paths']['bandi_sdk2_include']),
             'client_project_graph': projects, 'runtime_qualification': False}
    with (output / 'plan.json').open('x', encoding='utf-8') as f:
        json.dump(value, f, indent=2); f.write('\n')
    return value


def verify_post(plan_file):
    plan = json.loads(plan_file.read_bytes()); output = Path(plan['output_root']); core = Path(plan['source_root'])
    require(freeze(core) == plan['source_files'], 'I found changed or extra source-tree files after the build.')
    check_pin(Path(plan['source_manifest']).read_bytes(), {'bytes': len(Path(plan['source_manifest']).read_bytes()), 'sha256': plan['source_manifest_sha256']}, 'source manifest')
    dependencies = dependency_check(Path(plan['dependency_root']), Path(plan['dependency_contract']), plan['dependency_contract_sha256'])
    for row in plan['tools']:
        check_pin((Path(__file__).parent / row['path']).read_bytes(), row, 'build recipe')
    for row in plan['directx_pins']:
        check_pin((Path(plan['directx_sdk']) / row['path']).read_bytes(), row, 'DirectX input')
    results = json.loads((output / 'results.json').read_text(encoding='utf-8-sig'))
    require(len(results) == len(PLUGINS) + 1 and all(r['exit_code'] == 0 and r['platform'] == 'x64' and r['CL_MPCount'] == 1 for r in results),
            'I require every explicit native stage to finish successfully.')
    image = output / 'bin/Client.exe'
    require(image_machine(image) == 0x8664, 'I require the actual new AMD64 Client output.')
    for row in dependencies['runtime_files']:
        source = no_links(Path(plan['dependency_root']), row['source']['path'])
        target = output / 'bin' / row['destination_name']
        require(not target.exists(), 'I refuse an unexpected runtime output overwrite.')
        shutil.copy2(source, target)
        check_pin(target.read_bytes(), row['source'], 'staged build-local DLL')
    outputs = []
    for p in sorted((output / 'bin').iterdir()):
        if p.suffix.lower() in ('.exe', '.dll'):
            require(image_machine(p) == 0x8664, 'I refuse a non-AMD64 output image.')
            outputs.append({'path': p.relative_to(output).as_posix(), **file_pin(p)})
    value = {'state': 'NATIVE_AMD64_OUTPUT_ONLY_NEEDS_STATIC_AND_GAME_QUALIFICATION', 'outputs': outputs,
             'source_and_dependency_pins_match': True, 'installation_performed': False, 'game_launched': False,
             'limits': ['I have not checked full import closure, SymChk, selected guarded-object MAP provenance, or audiovisual/gameplay behavior in this metadata step.']}
    with (output / 'native-output-metadata.json').open('x', encoding='utf-8') as f:
        json.dump(value, f, indent=2); f.write('\n')
    return value


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument('--plan', action='store_true'); modes.add_argument('--verify-post', action='store_true')
    for name in ('source-root', 'source-manifest', 'dependency-root', 'dependency-contract', 'directx-sdk', 'output-root', 'plan-file'):
        parser.add_argument('--' + name, type=Path)
    parser.add_argument('--source-manifest-sha256'); parser.add_argument('--dependency-contract-sha256')
    args = parser.parse_args()
    if not args.plan and not args.verify_post:
        print('PREPARED_ONLY_NO_BUILD_SOURCE_SNAPSHOT_OR_RUNTIME_OBSERVATION'); return
    try:
        result = verify_post(args.plan_file) if args.verify_post else prepare(args)
        print(json.dumps({'state': result['state']}))
    except (Refusal, OSError, ValueError, KeyError, TypeError, AttributeError) as error:
        parser.exit(2, 'I refused the native build guard: ' + str(error) + '\n')


if __name__ == '__main__':
    main()
