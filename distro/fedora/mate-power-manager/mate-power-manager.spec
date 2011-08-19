Summary: MATE power management service
Name: mate-power-manager
Version: 2.32.0
Release: 1%{?dist}
License: GPLv2+ and GFDL
Group: Applications/System
Source: http://matsusoft.com.ar/uploads/gnu-linux/mate/mate-power-manager.tar.gz

URL: http://projects.gnome.org/gnome-power-manager/

BuildRequires: mate-panel
BuildRequires: scrollkeeper
BuildRequires: mate-doc-utils
BuildRequires: desktop-file-utils
BuildRequires: gettext
BuildRequires: libtool
BuildRequires: cairo-devel
BuildRequires: libcanberra-devel
BuildRequires: libnotify-devel >= 0.5.0
BuildRequires: upower-devel >= 0.9.0
BuildRequires: intltool
BuildRequires: unique-devel >= 1.0.0
BuildRequires: glib2-devel >= 2.25.9
BuildRequires: GConf2-devel >= 2.31.1
BuildRequires: gtk2-devel >= 2.16.0
BuildRequires: dbus-glib-devel
BuildRequires: libwnck-devel
BuildRequires: control-center-devel >= 2.31.4
Requires: mate-icon-theme
Requires: libcanberra
Requires: dbus-x11
Requires: upower >= 0.9.0
Requires(post): scrollkeeper
Requires(postun): scrollkeeper
Provides: gnome-power-manager = %{version}
Provides: gnome-power-manager = %{version}
Conflicts: gnome-power-manager >= 3.0
# obsolete sub-package
Obsoletes: gnome-power-manager-extra <= 2.30.1
Provides: gnome-power-manager-extra

Patch0: dont-eat-the-logs.patch
Patch1: gpm-manager.patch
BuildRequires: autoconf automake libtool

%description
MATE Power Manager uses the information and facilities provided by UPower
displaying icons and handling user callbacks in an interactive MATE session.

%prep
%setup -q
%patch0 -p1 -b .logs
%patch1 -p1 -b .gpm

autoreconf -i -f

%build
%configure --disable-scrollkeeper \
	--disable-schemas-install \
	--disable-applets
make AM_LDFLAGS="-Wl,-O1,--as-needed"

# strip unneeded translations from .mo files
# ideally intltool (ha!) would do that for us
# http://bugzilla.gnome.org/show_bug.cgi?id=474987
cd po
grep -v ".*[.]desktop[.]in[.]in$\|.*[.]server[.]in[.]in$" POTFILES.in > POTFILES.keep
mv POTFILES.keep POTFILES.in
intltool-update --pot
for p in *.po; do
  msgmerge $p %{name}.pot > $p.out
  msgfmt -o `basename $p .po`.gmo $p.out
done

%install
make install DESTDIR=$RPM_BUILD_ROOT

desktop-file-install --vendor mate --delete-original                   \
  --dir $RPM_BUILD_ROOT%{_sysconfdir}/xdg/autostart                     \
  $RPM_BUILD_ROOT%{_sysconfdir}/xdg/autostart/mate-power-manager.desktop

# save space by linking identical images in translated docs
helpdir=$RPM_BUILD_ROOT%{_datadir}/mate/help/%{name}
for f in $helpdir/C/figures/*.png; do
  b="$(basename $f)"
  for d in $helpdir/*; do
    if [ -d "$d" -a "$d" != "$helpdir/C" ]; then
      g="$d/figures/$b"
      if [ -f "$g" ]; then
        if cmp -s $f $g; then
          rm "$g"; ln -s "../../C/figures/$b" "$g"
        fi
      fi
    fi
  done
done

%find_lang %{name} --with-gnome

%post
export GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source`
gconftool-2 --makefile-install-rule \
        %{_sysconfdir}/gconf/schemas/mate-power-manager.schemas >/dev/null || :
touch --no-create %{_datadir}/icons/hicolor
if [ -x /usr/bin/gtk-update-icon-cache ]; then
    gtk-update-icon-cache -q %{_datadir}/icons/hicolor &> /dev/null || :
fi
update-desktop-database %{_datadir}/applications &> /dev/null || :

%pre
if [ "$1" -gt 1 ]; then
    export GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source`
    gconftool-2 --makefile-uninstall-rule \
      %{_sysconfdir}/gconf/schemas/mate-power-manager.schemas &> /dev/null || :
fi

%preun
if [ "$1" -eq 0 ]; then
    export GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source`
    gconftool-2 --makefile-uninstall-rule \
      %{_sysconfdir}/gconf/schemas/mate-power-manager.schemas &> /dev/null || :
fi

%postun
touch --no-create %{_datadir}/icons/hicolor
if [ -x /usr/bin/gtk-update-icon-cache ]; then
    gtk-update-icon-cache -q %{_datadir}/icons/hicolor &> /dev/null || :
fi
update-desktop-database %{_datadir}/applications &> /dev/null || :

%files -f %{name}.lang
%defattr(-,root,root)
%doc AUTHORS COPYING README
%{_bindir}/*
%{_datadir}/applications/*.desktop
%{_datadir}/dbus-1/services/mate-power-manager.service
%{_datadir}/mate-power-manager/*.ui
%{_datadir}/mate-power-manager/icons/hicolor/*/*/*.*
%{_datadir}/icons/hicolor/*/apps/mate-brightness-applet.*
%{_datadir}/icons/hicolor/*/apps/mate-inhibit-applet.*
%{_datadir}/icons/hicolor/*/apps/mate-power-manager.*
%{_datadir}/icons/hicolor/*/apps/mate-power-statistics.*
%{_datadir}/omf/mate-power-manager
%{_datadir}/polkit-1/actions/org.mate.power.policy
%dir %{_datadir}/mate-power-manager
#%{_datadir}/mate-2.0/ui/*.xml
#%{_libdir}/bonobo/servers/*.server
#%{_libexecdir}/*
%config(noreplace) %{_sysconfdir}/gconf/schemas/*.schemas
%{_mandir}/man1/*.1.gz
%{_sbindir}/*
%{_sysconfdir}/xdg/autostart/*.desktop

%changelog
* Tue Jun 28 2011 Juan Rodriguez <nushio@fedoraproject.org> 2.32.0-1
- Initial release of Mate-Power-Manager
