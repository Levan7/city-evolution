from pathlib import Path
import subprocess,concurrent.futures,tarfile
r=Path('preview15-work').resolve(); (r/'deps').mkdir(exist_ok=True)
def get(item):
 repo,ref,folder=item; p=r/'deps'/(folder+'.tar.gz')
 subprocess.run(['curl','-fLsS','--retry','2','--max-time','150','https://codeload.github.com/'+repo+'/tar.gz/'+ref,'-o',str(p)],check=True)
 with tarfile.open(p) as t:t.extractall(r/'deps',filter='data')
 print(folder,flush=True)
with concurrent.futures.ThreadPoolExecutor(4) as e:list(e.map(get,[('libsdl-org/SDL','refs/tags/release-3.4.0','SDL-release-3.4.0'),('libsdl-org/SDL_image','refs/tags/release-3.2.4','SDL_image-release-3.2.4'),('libsdl-org/SDL_ttf','refs/tags/release-3.2.2','SDL_ttf-release-3.2.2'),('libsdl-org/SDL_mixer','refs/heads/main','SDL_mixer-main')]))
p=r/'appimagetool';subprocess.run(['curl','-fLsS','--retry','2','--max-time','120','https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage','-o',str(p)],check=True);p.chmod(0o755)
subprocess.run([str(p),'--appimage-extract'],cwd=r,check=True,stdout=subprocess.DEVNULL)
