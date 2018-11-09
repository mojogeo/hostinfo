Name: hostinfo
Summary: Host Information Service
Version: %{getenv:HOSTINFO_BUILD_VERSION}
Release: 1%{?dist}
License: MIT
Group: *Applications/Network*
URL: https://github.com/mojogeo/hostinfo
Source: %{name}-%{version}.tar.gz
Packager: Chris Juszak
Requires: glibc openssl-libs krb5-libs libcom_err zlib keyutils-libs libselinux pcre
BuildRoot: %{_tmppath}/%{name}-root

%description
Host information service (hostinfo).

%prep
%setup -q

%install
rm -fr %{buildroot}

# Create required directories
mkdir -p %{buildroot}/usr/bin

# Install hostinfo binary to /usr/bin
install -p -m 755 hostinfo %{buildroot}/usr/bin/

%clean
rm -fr %{buildroot}

%files
%defattr(-,root,root)
/usr/bin/hostinfo
