# >> macros
%define neardal_dir %{_libdir}
%define neardal_pkg %{_libdir}/pkgconfig
%define neardal_inc %{_includedir}/neardal

%define glib2_version   		2.30.0
# << macros

Name: neardal-tizen
Summary: Neard Abstraction Library (for Neard v0.7)
Version: 0.7.0
Release: 1.0
Group: System/Libraries
License: LGPLv2
URL: https://github.com/connectivity/neardal.git
Source0: %{name}-%{version}.tar.bz2

Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires: python
BuildRequires: intltool >= %{intltool_version}
BuildRequires: libtool
BuildRequires: automake
BuildRequires: autoconf
BuildRequires: gettext-tools
BuildRequires: python-xml
BuildRequires: pkgconfig(glib-2.0) >= %{glib2_version}
BuildRequires: pkgconfig(dbus-glib-1)

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
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}


%description devel
Development headers and libraries for neardal

%package ncl
Summary:    Neardal Command Line: Simple command line interpretor for neardal/Neard
Group:      Tools
Requires:   %{name} = %{version}-%{release}


%description ncl
Neardal Command Line: Simple command line interpretor for neardal/Neard

%install
rm -rf %{buildroot}
%make_install

# executed after install
%post
/sbin/ldconfig

# executed before uninstall
%postun
/sbin/ldconfig

# No locale
# %%find_lang %%{name}
# %%files -f %%{name}.lang

%files
%defattr(-,root,root,-)
%doc README AUTHORS NEWS COPYING

# libraries files
%{neardal_dir}/libneardal.so
%{neardal_dir}/libneardal.so.0
%{neardal_dir}/libneardal.so.0.0.1

%changelog

%files devel
# headers files
%{neardal_inc}/*.h
# pkg-config files
%{neardal_pkg}/neardal.pc

%files ncl
%{_bindir}/ncl
