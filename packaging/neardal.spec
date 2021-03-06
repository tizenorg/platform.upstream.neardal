Name:    neardal
Summary: Neard Abstraction Library (for Neard v0.7)
Version: 0.14
Release: 0
Group:   Network & Connectivity/NFC
License: LGPL-2.0
URL:     https://github.com/connectivity/neardal.git
Source0: %{name}-%{version}.tar.bz2
Source1001: %{name}.manifest

Requires(post):   /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires: python
BuildRequires: intltool
BuildRequires: libtool
BuildRequires: automake
BuildRequires: autoconf
BuildRequires: readline-devel
BuildRequires: gettext-tools
BuildRequires: python-xml
BuildRequires: pkgconfig(glib-2.0) >= 2.30.0
BuildRequires: pkgconfig(dbus-glib-1)
BuildRequires: pkgconfig(dlog)

%description
This package provides simple C APIs to exchange datas with NFC daemon (Neard) present on the system.

%prep
%setup -q -n %{name}-%{version}
cp %{SOURCE1001} .

%build
%reconfigure --disable-traces --prefix=/usr
%__make

%package devel
Summary:    Headers for neardal
Requires:   %{name} = %{version}-%{release}

%description devel
Development headers and libraries for neardal.

%install
rm -rf %{buildroot}
%make_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%manifest %{name}.manifest
%license COPYING
%doc README AUTHORS
%{_libdir}/libneardal.so.0
%{_libdir}/libneardal.so.0.0.*
%{_sysconfdir}/dbus-1/system.d/org.neardal.conf

%files devel
%manifest %{name}.manifest
%{_includedir}/neardal/*.h
%{_libdir}/pkgconfig/neardal.pc
%{_libdir}/libneardal.so
