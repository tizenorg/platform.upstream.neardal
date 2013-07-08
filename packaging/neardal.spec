Name: neardal
Summary: Neard Abstraction Library (for Neard v0.7)
Version: 0.7.0
Release: 1.0
Group: Connectivity/NFC
License: LGPL-2.0
URL: https://github.com/connectivity/neardal.git
Source0: %{name}-%{version}.tar.bz2

Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires: python
BuildRequires: intltool
BuildRequires: libtool
BuildRequires: automake
BuildRequires: autoconf
BuildRequires: gettext-tools
BuildRequires: python-xml
BuildRequires: pkgconfig(glib-2.0) >= 2.30.0
BuildRequires: pkgconfig(dbus-glib-1)
BuildRequires: pkgconfig(dlog)

%description
This package provides simple C APIs to exchange datas with NFC daemon (Neard) present on the system.

%prep
%setup -q -n %{name}-%{version}

%build
autoreconf --force --install

%configure --disable-traces --prefix=/usr
make

%package devel
Summary:    Headers for neardal
Requires:   %{name} = %{version}-%{release}


%description devel
Development headers and libraries for neardal

%install
rm -rf %{buildroot}
%make_install

# executed after install
%post -p /sbin/ldconfig

# executed before uninstall
%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%license COPYING
%doc README AUTHORS NEWS

# libraries files
%{_libdir}/libneardal.so.0
%{_libdir}/libneardal.so.0.0.1

%changelog

%files devel
# headers files
%{_includedir}/neardal/*.h
# pkg-config files
%{_libdir}/pkgconfig/neardal.pc
%{_libdir}/libneardal.so
