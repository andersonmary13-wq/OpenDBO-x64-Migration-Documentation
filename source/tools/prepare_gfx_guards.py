"""I check/prepare only two guarded sources from a matching external GFx SDK."""
import argparse
import json
from pathlib import Path
import tempfile
from apply_migration import (Refusal, require, check_pin, digest, no_links, project,
                             restore, patch_membership, atomic_bytes, git)


def check(sdk, manifest, folder):
    patch = no_links(folder, manifest['patch']['path'])
    check_pin(patch.read_bytes(), manifest['patch'], 'minimal GFx patch')
    before = {}
    with tempfile.TemporaryDirectory(prefix='opendbo-gfx-guards-') as tmp:
        work = Path(tmp)
        for row in manifest['files']:
            raw = no_links(sdk, row['path']).read_bytes()
            check_pin(raw, row['before'], 'external GFx source')
            check_pin(project(raw), row['before_projection'], 'external GFx projection')
            before[row['path']] = raw
            atomic_bytes(work / row['path'], project(raw))
        require(patch_membership(work, patch) == set(before), 'I refuse an extra GFx patch target.')
        git(work, 'apply', '--no-index', '--unidiff-zero', '--check', str(patch))
        git(work, 'apply', '--no-index', '--unidiff-zero', str(patch))
        after = {}
        for row in manifest['files']:
            value = (work / row['path']).read_bytes()
            check_pin(value, row['after_projection'], 'guarded GFx projection')
            value = restore(value, row['after_format'])
            check_pin(value, row['after'], 'guarded GFx source')
            after[row['path']] = value
    require(all(no_links(sdk, name).read_bytes() == raw for name, raw in before.items()),
            'I found an SDK source change during preparation.')
    return after


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk-root', type=Path)
    parser.add_argument('--manifest', type=Path,
                        default=Path(__file__).resolve().parents[1] / 'dependencies/gfx-guards-manifest.json')
    parser.add_argument('--prepare', action='store_true', help='I explicitly write two sources to a NEW separate output.')
    parser.add_argument('--output-root', type=Path)
    args = parser.parse_args()
    if args.sdk_root is None:
        print('PREPARED_ONLY_NO_SDK_BUILD_INSTALL_OR_LAUNCH'); return
    try:
        sdk = args.sdk_root.resolve(strict=True); source = args.manifest.resolve(strict=True)
        manifest = json.loads(source.read_bytes())
        require(manifest.get('schema') == 'opendbo-gfx-allocation-guards/v1' and len(manifest['files']) == 2,
                'I require the two-source guard manifest.')
        values = check(sdk, manifest, source.parent)
        if args.prepare:
            require(args.output_root is not None, 'I require an explicit separate output root.')
            out = args.output_root.absolute()
            require(all(not p.is_symlink() and not getattr(p, 'is_junction', lambda: False)()
                        for p in (out, *out.parents)), 'I refuse linked source-output parents.')
            out = out.resolve()
            require(not out.exists() and not out.is_relative_to(sdk), 'I do not overwrite or write inside the SDK.')
            out.mkdir(parents=True)
            for name, raw in values.items():
                atomic_bytes(out / name, raw)
        print(json.dumps({'state': 'GUARDED_SOURCES_PREPARED_NOT_COMPILED' if args.prepare else 'GUARD_PATCH_CHECK_PASS_NO_SDK_WRITES',
                          'source_count': 2, 'manifest_sha256': digest(source.read_bytes()),
                          'sdk_modified': False, 'archive_built': False, 'headless_or_game_test_executed': False}, indent=2))
    except (Refusal, OSError, ValueError, KeyError, TypeError) as error:
        parser.exit(2, 'I refused GFx source preparation: ' + str(error) + '\n')


if __name__ == '__main__':
    main()
