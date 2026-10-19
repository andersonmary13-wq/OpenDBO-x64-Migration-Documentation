"""I test prepared build guards with synthetic files; I do not invoke a compiler or SDK."""
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
from types import SimpleNamespace
from unittest.mock import patch

TOOLS = Path(__file__).resolve().parents[1] / 'tools'
sys.path.insert(0, str(TOOLS))
import build_support as support
from apply_migration import Refusal


def pe(machine=0x8664):
    raw = bytearray(128); raw[:2] = b'MZ'; struct.pack_into('<I', raw, 60, 64)
    raw[64:68] = b'PE\0\0'; struct.pack_into('<H', raw, 68, machine)
    return bytes(raw)


def archive(count=188, machine=0x8664):
    parts = [b'!<arch>\n']
    for i in range(count):
        body = struct.pack('<H', machine) + b'\0' * 18
        header = ('%-16s%-12s%-6s%-6s%-8s%-10s`\n' % ('o%d.obj/' % i, '0', '0', '0', '100644', str(len(body)))).encode()
        parts.extend((header, body))
    return b''.join(parts)


class PreparedBuildTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix='opendbo-build-guards-test-')
        self.root = Path(self.tmp.name)
        self.deps = self.root / 'deps'; self.deps.mkdir()
        (self.deps / 'link').mkdir(); (self.deps / 'sdk').mkdir(); (self.deps / 'bandi').mkdir()
        (self.deps / 'link/libgfx.lib').write_bytes(archive())
        (self.deps / 'proof.json').write_text('{"synthetic":true}')
        names = ['DevIL.dll', 'BugTrap-x64.dll', 'BugTrapU-x64.dll', 'discord-rpc.dll', 'fmod.dll', 'bdvid64.dll']
        runtime = []
        for name in names:
            (self.deps / name).write_bytes(pe())
            runtime.append({'source': {'path': name, **support.file_pin(self.deps / name)}, 'destination_name': name})
        self.data = {'schema': 'opendbo-client64-dependencies/v1', 'architecture': 'AMD64',
                     'native_build_ready': True,
                     'files': [{'path': 'link/libgfx.lib', **support.file_pin(self.deps / 'link/libgfx.lib')}],
                     'features': {x: True for x in ('Discord', 'BugTrap', 'DevIL', 'FMOD', 'BandiVideo')},
                     'gfx': {'guards_preserved': True,
                             'protected_archive': {'path': 'link/libgfx.lib', **support.file_pin(self.deps / 'link/libgfx.lib')},
                             'qualification_receipt': {'path': 'proof.json', **support.file_pin(self.deps / 'proof.json')},
                             'headless_checks': 48, 'headless_failures': 0},
                     'runtime_files': runtime, 'paths': {'gfx_sdk': 'sdk', 'bandi_sdk2_include': 'bandi'},
                     'BandiVideo': {'actual_export': 'CreateBandiVideo2', 'verification_preserved': True}}
        self.contract = self.root / 'contract.json'

    def tearDown(self):
        self.tmp.cleanup()

    def check(self):
        self.contract.write_text(json.dumps(self.data))
        return support.dependency_check(self.deps, self.contract, support.file_pin(self.contract)['sha256'])

    def test_valid_synthetic_contract_and_all188_native_objects(self):
        self.assertEqual(self.check(), self.data)
        self.assertEqual(support.archive64(self.deps / 'link/libgfx.lib'), 188)

    def test_x86_archive_refused(self):
        (self.deps / 'link/libgfx.lib').write_bytes(archive(machine=0x14c))
        self.data['files'][0].update(support.file_pin(self.deps / 'link/libgfx.lib'))
        self.data['gfx']['protected_archive'].update(support.file_pin(self.deps / 'link/libgfx.lib'))
        with self.assertRaises(Refusal): self.check()

    def test_x86_runtime_refused(self):
        (self.deps / 'DevIL.dll').write_bytes(pe(0x14c))
        self.data['runtime_files'][0]['source'].update(support.file_pin(self.deps / 'DevIL.dll'))
        with self.assertRaises(Refusal): self.check()

    def test_missing_guard_and_failed_oracle_refused(self):
        self.data['gfx']['guards_preserved'] = False
        with self.assertRaises(Refusal): self.check()
        self.data['gfx']['guards_preserved'] = True
        self.data['gfx']['headless_failures'] = 1
        with self.assertRaises(Refusal): self.check()

    def test_feature_stub_refused(self):
        self.data['features']['Discord'] = False
        with self.assertRaises(Refusal): self.check()

    def test_renamed_sdk_and_fake_export_refused(self):
        self.data['BandiVideo']['actual_export'] = 'CreateBandiVideo'
        with self.assertRaises(Refusal): self.check()
        self.data['BandiVideo']['actual_export'] = 'CreateBandiVideo2'
        self.data['runtime_files'][-1]['destination_name'] = 'BVL64.dll'
        with self.assertRaises(Refusal): self.check()

    def test_path_traversal_refused(self):
        self.data['files'][0]['path'] = '../outside.lib'
        with self.assertRaises(Refusal): self.check()

    def test_dependency_hash_refused(self):
        (self.deps / 'DevIL.dll').write_bytes(pe() + b'changed')
        with self.assertRaises(Refusal): self.check()

    def test_native_project_graph_requires_all_frozen_inputs(self):
        core = self.root / 'core'; core.mkdir()
        project = core / 'sample.vcxproj'; (core / 'sample.cpp').write_text('int x;')
        project.write_text('<Project><ItemGroup><ProjectConfiguration Include="Release|x64"/><ClCompile Include="sample.cpp"/></ItemGroup></Project>')
        self.assertEqual(len(support.graph(core, project, {'sample.vcxproj', 'sample.cpp'})), 1)
        with self.assertRaises(Refusal): support.graph(core, project, {'sample.vcxproj'})
        project.write_text('<Project><ProjectConfiguration Include="Release|x64"/><ClCompile Include="$(UNKNOWN)/x.cpp"/></Project>')
        with self.assertRaises(Refusal): support.graph(core, project, {'sample.vcxproj'})

    def test_properties_and_plugin_contracts(self):
        for name in ('Client64.Output.props', 'RenderWare64.Output.props', 'Dependency64.props'):
            ET.parse(TOOLS / name)
        text = (TOOLS / 'Client64.Output.props').read_text()
        self.assertIn('<RuntimeLibrary>MultiThreaded</RuntimeLibrary>', text)
        self.assertIn('<OutputDirectory>$(IntDir)midl\\</OutputDirectory>', text)
        self.assertIn('<HeaderFileName>%(Filename)_h.h</HeaderFileName>', text)
        self.assertIn('<InterfaceIdentifierFileName>%(Filename)_i.c</InterfaceIdentifierFileName>', text)
        self.assertIn("'$(MSBuildProjectName)' == 'Client' or '$(MSBuildProjectName)' == 'NtlSound'", text)
        self.assertIn('Migration64MidlOutputGuard', text)
        self.assertEqual(len(support.PLUGINS), 33)
        wrapper = (TOOLS / 'Build-Client64.ps1').read_text()
        self.assertIn('/p:BuildProjectReferences=true', wrapper)
        self.assertIn('/p:CL_MPCount=1', wrapper)
        self.assertIn('/p:ForceImportBeforeCppTargets=', wrapper)
        self.assertIn('/p:TargetName=rpskin', wrapper)
        self.assertIn('/p:TargetName=rpusrdat', wrapper)
        self.assertIn('if (-not $Execute)', wrapper)

    def test_linked_output_parent_refused_before_manifest_reads(self):
        args = SimpleNamespace(source_root=self.deps, output_root=self.root / 'new-output')
        with patch.object(Path, 'is_junction', lambda p: p == self.root):
            with self.assertRaisesRegex(Refusal, 'linked build-output parents'):
                support.prepare(args)


if __name__ == '__main__':
    unittest.main()
