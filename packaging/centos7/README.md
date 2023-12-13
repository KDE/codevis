Automated build scripts for Codevis
---

Run:
```
export CODEVIS_PKG_DIST=centos7
export CODEVIS_PKG_OUTDIR=$(pwd)/codevis-build-${CODEVIS_PKG_DIST}/
export CODEVIS_PKG_IMGNAME=codevis-${CODEVIS_PKG_DIST}
mkdir -p ${CODEVIS_PKG_OUTDIR}
docker build . -t ${CODEVIS_PKG_IMGNAME}
docker run -v ${CODEVIS_PKG_OUTDIR}:/artifacts -t ${CODEVIS_PKG_IMGNAME}
cp codevis.sh ${CODEVIS_PKG_OUTDIR}
```

Notes:
- This will build the **command line tools only**, not the GUI.
- Plugins are **DISABLED**.
