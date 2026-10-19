#!/usr/bin/env python3
"""I compare the native Win32/x64 wire layouts of my two supplied source trees.

I preserve the original protocol enumeration, static assertions, critical offsets,
ten golden-byte records, project/toolset settings and record comparison logic.
This portable adaptation has not itself repeated the historical native proof.
Preparation is the default. Only an explicit --run compiles and runs the harness.
"""
from pathlib import Path
import re, json, hashlib, sys, subprocess, datetime, uuid, argparse, os, stat


def file_bytes(path):
    s=path.lstat()
    if not stat.S_ISREG(s.st_mode) or path.is_symlink() or (getattr(s,'st_file_attributes',0)&0x400):
        raise ValueError('I require a regular input file without a reparse point.')
    return path.read_bytes()


def source_pin(path, source_root, label):
    data=file_bytes(path)
    return {'path':label+'/'+path.relative_to(source_root).as_posix(),
            'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}


def output_pin(path, folder):
    data=file_bytes(path)
    return {'path':'output/'+path.relative_to(folder).as_posix(),
            'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}


def write_json(path, value):
    with path.open('x',encoding='utf-8',newline='\n') as f:
        json.dump(value,f,indent=2);f.write('\n')


def resolved_source(value):
    path=Path(value).expanduser().resolve(strict=True)
    if not path.is_dir():raise ValueError('I require a source directory.')
    for rel in ('DboShared/NtlShared2/NtlPacketCommon.h','DboShared/NtlShared2/NtlInventoryStackSort.h'):
        file_bytes(path/rel)
    return path


def within(path, source):
    return path==source or source in path.parents


def check_output_ancestors(path):
    for ancestor in (path,*path.parents):
        if ancestor.exists():
            s=ancestor.lstat()
            if ancestor.is_symlink() or (getattr(s,'st_file_attributes',0)&0x400):
                raise ValueError('I do not write output through a reparse point.')


def new_output(args, source32, source64):
    requested=Path(args.output_root).expanduser().absolute()
    check_output_ancestors(requested)
    output=requested.resolve(strict=False)
    if within(output,source32) or within(output,source64):
        raise ValueError('I require --output-root outside both source trees.')
    output.mkdir(parents=True,exist_ok=True)
    label='run' if args.run else 'prepare'
    folder=output/(label+'-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ')+'-'+uuid.uuid4().hex[:8])
    if within(folder,source32) or within(folder,source64):
        raise ValueError('I require an isolated output directory.')
    folder.mkdir(exist_ok=False)
    return folder


def check_source_pins(info, source32, source64):
    for entry in info['inputs']:
        label,rel=entry['path'].split('/',1)
        root=source32 if label=='source32' else source64 if label=='source64' else None
        if root is None:raise ValueError('Unknown source label.')
        actual=source_pin(root/rel,root,label)
        if actual!=entry:raise ValueError('A source input changed during my proof.')


def strip_comments(t):
 return re.sub(r'//[^\n]*|/\*.*?\*/','',t,flags=re.S)


def prepare(folder, c32, c64):
 headers=[];packets={};aliases={};pins=[]
 for f in sorted((c32/'DboShared/NtlShared2').glob('NtlPacket*.h')):
  if f.name=='NtlPacketCommon.h':continue
  text=strip_comments(f.read_text(encoding='latin1'))
  names=re.findall(r'\bBEGIN_PROTOCOL\s*\(\s*([A-Za-z_]\w*)\s*\)',text)
  alias=re.findall(r'\bBEGIN_PROTOCOL_IDENTITY\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)',text)
  if not names and not alias:continue
  rel=f.relative_to(c32);other=c64/rel;assert f.read_bytes()==other.read_bytes(),str(rel)
  headers.append(f.name);pins.extend([source_pin(f,c32,'source32'),source_pin(other,c64,'source64')])
  for n in names:
   assert n not in packets,(n,f.name);packets[n]=f.name
  for n,target in alias:aliases[n]=target
 # Order aliases after their defining header, otherwise keep stable order.
 ordered=[];pending=headers[:]
 while pending:
  progress=False
  for name in pending[:]:
   required={packets[target] for alias,target in aliases.items() if packets.get(alias)==name and target in packets}
   if required-set(ordered)-{name}:continue
   ordered.append(name);pending.remove(name);progress=True
  assert progress,'Cyclic protocol header dependencies need review'
 cpp=['#define WIN32_LEAN_AND_MEAN','#include <Windows.h>','#include <cstdint>','#include <cstdio>','#include <cstring>','#include <climits>','#include <cstddef>','#include <type_traits>','#include "NtlPacketCommon.h"']+['#include "'+n+'"' for n in ordered]+['#include "NtlInventoryStackSort.h"']
 cpp+=['static_assert(sizeof(void*)==WIRE_PTR_BYTES,"native architecture");','static_assert(sizeof(WORD)==2&&sizeof(DWORD)==4&&sizeof(WCHAR)==2,"Windows wire scalars");','static_assert(sizeof(HOBJECT)==4&&sizeof(TBLIDX)==4&&sizeof(ITEMID)==8,"wire identity widths");','static_assert(sizeof(NtlInventoryStackSort::Move)==25&&sizeof(NtlInventoryStackSort::ClientMove)==10,"stack sort wire");','static_assert(NtlInventoryStackSort::MaxItems==160,"wire cap cohort");','static_assert(sizeof(sGQ_INVENTORY_STACK_SORT_ATOMIC_REQ)<=4095&&sizeof(sQG_INVENTORY_STACK_SORT_ATOMIC_RES)<=4095,"maximum frame cap");']
 cpp+=['static void size(const char*n,unsigned op,size_t z){printf("S|%s|%u|%zu\\n",n,op,z);}','static void off(const char*n,size_t o,size_t z){printf("O|%s|%zu|%zu\\n",n,o,z);}','static void gold(const char*n,const void*p,size_t z){printf("G|%s|%zu|",n,z);const unsigned char*b=(const unsigned char*)p;for(size_t i=0;i<z;++i)printf("%02x",b[i]);puts("");}','template<class T> static void zero(T&v,WORD opcode){memset(&v,0,sizeof(v));v.wOpCode=opcode;}']
 cpp+=['int main(){printf("A|pointer|%zu\\n",sizeof(void*));']
 for n in sorted(set(packets)|set(aliases)):cpp+=['size("s'+n+'",'+n+',sizeof(s'+n+'));']
 fields={'sUG_INVENTORY_STACK_SORT_ATOMIC_REQ':['token','byScope','byBankIndex','hNpcHandle'],'sGU_INVENTORY_STACK_SORT_ATOMIC_RES':['token','byScope','byBankIndex','byStatus','wCount','moves'],'sGQ_INVENTORY_STACK_SORT_ATOMIC_REQ':['token','handle','charId','accountId','byScope','byBankIndex','wCount','moves'],'sQG_INVENTORY_STACK_SORT_ATOMIC_RES':['token','handle','charId','byScope','byBankIndex','byStatus','wCount','moves'],'sUG_ITEM_UPGRADE_BY_COUPON_REQ':['hNpcHandle','byItemPlace','byItemPos','byCouponPlace','byCouponPos'],'sGU_ITEM_UPGRADE_BY_COUPON_RES':['wResultCode','byItemPlace','byItemPos','byCouponPlace','byCouponPos','byGrade','byDurationType','nUseStartTime','nUseEndTime','byCouponStack','byRestrictState'],'sGQ_ITEM_UPGRADE_BY_COUPON_REQ':['mutationToken','handle','charId','itemId','couponId','byGrade','byItemPlace','byItemPos','byCouponPlace','byCouponPos','byDurationType','nUseStartTime','nUseEndTime','byRestrictState'],'sQG_ITEM_UPGRADE_BY_COUPON_RES':['mutationToken','byPersistenceUnknown','wResultCode','handle','charId','itemId','couponId','byStack']}
 for t,fs in fields.items():
  for field in fs:cpp+=['off("'+t+'.'+field+'",offsetof('+t+','+field+'),sizeof((('+t+'*)0)->'+field+'));']
 cpp+=[r'''
 sUG_INVENTORY_STACK_SORT_ATOMIC_REQ ug;zero(ug,UG_INVENTORY_STACK_SORT_ATOMIC_REQ);ug.token=0xFEDCBA9876543210ULL;ug.byScope=NtlInventoryStackSort::PersonalBank;ug.byBankIndex=2;ug.hNpcHandle=0xF1234567U;gold("UG_Stack",&ug,sizeof(ug));
 sGQ_INVENTORY_STACK_SORT_ATOMIC_REQ gq;zero(gq,GQ_INVENTORY_STACK_SORT_ATOMIC_REQ);gq.token=ug.token;gq.handle=0xFEDCBA98U;gq.charId=0xA1234567U;gq.accountId=0xB7654321U;gq.byScope=NtlInventoryStackSort::Bags;gq.byBankIndex=255;gq.wCount=160;
 for(unsigned i=0;i<160;++i){auto&m=gq.moves[i];m.itemId=0xF123456700000000ULL+i;m.handle=0x80000000U+i;m.tableId=0x90000000U+i;m.count=99;m.rank=4;m.grade=15;m.from.place=1+i/32;m.from.pos=i%32;m.to.place=i%5==0?255:m.from.place;m.to.pos=i%5==0?255:31-m.from.pos;m.afterCount=i%5==0?0:98;m.maxStack=99;}gold("GQ_Stack160",&gq,sizeof(gq));
 sGU_INVENTORY_STACK_SORT_ATOMIC_RES gu;zero(gu,GU_INVENTORY_STACK_SORT_ATOMIC_RES);gu.token=ug.token;gu.byScope=gq.byScope;gu.byBankIndex=255;gu.byStatus=NtlInventoryStackSort::Ok;gu.wCount=160;for(unsigned i=0;i<160;++i){gu.moves[i].handle=gq.moves[i].handle;gu.moves[i].from=gq.moves[i].from;gu.moves[i].to=gq.moves[i].to;gu.moves[i].beforeCount=gq.moves[i].count;gu.moves[i].afterCount=gq.moves[i].afterCount;}gold("GU_Stack160",&gu,sizeof(gu));gu.byStatus=NtlInventoryStackSort::PersistenceUnknown;gu.wCount=0;gold("GU_Unknown",&gu,offsetof(sGU_INVENTORY_STACK_SORT_ATOMIC_RES,moves));
 sQG_INVENTORY_STACK_SORT_ATOMIC_RES qg;zero(qg,QG_INVENTORY_STACK_SORT_ATOMIC_RES);qg.token=gq.token;qg.handle=gq.handle;qg.charId=gq.charId;qg.byScope=gq.byScope;qg.byBankIndex=gq.byBankIndex;qg.byStatus=NtlInventoryStackSort::Ok;qg.wCount=160;memcpy(qg.moves,gq.moves,sizeof(gq.moves));gold("QG_Stack160",&qg,sizeof(qg));
 sUG_INVENTORY_SORT_ATOMIC_REQ oldug;zero(oldug,UG_INVENTORY_SORT_ATOMIC_REQ);oldug.token=ug.token;oldug.byScope=ug.byScope;oldug.byBankIndex=ug.byBankIndex;oldug.hNpcHandle=ug.hNpcHandle;gold("UG_LegacySort",&oldug,sizeof(oldug));
 sUG_ITEM_UPGRADE_BY_COUPON_REQ cup;zero(cup,UG_ITEM_UPGRADE_BY_COUPON_REQ);cup.hNpcHandle=0xF1234567U;cup.byItemPlace=1;cup.byItemPos=31;cup.byCouponPlace=2;cup.byCouponPos=30;gold("UG_Coupon",&cup,sizeof(cup));
 sGQ_ITEM_UPGRADE_BY_COUPON_REQ cq;zero(cq,GQ_ITEM_UPGRADE_BY_COUPON_REQ);cq.mutationToken=ug.token;cq.handle=gq.handle;cq.charId=gq.charId;cq.itemId=0x8123456789ABCDEFULL;cq.couponId=0xFEDCBA9876543210ULL;cq.byGrade=15;cq.byItemPlace=1;cq.byItemPos=31;cq.byCouponPlace=2;cq.byCouponPos=30;cq.byDurationType=1;cq.nUseStartTime=0x100000001LL;cq.nUseEndTime=0x200000002LL;cq.byRestrictState=2;gold("GQ_Coupon",&cq,sizeof(cq));
 sQG_ITEM_UPGRADE_BY_COUPON_RES cr;zero(cr,QG_ITEM_UPGRADE_BY_COUPON_RES);cr.mutationToken=cq.mutationToken;cr.byPersistenceUnknown=1;cr.wResultCode=401;cr.handle=cq.handle;cr.charId=cq.charId;cr.itemId=cq.itemId;cr.couponId=cq.couponId;cr.byStack=1;gold("QG_CouponUnknown",&cr,sizeof(cr));
 sITEM_RANDOM_OPTION options[8]={};const int values[8]={0,INT_MAX,INT_MIN,-1,100,-100,123456789,-123456789};for(int i=0;i<8;++i){options[i].wOptionIndex=100+i;options[i].optionValue=values[i];}gold("SignedOption8",options,sizeof(options));
 return 0;
}
''']
 source=folder/'wire-proof.cpp';source.write_text('\n'.join(cpp)+'\n',encoding='ascii')
 project=r'''<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003"><ItemGroup Label="ProjectConfigurations"><ProjectConfiguration Include="Release|Win32"><Configuration>Release</Configuration><Platform>Win32</Platform></ProjectConfiguration><ProjectConfiguration Include="Release|x64"><Configuration>Release</Configuration><Platform>x64</Platform></ProjectConfiguration></ItemGroup><PropertyGroup Label="Globals"><ProjectGuid>{B4ADBD63-2510-4F26-B424-5B55D514E0A5}</ProjectGuid><WindowsTargetPlatformVersion>10.0.19041.0</WindowsTargetPlatformVersion></PropertyGroup><Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props"/><PropertyGroup Label="Configuration"><ConfigurationType>Application</ConfigurationType><PlatformToolset>v142</PlatformToolset><UseDebugLibraries>false</UseDebugLibraries></PropertyGroup><Import Project="$(VCTargetsPath)\Microsoft.Cpp.props"/><PropertyGroup><OutDir>$(ProjectDir)$(Platform)\</OutDir><IntDir>$(ProjectDir)obj\$(Platform)\</IntDir><TargetName>wire-proof</TargetName><WirePointerBytes Condition="'$(Platform)'=='Win32'">4</WirePointerBytes><WirePointerBytes Condition="'$(Platform)'=='x64'">8</WirePointerBytes></PropertyGroup><ItemDefinitionGroup><ClCompile><RuntimeLibrary>MultiThreaded</RuntimeLibrary><MultiProcessorCompilation>false</MultiProcessorCompilation><LanguageStandard>stdcpp17</LanguageStandard><WarningLevel>Level3</WarningLevel><PreprocessorDefinitions>WIRE_PTR_BYTES=$(WirePointerBytes);_CRT_SECURE_NO_WARNINGS;NDEBUG</PreprocessorDefinitions><AdditionalIncludeDirectories>$(SourceRoot)\DboShared\NtlShared2;$(SourceRoot)\NtlLib\Shared\Util;$(SourceRoot)\NtlLib\Shared\NtlTrigger;$(SourceRoot)\NtlLib\Server\NtlNetwork;$(SourceRoot)\NtlLib\Server\NtlSystem</AdditionalIncludeDirectories></ClCompile><Link><SubSystem>Console</SubSystem></Link></ItemDefinitionGroup><ItemGroup><ClCompile Include="wire-proof.cpp"/></ItemGroup><Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets"/></Project>'''
 (folder/'wire-proof.vcxproj').write_text(project)
 info={'source_packet_headers':ordered,'protocol_structs':len(packets),'protocol_aliases':len(aliases),'critical_offset_records':sum(map(len,fields.values())),'inputs':pins,'source':output_pin(source,folder),'project':output_pin(folder/'wire-proof.vcxproj',folder)};(folder/'inputs.json').write_text(json.dumps(info,indent=2)+'\n');return info


def run_native(args, folder, info, source32, source64):
    msbuild=Path(args.msbuild).expanduser().resolve(strict=True)
    file_bytes(msbuild)
    results=[];records={}
    try:
        for platform,source in [('Win32',source32),('x64',source64)]:
            log=folder/(platform+'-compile.log')
            cmd=[str(msbuild),str(folder/'wire-proof.vcxproj'),'/t:Build','/nologo','/v:normal','/m:1','/nr:false','/p:Configuration=Release','/p:Platform='+platform,'/p:CL_MPCount=1','/p:SourceRoot='+str(source)]
            call=subprocess.run(cmd,capture_output=True,timeout=240)
            log.write_bytes(call.stdout+call.stderr)
            results.append({'platform':platform,'compiler_exit':call.returncode,'compile_log':output_pin(log,folder)})
            assert call.returncode==0,platform+' native compiler failed; I retain the local compile log.'
            exe=folder/platform/'wire-proof.exe'
            call=subprocess.run([str(exe)],capture_output=True,timeout=30)
            output=folder/(platform+'-records.txt');output.write_bytes(call.stdout)
            assert call.returncode==0
            lines=call.stdout.decode('ascii').splitlines()
            architecture=[line for line in lines if line.startswith('A|')]
            data=[line for line in lines if line.startswith(('S|','O|','G|'))]
            assert len(data)==info['protocol_structs']+info['protocol_aliases']+info['critical_offset_records']+10
            records[platform]=data
            results[-1].update({'run_exit':call.returncode,'architecture':architecture,'records':output_pin(output,folder),'exe':output_pin(exe,folder)})
        assert records['Win32']==records['x64'],'WIRE_LAYOUT_OR_GOLDEN_BYTES_MISMATCH'
        check_source_pins(info,source32,source64)
    except BaseException as error:
        # I retain raw tool output locally; compiler logs can contain local paths.
        # My receipt records exception type only, never arbitrary private paths.
        write_json(folder/'receipt.json',{'state':'FAIL_NO_EQUIVALENCE_CLAIM','error_type':type(error).__name__,'results':results,'inputs':info})
        raise
    receipt={'state':'PASS_NATIVE_WIRE32_64_EQUIVALENT','utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
             'protocol_structs':info['protocol_structs'],'protocol_aliases':info['protocol_aliases'],
             'critical_offsets':info['critical_offset_records'],'golden_packets':10,
             'records_compared':len(records['Win32']),'results':results,'inputs':info,'compiler_budget':1,
             'limits':['I prove native header layouts and bytes only; I do not exercise transport, accounts, databases, GUI, GPU, BVL or gameplay.',
                       'I do not modify either supplied source tree.',
                       'I retain raw compiler logs locally; I do not treat those logs as sanitized publication samples.']}
    write_json(folder/'receipt.json',receipt)
    return receipt


def main():
    parser=argparse.ArgumentParser(description=__doc__,formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--source32',required=True,help='My preserved Win32 OpenDBO-Core source directory (read only).')
    parser.add_argument('--source64',required=True,help='My candidate x64 OpenDBO-Core source directory (read only).')
    parser.add_argument('--output-root',required=True,help='My output directory outside both source trees; I create a fresh child.')
    parser.add_argument('--msbuild',required=True,help='My MSBuild executable; I use it only with --run.')
    mode=parser.add_mutually_exclusive_group()
    mode.add_argument('--prepare',action='store_true',help='I generate the isolated harness without compiling (default).')
    mode.add_argument('--run',action='store_true',help='I explicitly compile and run the two native console harnesses.')
    args=parser.parse_args()
    source32=resolved_source(args.source32);source64=resolved_source(args.source64)
    if source32==source64:parser.error('I require two distinct source trees.')
    folder=new_output(args,source32,source64)
    try:
        info=prepare(folder,source32,source64)
        info['expected_records']=info['protocol_structs']+info['protocol_aliases']+info['critical_offset_records']+10
        info['portable_adaptation_native_rerun_claimed']=False
        if args.run:
            receipt=run_native(args,folder,info,source32,source64)
        else:
            receipt={'state':'PREPARED_NOT_COMPILED_OR_RUN','utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
                     'narrative':'I generated a fresh isolated harness and input hashes only. I did not run MSBuild or any executable.',
                     'inputs':info,'portable_adaptation_native_rerun_claimed':False,
                     'limits':['I preserve the historical comparison logic; I do not claim that this portable adaptation repeated its native result.',
                               'I compare the listed packet-header source pairs; I do not claim an exhaustive transitive-header or product proof.']}
            check_source_pins(info,source32,source64)
            write_json(folder/'receipt.json',receipt)
        print(json.dumps({'receipt':output_pin(folder/'receipt.json',folder),'state':receipt['state'],
                          'expected_records':info['expected_records'],'output_child':folder.name}))
    except BaseException as error:
        if not (folder/'receipt.json').exists():
            write_json(folder/'receipt.json',{'state':'PREPARATION_FAILED_NO_EQUIVALENCE_CLAIM','error_type':type(error).__name__})
        raise


if __name__=='__main__':
    main()
