# How I prepare my native wire ABI comparison

I adapted my existing native wire proof to accept explicit source and tool paths. I retain its protocol enumeration, static assertions, critical field offsets, ten golden-byte records, and exact Win32/x64 record comparison. My historical source pair produced 2,304 comparison records. I do not claim that preparing this portable adaptation repeats that native result.

I supply two distinct OpenDBO-Core trees. I use an output directory outside both trees; each invocation creates a fresh child and leaves both sources unchanged. I provide the MSBuild path explicitly, including when I only prepare the harness.

```text
python tests/wire_abi.py --source32 "<SOURCE32>" --source64 "<SOURCE64>" --output-root "<OUTPUT_ROOT>" --msbuild "<MSBUILD_EXE>"
```

I get `PREPARED_NOT_COMPILED_OR_RUN` by default. I can add `--prepare` for the same behavior. I receive the generated C++ harness, its project, source-pair hashes, and a preparation receipt. My published receipt labels use `source32/`, `source64/`, and `output/`, rather than personal absolute paths.

I explicitly add `--run` when I am ready to compile and run the console proof:

```text
python tests/wire_abi.py --source32 "<SOURCE32>" --source64 "<SOURCE64>" --output-root "<OUTPUT_ROOT>" --msbuild "<MSBUILD_EXE>" --run
```

I need Windows, MSBuild with MSVC v142, Windows SDK 10.0.19041.0, and the native headers used by the supplied sources. I build Win32 and x64 sequentially with one compiler job, then compare the complete `S`, `O`, and `G` record sequences. The static assertions verify the intended native pointer size and fixed wire widths. I bound each compilation to 240 seconds and each console run to 30 seconds.

I keep the generated raw compiler logs local because MSBuild may include local paths in them. I do not treat those logs as sanitized publication samples. My golden values are synthetic fixture data, including deliberately high-bit identifiers; they are not account or authentication values.

I prove the enumerated packet layouts, selected critical offsets, and golden bytes only. I do not exercise network transport, a game executable, a server, a database, a GPU, a movie SDK, gameplay, or every transitive header. I do not infer full product compatibility or a new human test from this proof.
