Name:           dchat
Version:        1.0
Release:        1%{?dist}
Summary:        A small and dumb message exchange application

License: 		MIT       
URL:     		https://github.com/theMavl/INNO2019OMP_Chat/
Source0: 		dchat-1.0.tar.gz      

Requires(post): info
Requires(preun): info     

%description
A small and dumb message exchange application

%prep
%setup

%build
make PREFIX=/usr %{?_smp_mflags}

%install
make PREFIX=/usr DESTDIR=%{?buildroot} install

%clean
rm -rf %{buildroot}

%files
%{_bindir}/dchat

