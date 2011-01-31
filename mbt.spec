# $Id$
# $URL$

Summary: Memory Based Tagger
Name: mbt
Version: 3.2.4
Release: 1
License: GPL
Group: Applications/System
URL: http://ilk.uvt.nl/mbt/

Packager: Joost van Baal <joostvb-mbt@ad1810.com>
Vendor: ILK, http://ilk.uvt.nl/

# rpm and src.rpm should end up in
# zeus:/var/www/ilk/packages.
# orig source available from
# zeus:/var/www/ilk/downloads/pub/software/mbt-3.2.2.tar.gz
Source: http://ilk.uvt.nl/downloads/pub/software/mbt-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

Requires: timbl
BuildRequires: gcc-c++, timbl, timblserver

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

MBT is a product of the ILK Research Group (Tilburg University, The
Netherlands) and the CLiPS Research Centre (University of Antwerp, Belgium).

If you do scientific research in natural language processing, MBT will likely
be of use to you.

%prep
%setup

%build
%configure
%install
%{__rm} -rf %{buildroot}
%makeinstall

%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-, root, root, 0755)
%doc AUTHORS ChangeLog NEWS README TODO
%{_datadir}/doc/%{name}/examples/*
%{_libdir}/libMbt*
%{_bindir}/Mbt*
%{_includedir}/%{name}/*.h
%{_libdir}/pkgconfig/mbt.pc
%{_mandir}/man*/Mbt*

%changelog
* Mon Jan 31 2011 Joost van Baal <joostvb-timbl@ad1810.com> - 3.2.4-1
- New upstream release
  + No longer ships pdf manual; does ship Mbt(1).
  + No longer ships init script.
* Thu Aug 19 2010 Joost van Baal <joostvb-timbl@ad1810.com> - 3.2.2.99.1
- New upstream SVN snapshot release
* Wed Aug 18 2010 Joost van Baal <joostvb-timbl@ad1810.com> - 3.2.2-1
- Unpublished release.
- New stable upstream release, 2010-06-02
  + Now needs new timblserver for building
* Mon Apr 5 2010 Joost van Baal <joostvb-timbl@ad1810.com> - 3.2.1-1
- Initial unpublished release.

