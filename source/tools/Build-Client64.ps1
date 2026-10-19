#requires -Version 5.1
[CmdletBinding()]
param(
    [switch]$Execute,
    [string]$SourceRoot,
    [string]$SourceManifest,
    [string]$SourceManifestSha256,
    [string]$DependencyRoot,
    [string]$DependencyContract,
    [string]$DependencyContractSha256,
    [string]$DirectXSDK,
    [string]$BuildRoot,
    [string]$MSBuild,
    [string]$Python,
    [string]$WindowsSDK = '10.0.19041.0',
    [string]$QualificationScript,
    [string]$QualificationScriptSha256,
    [ValidateRange(1,2)][int]$Jobs = 2
)
$ErrorActionPreference = 'Stop'
if (-not $Execute) { Write-Output 'PREPARED_ONLY_NO_SNAPSHOT_BUILD_INSTALL_OR_LAUNCH'; return }
foreach ($required in @($SourceRoot,$SourceManifest,$SourceManifestSha256,$DependencyRoot,$DependencyContract,$DependencyContractSha256,$DirectXSDK,$BuildRoot,$MSBuild,$Python)) {
    if (-not $required) { throw 'I require every explicit source, dependency, output and toolchain parameter.' }
}
if (Test-Path -LiteralPath $BuildRoot) { throw 'I require a new output directory; I do not overwrite an earlier build.' }
$support = Join-Path $PSScriptRoot 'build_support.py'
$outputProps = Join-Path $PSScriptRoot 'Client64.Output.props'
$dependencyProps = Join-Path $PSScriptRoot 'Dependency64.props'
$rwProps = Join-Path $PSScriptRoot 'RenderWare64.Output.props'
$prepare = @($support,'--plan','--source-root',$SourceRoot,'--source-manifest',$SourceManifest,'--source-manifest-sha256',$SourceManifestSha256,'--dependency-root',$DependencyRoot,'--dependency-contract',$DependencyContract,'--dependency-contract-sha256',$DependencyContractSha256,'--directx-sdk',$DirectXSDK,'--output-root',$BuildRoot)
& $Python @prepare
if ($LASTEXITCODE -ne 0) { throw 'I refused the pre-build source/dependency guard; no compiler started.' }
$plan = Get-Content -LiteralPath (Join-Path $BuildRoot 'plan.json') -Raw | ConvertFrom-Json
$solution = ([IO.Path]::GetFullPath((Join-Path $SourceRoot 'DboClient'))).Replace('\','/') + '/'
$out = ([IO.Path]::GetFullPath($BuildRoot)).Replace('\','/') + '/'
$deps = ([IO.Path]::GetFullPath($DependencyRoot)).Replace('\','/') + '/'
$dx = ([IO.Path]::GetFullPath($DirectXSDK)).Replace('\','/') + '/'
$results = [System.Collections.Generic.List[object]]::new()
function Invoke-Native {
    param([string]$Project,[string]$Label,[string]$Props,[string[]]$Extra=@())
    $log = Join-Path $BuildRoot ($Label+'.log')
    $arguments = @($Project,'/t:Build','/nologo','/v:minimal',"/m:$Jobs",'/nr:false','/p:Configuration=Release','/p:Platform=x64','/p:PlatformToolset=v142','/p:PreferredToolArchitecture=x64',"/p:WindowsTargetPlatformVersion=$WindowsSDK",'/p:CL_MPCount=1','/p:BuildProjectReferences=true',"/p:SolutionDir=$solution",'/p:Migration64Enabled=true',"/p:Migration64BuildRoot=$out",("/p:Migration64RenderWareLibRoot="+$out+'lib/'),"/p:Migration64DependencyRoot=$deps",("/p:Migration64GFxSDKRoot="+$plan.gfx_sdk_root),('/p:Migration64BandiSDK2Include='+$plan.bandi_sdk2_include),"/p:Migration64DirectXSDK=$dx","/p:DXSDK_DIR=$dx","/p:Migration64DependencyProps=$dependencyProps","/p:ForceImportBeforeCppTargets=$Props",'/fl',"/flp:logfile=$log;verbosity=normal;encoding=UTF-8") + $Extra
    Push-Location (Split-Path -Parent $Project)
    try { & $MSBuild @arguments; $code=$LASTEXITCODE } finally { Pop-Location }
    $results.Add([pscustomobject]@{project=$Project;label=$Label;exit_code=$code;log=$log;platform='x64';jobs=$Jobs;CL_MPCount=1})
    $results | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $BuildRoot 'results.json') -Encoding UTF8
    if ($code -ne 0) { throw "I stopped at native build failure: $Label. I preserved the output and log." }
}
foreach ($plugin in $plan.plugins) {
    $extra=@('/p:CharacterSet=MultiByte')
    if ($plugin -eq 'rpskin2') { $extra+='/p:TargetName=rpskin' }
    if ($plugin -eq 'rpuserdat') { $extra+='/p:TargetName=rpusrdat' }
    Invoke-Native (Join-Path $SourceRoot "DboClient/Renderware/Include/$plugin/$plugin.vcxproj") $plugin $rwProps $extra
}
Invoke-Native (Join-Path $SourceRoot 'DboClient/Client/Client.vcxproj') 'Client' $outputProps
& $Python $support --verify-post --plan-file (Join-Path $BuildRoot 'plan.json')
if ($LASTEXITCODE -ne 0) { throw 'I do not accept native exit0 without the post-build source/dependency/AMD64 guard.' }
if ($QualificationScript) {
    if (-not $QualificationScriptSha256 -or (Get-FileHash -LiteralPath $QualificationScript -Algorithm SHA256).Hash.ToLowerInvariant() -ne $QualificationScriptSha256) { throw 'I require the explicit static-qualifier SHA-256.' }
    & $Python $QualificationScript --native-output-manifest (Join-Path $BuildRoot 'native-output-metadata.json')
    if ($LASTEXITCODE -ne 0) { throw 'I preserved the failed static-qualification attempt.' }
}
Write-Output 'NATIVE_AMD64_OUTPUT_ONLY: I have not installed, launched or functionally qualified a client.'
