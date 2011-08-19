%define gstreamer_version	0.10.1
%define gstreamer_plugins_base_version	0.10.1
%define gstreamer_plugins_good_version	0.10.0

Summary: Movie player for MATE 2
Name: totem
Version: 2.32.0
Release: 1
License: GPL
Group: Applications/Multimedia
URL: http://www.mate.org/projects/totem/
Source0: http://ftp.mate.org/pub/MATE/sources/totem/0.99/totem-%{version}.tar.bz2
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
Requires(post): MateConf2
Requires: mate-desktop >= 2.6.0
Requires: gstreamer >= %gstreamer_version
Requires: gstreamer-plugins-base >= %gstreamer_plugins_base_version
Requires: gstreamer-plugins-good >= %gstreamer_plugins_good_version

Requires: iso-codes
BuildRequires: gcc-c++, pkgconfig, gettext, scrollkeeper
BuildRequires: gstreamer-devel >= %gstreamer_version
BuildRequires: gstreamer-plugins-base-devel >= %gstreamer_plugins_base_version
BuildRequires: mate-desktop-devel >= 2.6.0, mate-vfs2-devel, libglade2-devel
BuildRequires: caja-devel
BuildRequires: perl-XML-Parser
BuildRequires: iso-codes-devel
Obsoletes: caja-media

%description
Totem is simple movie player for the Mate desktop. It features a
simple playlist, a full-screen mode, seek and volume controls, as well as
a pretty complete keyboard navigation.

%package mozplugin
Summary: Mozilla plugin for Totem
Group: Applications/Internet

%description mozplugin
Totem is simple movie player for the Mate desktop. The mozilla plugin
for totem allows totem to be embeded into a web browser. 

%prep
%setup -q

%build
export MOZILLA_PLUGINDIR=%{_libdir}/mozilla 
%configure --enable-gstreamer=yes --enable-mozilla --enable-nvtv

make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
export MATECONF_DISABLE_MAKEFILE_SCHEMA_INSTALL=1
make install DESTDIR=$RPM_BUILD_ROOT

# no static libs and libtool archives either
rm -f $RPM_BUILD_ROOT%{_libdir}/*.{a,la}
rm -f $RPM_BUILD_ROOT%{_libdir}/caja/extensions-1.0/*.{a,la}
rm -f $RPM_BUILD_ROOT%{_libdir}/mozilla/plugins/*.{a,la}
rm -f $RPM_BUILD_ROOT%{_libdir}/totem/plugins/*/*.{a,la}

%clean
rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig
update-desktop-database %{_datadir}/applications
export MATECONF_CONFIG_SOURCE=`mateconftool-2 --get-default-source`

SCHEMAS="totem.schemas totem-handlers.schemas totem-video-thumbnail.schemas" 

for S in $SCHEMAS; do
  mateconftool-2 --makefile-install-rule \
    %{_sysconfdir}/mateconf/schemas/$S \
     >/dev/null || :
done

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%doc AUTHORS COPYING ChangeLog NEWS README TODO
%config %{_sysconfdir}/mateconf/schemas/*.schemas
%{_bindir}/%{name}
%{_bindir}/totem-video-indexer
%{_datadir}/applications/%{name}.desktop
%{_datadir}/mate/help/%{name}/
%{_datadir}/omf/%{name}/
%{_datadir}/%{name}/
%{_datadir}/icons
%{_libdir}/caja/extensions-1.0/*.so*
%{_bindir}/%{name}-video-thumbnailer
%{_bindir}/%{name}-audio-preview
%{_mandir}/man1/%{name}.1*
%{_mandir}/man1/totem-video-thumbnailer.1.gz
%{_datadir}/locale
%{_libdir}/totem/plugins/gromit/gromit.totem-plugin
%{_libdir}/totem/plugins/gromit/libgromit.so
%{_libdir}/totem/plugins/ontop/libontop.so
%{_libdir}/totem/plugins/ontop/ontop.totem-plugin
%{_libdir}/totem/plugins/screensaver/libscreensaver.so
%{_libdir}/totem/plugins/screensaver/screensaver.totem-plugin
%{_libdir}/totem/plugins/skipto/libskipto.so
%{_libdir}/totem/plugins/skipto/skipto.totem-plugin
%{_libdir}/totem/plugins/skipto/skipto.ui
%{_libdir}/totem/plugins/media-player-keys/libmedia_player_keys.so
%{_libdir}/totem/plugins/media-player-keys/media-player-keys.totem-plugin
%{_libdir}/totem/plugins/properties/libmovie-properties.so
%{_libdir}/totem/plugins/properties/movie-properties.totem-plugin
%{_libdir}/totem/plugins/totem/*.py
%{_libdir}/totem/plugins/totem/*.pyc
%{_libdir}/totem/plugins/totem/*.pyo
%{_libdir}/totem/plugins/youtube/*.py
%{_libdir}/totem/plugins/youtube/*.pyc
%{_libdir}/totem/plugins/youtube/*.pyo
%{_libdir}/totem/plugins/youtube/youtube.totem-plugin
%{_libdir}/totem/plugins/youtube/youtube.ui
%{_libdir}/totem/plugins/pythonconsole/*.py
%{_libdir}/totem/plugins/pythonconsole/*.pyc
%{_libdir}/totem/plugins/pythonconsole/*.pyo
%{_libdir}/totem/plugins/pythonconsole/pythonconsole.totem-plugin
%{_libdir}/totem/plugins/jamendo/*.py
%{_libdir}/totem/plugins/jamendo/*.pyc
%{_libdir}/totem/plugins/jamendo/*.pyo
%{_libdir}/totem/plugins/jamendo/jamendo.totem-plugin
%{_libdir}/totem/plugins/jamendo/jamendo.glade
%{_libexecdir}/totem/totem-bugreport.*

%files mozplugin
%defattr(-, root, root)
%{_libdir}/mozilla/libtotem*
%{_libexecdir}/totem-plugin-viewer

%changelog
* Tue Nov 25 2008 David JEAN LOUIS <izimobil@gmail.com>
- Added jamendo python plugin

* Sat Mar 1 2008 Christian Schaller <christian.schaller@collabora.co.uk>
- Update for new plugins and removed pl-parser

* Fri May 18 2007 Christian Schaller <christian@fluendo.com>
- Update for plugins system

* Mon Feb 13 2006 Matthias Clasen <mclasen@redhat.com> - 1.3.91-1
- Update to 1.3.91

* Fri Feb 10 2006 Jesse Keating <jkeating@redhat.com> - 1.3.90-2.1
- bump again for double-long bug on ppc(64)

* Thu Feb  9 2006 Matthias Clasen <mclasen@redhat.com> - 1.3.90-2
- Rebuild

* Tue Feb 07 2006 Jesse Keating <jkeating@redhat.com> - 1.3.90-1.1
- rebuilt for new gcc4.1 snapshot and glibc changes

* Mon Jan 30 2006 Matthias Clasen <mclasen@redhat.com> - 1.3.90-1
- Update to 1.3.90

* Fri Jan 20 2006 Matthias Clasen <mclasen@redhat.com> - 1.3.1-1
- Update to 1.3.1

* Fri Jan 06 2006 John (J5) Palmieri <johnp@redhat.com> 1.3.0-3
- Build with gstreamer 0.10
- Enable the mozilla plugin

* Thu Jan 05 2006 John (J5) Palmieri <johnp@redhat.com> 1.3.0-2
- GStreamer has been split into gstreamer08 and gstreamer (0.10) packages
  we need gstreamer08 for now

* Thu Dec 20 2005 Matthias Clasen <mclasen@redhat.com> 1.3.0-1
- Update to 1.3.0

* Thu Dec 15 2005 Matthias Clasen <mclasen@redhat.com> 1.2.1-1
- Update to 1.2.1

* Fri Dec 09 2005 Jesse Keating <jkeating@redhat.com>
- rebuilt

* Wed Oct 26 2005 John (J5) Palmieri <johnp@redhat.com> - 1.2.0-1
- Update to 1.2.0

* Tue Oct 25 2005 Matthias Clasen <mclasen@redhat.com> - 1.1.5-1
- Update to 1.1.5

* Tue Aug 18 2005 John (J5) Palmieri <johnp@redhat.com> - 1.1.4-1
- Update to upstream version 1.1.4 and rebuild
- Don't build with caja-cd-burner on s390 platforms

* Fri Jul 22 2005 Colin Walters <walters@redhat.com> - 1.1.3-1
- Update to upstream version 1.1.2

* Wed Jun 29 2005 John (J5) Palmieri <johnp@redhat.com> - 1.1.2-1
- Update to upstream version 1.1.2

* Tue May 17 2005 John (J5) Palmieri <johnp@redhat.com> - 1.0.2-1
- Update to upstream version 1.0.2 to fix minor bugs
- Register the thumbnail and handlers schemas

* Tue Feb 29 2005 John (J5) Palmieri <johnp@redhat.com> - 1.0.1-1
- Update to upstream version 1.0.1
- Break out devel package

* Mon Feb 21 2005 Bill Nottingham <notting@redhat.com> - 0.101-4
- fix %%post

* Wed Feb  2 2005 Matthias Clasen <mclasen@redhat.com> - 0.101-3
- Obsolete caja-media
- Install property page and thumbnailer

* Wed Feb  2 2005 Matthias Clasen <mclasen@redhat.com> - 0.101-2
- Update to 0.101
 
* Mon Jan 03 2005 Colin Walters <walters@redhat.com> - 0.100-2
- Grab patch totem-0.100-desktopfile.patch from CVS to fix
  missing menu entry (144088)
- Remove workaround for desktop file being misinstalled, fixed
  by above patch

* Mon Jan 03 2005 Colin Walters <walters@redhat.com> - 0.100-1
- New upstream version 0.100

* Sun Dec  5 2004 Bill Nottingham <notting@redhat.com> - 0.99.22-1
- update to 0.99.22

* Thu Oct 28 2004 Colin Walters <walters@redhat.com> - 0.99.19-2
- Add patch to remove removed items from package from help

* Thu Oct 14 2004 Colin Walters <walters@redhat.com> - 0.99.19-1
- New upstream 0.99.19
  - Fixes crasher with CD playback (see NEWS)

* Tue Oct 12 2004 Alexander Larsson <alexl@redhat.com> - 0.99.18-2
- Call update-desktop-database in post

* Tue Oct 12 2004 Alexander Larsson <alexl@redhat.com> - 0.99.18-1
- update to 0.99.18

* Wed Oct  6 2004 Alexander Larsson <alexl@redhat.com> - 
- Initial version, based on specfile by Matthias Saou <http://freshrpms.net/>

