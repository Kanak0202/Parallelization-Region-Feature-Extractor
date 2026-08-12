#!/usr/bin/env python3
from __future__ import annotations
import argparse, re, sys
from pathlib import Path

MODE = "serial"
OUT_MODE = "serial"
BEGIN = re.compile(r'^\s*#\s*pragma\s+capc\s+profitability_region\s+begin\b')
END = re.compile(r'^\s*#\s*pragma\s+capc\s+profitability_region\s+end\b')
OMP = re.compile(r'^\s*#\s*pragma\s+omp\b', re.I)
ACC = re.compile(r'^\s*#\s*pragma\s+acc\b', re.I)

def directive(lines, i):
    parts=[lines[i]]; j=i
    while parts[-1].rstrip().endswith('\\') and j+1<len(lines):
        j+=1; parts.append(lines[j])
    return ''.join(parts), j

def transfer_kind(text):
    s=' '.join(text.lower().replace('\\',' ').split())
    if MODE=='omp45':
        if 'target update' in s:
            if re.search(r'\bfrom\s*\(',s): return 'D2H'
            if re.search(r'\bto\s*\(',s): return 'H2D'
        if 'target enter data' in s and re.search(r'\bmap\s*\(\s*(to|tofrom)\s*:',s): return 'H2D'
        if 'target exit data' in s and re.search(r'\bmap\s*\(\s*(from|tofrom)\s*:',s): return 'D2H'
    if MODE=='openacc':
        if ' update ' in f' {s} ':
            if re.search(r'\b(self|host)\s*\(',s): return 'D2H'
            if re.search(r'\bdevice\s*\(',s): return 'H2D'
        if 'enter data' in s and re.search(r'\b(copyin|copy)\s*\(',s): return 'H2D'
        if 'exit data' in s and re.search(r'\b(copyout|copy)\s*\(',s): return 'D2H'
    return None

def add_omp_header(src):
    if re.search(r'^\s*#\s*include\s*[<\"]omp\.h[>\"]',src,re.M): return src
    ms=list(re.finditer(r'^\s*#\s*include[^\n]*\n',src,re.M))
    p=ms[-1].end() if ms else 0
    return src[:p]+'#include <omp.h>\n'+src[p:]

def support(nr, kinds):
    rs=max(nr,1); ts=max(len(kinds),1)
    labels=', '.join('"'+x+'"' for x in kinds) or '""'
    return f'''\n/* CAPC timing support: generated */
static double __capc_rt[{rs}]={{0}}; static unsigned long long __capc_rc[{rs}]={{0}};
static double __capc_tt[{ts}]={{0}}; static unsigned long long __capc_tc[{ts}]={{0}};
static const char *__capc_tk[{ts}]={{{labels}}};
static void __capc_report(void){{
 int q; double h=0,d=0; unsigned long long hc=0,dc=0;
 printf("\\n===== CAPC TIMING REPORT ({MODE}) =====\\n");
 for(q=0;q<{nr};q++) if(__capc_rc[q]) printf("Region %d: total=%0.9f s, executions=%llu, average=%0.9f s\\n",q,__capc_rt[q],__capc_rc[q],__capc_rt[q]/(double)__capc_rc[q]);
 for(q=0;q<{len(kinds)};q++) if(__capc_tc[q]){{
   printf("%s transfer %d: total=%0.9f s, executions=%llu, average=%0.9f s\\n",__capc_tk[q],q,__capc_tt[q],__capc_tc[q],__capc_tt[q]/(double)__capc_tc[q]);
   if(__capc_tk[q][0]=='H'){{h+=__capc_tt[q];hc+=__capc_tc[q];}} else {{d+=__capc_tt[q];dc+=__capc_tc[q];}}
 }}
 if(hc) printf("H2D summary: total=%0.9f s, transfers=%llu, average=%0.9f s\\n",h,hc,h/(double)hc);
 if(dc) printf("D2H summary: total=%0.9f s, transfers=%llu, average=%0.9f s\\n",d,dc,d/(double)dc);
 printf("=======================================\\n");
}}
/* end CAPC timing support */\n\n'''

def annotate(src):
    src=add_omp_header(src); lines=src.splitlines(True)
    nr=sum(bool(BEGIN.match(x)) for x in lines)
    kinds=[]
    if MODE in ('omp45','openacc'):
        i=0
        while i<len(lines):
            cand=(MODE=='omp45' and OMP.match(lines[i])) or (MODE=='openacc' and ACC.match(lines[i]))
            if cand:
                text,j=directive(lines,i); k=transfer_kind(text)
                if k:kinds.append(k)
                i=j+1
            else:i+=1
    out=[]; stack=[]; rid=0; tid=0; i=0
    while i<len(lines):
        line=lines[i]; ind=re.match(r'^\s*',line).group(0)
        if BEGIN.match(line):
            stack.append(rid); out += [line,f'{ind}double __capc_rs_{rid}=omp_get_wtime();\n']; rid+=1; i+=1; continue
        if END.match(line):
            if not stack: raise ValueError('CAPC end without begin')
            r=stack.pop(); out += [f'{ind}__capc_rt[{r}]+=omp_get_wtime()-__capc_rs_{r};\n',f'{ind}__capc_rc[{r}]++;\n',line]; i+=1; continue
        cand=(MODE=='omp45' and OMP.match(line)) or (MODE=='openacc' and ACC.match(line))
        if cand:
            text,j=directive(lines,i); k=transfer_kind(text)
            if k:
                out.append(f'{ind}double __capc_ts_{tid}=omp_get_wtime();\n'); out.extend(lines[i:j+1]);
                out += [f'{ind}__capc_tt[{tid}]+=omp_get_wtime()-__capc_ts_{tid};\n',f'{ind}__capc_tc[{tid}]++;\n']; tid+=1; i=j+1; continue
        out.append(line); i+=1
    if stack: raise ValueError('CAPC begin without end')
    text=''.join(out)
    ms=list(re.finditer(r'^\s*#\s*include[^\n]*\n',text,re.M)); p=ms[-1].end() if ms else 0
    text=text[:p]+support(nr,kinds)+text[p:]
    m=re.search(r'\b(?:int|signed\s+int)\s+main\s*\([^)]*\)\s*\{',text,re.M)
    if not m: raise ValueError('main() not found')
    return text[:m.end()]+'\n    atexit(__capc_report);'+text[m.end():]

def project_root(inp):
    for p in (inp,*inp.parents):
        if (p/'outputs').is_dir(): return p
    return Path.cwd()

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('input_directory',type=Path); ap.add_argument('--output-root',type=Path)
    a=ap.parse_args(); inp=a.input_directory.expanduser().resolve()
    if not inp.is_dir(): print('Invalid input directory:',inp,file=sys.stderr); return 2
    root=a.output_root.expanduser().resolve() if a.output_root else project_root(inp)/'outputs'/'annotated'
    dest=root/OUT_MODE/inp.name; files=sorted(inp.rglob('*.c'))
    if not files: print('No .c files found',file=sys.stderr); return 2
    ok=bad=0
    for f in files:
        o=dest/f.relative_to(inp); o.parent.mkdir(parents=True,exist_ok=True)
        try: o.write_text(annotate(f.read_text(encoding='utf-8')),encoding='utf-8'); print('[OK]',f,'->',o); ok+=1
        except Exception as e: print('[ERROR]',f,e,file=sys.stderr); bad+=1
    print(f'Completed {MODE}: {ok} written, {bad} failed. Output: {dest}')
    return 1 if bad else 0
if __name__=='__main__': raise SystemExit(main())
