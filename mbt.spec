# $Id$
# $URL$

Summary: Memory Based Tagger
Name: mbt
Version: 3.2.1
Release: 1
License: GPL
Group: Applications/System
URL: http://ilk.uvt.nl/mbt/

Packager: Joost van Baal <joostvb-mbt@ad1810.com>
Vendor: ILK, http://ilk.uvt.nl/

Source: http://ilk.uvt.nl/downloads/pub/software/mbt-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

Requires: timbl
BuildRequires: timbl

%description

MBT is used for natural language processing; it is a memory-based
tagger-generator and tagger in one. The tagger-generator part can generate a
sequence tagger on the basis of a training set of tagged sequences; the tagger
part can tag new sequences. MBT can, for instance, be used to generate
part-of-speech taggers or chunkers for natural language processing.

Features: * Tagger generation: tagged text in, tagger out; * Optional
feedback loop: feed previous tag decision back to input of next decision; *
Easily customizable feature representation; can incorporate user-provided
features; * Automatic generation of separate sub-taggers for known words and
unknown words; * Can make use of full algorithmic parameters of TiMBL.

TiMBL is a product of the ILK (Induction of Linguistic Knowledge) research
group of the Tilburg University and the CNTS research group of the University
of Antwerp.

If you do scientific research in natural language processing, MBT will likely
be of use to you.

%prep
%setup

%build
%configure
%install
%{__rm} -rf %{buildroot}
%makeinstall
%{__rm} %{buildroot}%{_sysconfdir}/init.d/mbt

%clean
%{__rm} -rf %{buildroot}

# FIXME mbt > 3.2.1 won't install Makefile.am
%files
%defattr(-, root, root, 0755)
%doc AUTHORS ChangeLog NEWS README TODO
%{_datadir}/doc/%{name}/*.pdf
%{_datadir}/doc/%{name}/examples/*
%{_libdir}/libMbt*
%{_bindir}/Mbt*
%{_includedir}/%{name}/*.h
%{_includedir}/%{name}/Makefile.am
%{_libdir}/pkgconfig/mbt.pc

%changelog
* Mon Apr 5 2010 Joost van Baal <joostvb-timbl@ad1810.com> - 3.2.1-1
- Initial release.


