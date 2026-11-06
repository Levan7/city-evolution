import pathlib,subprocess,shutil
r=pathlib.Path('preview13-work').resolve();s=r/'sysroot'
for p in (r/'debs').glob('*.deb'):subprocess.run(['dpkg-deb','-x',str(p),str(s)],check=True)
for p in (s/'usr/lib/x86_64-linux-gnu').glob('*.so'):
 if p.is_symlink() and not p.exists():
  target=pathlib.Path('/usr/lib/x86_64-linux-gnu')/p.readlink().name
  if target.exists():shutil.copy2(target.resolve(),p.parent/target.name)
(r/'env.sh').write_text('export TASK_ROOT='+str(r)+'\nexport PATH="$TASK_ROOT/sysroot/usr/bin:$PATH"\nexport LD_LIBRARY_PATH="$TASK_ROOT/sysroot/usr/lib/x86_64-linux-gnu:$TASK_ROOT/prefix/lib:$TASK_ROOT/prefix/lib64:${LD_LIBRARY_PATH:-}"\nexport PKG_CONFIG_PATH="$TASK_ROOT/prefix/lib/pkgconfig:$TASK_ROOT/prefix/lib64/pkgconfig:$TASK_ROOT/sysroot/usr/lib/x86_64-linux-gnu/pkgconfig:$TASK_ROOT/sysroot/usr/share/pkgconfig"\nexport CMAKE_PREFIX_PATH="$TASK_ROOT/prefix;$TASK_ROOT/sysroot/usr"\nexport CFLAGS="-I$TASK_ROOT/sysroot/usr/include"\nexport CXXFLAGS="$CFLAGS"\nexport LDFLAGS="-L$TASK_ROOT/sysroot/usr/lib/x86_64-linux-gnu -Wl,-rpath-link,$TASK_ROOT/sysroot/usr/lib/x86_64-linux-gnu"\n')
