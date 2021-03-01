Name: imagevis3d
Summary: Desktop volume rendering application for large data
Version: trunk
License: MIT
Release: 0
Group: unsorted
Provides: imagevis3d
Autoreqprov: yes
Source: imagevis3d-svn.tar.gz
Vendor: SCI Institute
Packager: Tom Fogal <tfogal@sci.utah.edu>
%description
ImageVis3D is a volume rendering application specifically designed to
render large data.  This is achieved by splitting the dataset into
multiple levels of detail (LoD), with each level itself decomposed
into multiple bricks (atomic rendering primitive).  Interaction occurs
at the coarsest LoD, which can be rendered instantaneously on almost
all modern systems.  After a configurable delay, ImageVis3D will
successively render finer levels of detail, until the data are visible
at their native resolution.

Development of ImageVis3D is/was sponsored by the NIH/NCRR Center for
Integrative Biomedical Computing (CIBC), and the DOE Visualization And Analytics
Center for Enabling Technologies (VACET).

%prep
if test -d imagevis3d ; then
  (cd imagevis3d && svn update)
else
  svn co https://gforge.sci.utah.edu/svn/imagevis3d
fi
rm -fr imagevis3d-build
cp -r imagevis3d imagevis3d-build
cd imagevis3d-build
CF="-fvisibility=hidden -Wall -Wextra"
CFX="${CF} -fvisibility-inlines-hidden"
qmake \
  QMAKE_CFLAGS="${CF}" \
  QMAKE_CXXFLAGS="${CFX}" \
  -recursive

%build
cd imagevis3d-build
make -j4

%install
mkdir -p %{buildroot}%{_bindir} %{buildroot}%{_mandir}/man1
mkdir -p %{buildroot}%{_datadir}/imagevis3d/shaders
cd imagevis3d-build
cp Build/ImageVis3D %{buildroot}%{_bindir}/imagevis3d
mkdir -p %{buildroot}%{_mandir}/man1
gzip -9c doc/imagevis3d.1 > %{buildroot}%{_mandir}/man1/imagevis3d.1.gz
gzip -9c doc/uvfconvert.1 > %{buildroot}%{_mandir}/man1/uvfconvert.1.gz
mkdir -p %{buildroot}%{_datadir}/imagevis3d/shaders
install --mode 644 Tuvok/Shaders/* %{buildroot}%{_datadir}/imagevis3d/shaders

%files
%{_bindir}/imagevis3d
%{_datadir}/imagevis3d/shaders/
%{_datadir}/imagevis3d/shaders/1D-slice-FS.glsl
%{_datadir}/imagevis3d/shaders/2D-slice-FS.glsl
%{_datadir}/imagevis3d/shaders/BBox-FS.glsl
%{_datadir}/imagevis3d/shaders/BBox-VS.glsl
%{_datadir}/imagevis3d/shaders/BTF.glsl
%{_datadir}/imagevis3d/shaders/clip-plane.glsl
%{_datadir}/imagevis3d/shaders/Compose-AF-FS.glsl
%{_datadir}/imagevis3d/shaders/Compose-Anaglyphs-FS.glsl
%{_datadir}/imagevis3d/shaders/Compose-Color-FS.glsl
%{_datadir}/imagevis3d/shaders/Compose-CV-FS.glsl
%{_datadir}/imagevis3d/shaders/Compose-FS.glsl
%{_datadir}/imagevis3d/shaders/Compose-SBS-FS.glsl
%{_datadir}/imagevis3d/shaders/Compose-Scanline-FS.glsl
%{_datadir}/imagevis3d/shaders/Compositing.glsl
%{_datadir}/imagevis3d/shaders/FTB.glsl
%{_datadir}/imagevis3d/shaders/GLRaycaster-1D-FS.glsl
%{_datadir}/imagevis3d/shaders/GLRaycaster-1D-light-FS.glsl
%{_datadir}/imagevis3d/shaders/GLRaycaster-2D-FS.glsl
%{_datadir}/imagevis3d/shaders/GLRaycaster-2D-light-FS.glsl
%{_datadir}/imagevis3d/shaders/GLRaycaster-Color-FS.glsl
%{_datadir}/imagevis3d/shaders/GLRaycaster-frontfaces-FS.glsl
%{_datadir}/imagevis3d/shaders/GLRaycaster-ISO-CV-FS.glsl
%{_datadir}/imagevis3d/shaders/GLRaycaster-ISO-FS.glsl
%{_datadir}/imagevis3d/shaders/GLRaycaster-MIP-Rot-FS.glsl
%{_datadir}/imagevis3d/shaders/GLRaycasterNoTransform-VS.glsl
%{_datadir}/imagevis3d/shaders/GLRaycaster-VS.glsl
%{_datadir}/imagevis3d/shaders/GLSBVR-1D-FS.glsl
%{_datadir}/imagevis3d/shaders/GLSBVR-1D-light-FS.glsl
%{_datadir}/imagevis3d/shaders/GLSBVR-2D-FS.glsl
%{_datadir}/imagevis3d/shaders/GLSBVR-2D-light-FS.glsl
%{_datadir}/imagevis3d/shaders/GLSBVR-Color-FS.glsl
%{_datadir}/imagevis3d/shaders/GLSBVR-ISO-FS.glsl
%{_datadir}/imagevis3d/shaders/GLSBVR-Mesh-1D-FS.glsl
%{_datadir}/imagevis3d/shaders/GLSBVR-Mesh-1D-light-FS.glsl
%{_datadir}/imagevis3d/shaders/GLSBVR-Mesh-2D-FS.glsl
%{_datadir}/imagevis3d/shaders/GLSBVR-Mesh-2D-light-FS.glsl
%{_datadir}/imagevis3d/shaders/GLSBVR-Mesh-VS.glsl
%{_datadir}/imagevis3d/shaders/GLSBVR-MIP-Rot-FS.glsl
%{_datadir}/imagevis3d/shaders/GLSBVR-VS.glsl
%{_datadir}/imagevis3d/shaders/lighting.glsl
%{_datadir}/imagevis3d/shaders/Mesh-FS.glsl
%{_datadir}/imagevis3d/shaders/Mesh-VS.glsl
%{_datadir}/imagevis3d/shaders/MIP-slice-FS.glsl
%{_datadir}/imagevis3d/shaders/RefineIsosurface.glsl
%{_datadir}/imagevis3d/shaders/SlicesIn3D.glsl
%{_datadir}/imagevis3d/shaders/Transfer-FS.glsl
%{_datadir}/imagevis3d/shaders/Transfer-MIP-FS.glsl
%{_datadir}/imagevis3d/shaders/Transfer-VS.glsl
%{_datadir}/imagevis3d/shaders/Volume2D-linear.glsl
%{_datadir}/imagevis3d/shaders/Volume2D-nearest.glsl
%{_datadir}/imagevis3d/shaders/Volume3D.glsl
%{_datadir}/imagevis3d/shaders/vr-col-tfqn.glsl
%{_datadir}/imagevis3d/shaders/vr-col-tfqn-lit.glsl
%{_datadir}/imagevis3d/shaders/vr-scal-tfqn.glsl
%{_datadir}/imagevis3d/shaders/vr-scal-tfqn-lit.glsl
%doc %{_mandir}/man1/imagevis3d.1.gz
%doc %{_mandir}/man1/uvfconvert.1.gz
