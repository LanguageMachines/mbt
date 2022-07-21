[![GitHub build](https://github.com/LanguageMachines/mbt/actions/workflows/mbt.yml/badge.svg?branch=master)](https://github.com/LanguageMachines/mbt/actions/)
[![Language Machines Badge](http://applejack.science.ru.nl/lamabadge.php/mbt)](http://applejack.science.ru.nl/languagemachines/)
[![DOI](https://zenodo.org/badge/20526403.svg)](https://zenodo.org/badge/latestdoi/20526403)

===================================
Mbt: Tilburg Memory Based Tagger
===================================

    Mbt 3.2 (c) CLST/ILK/CNTS 1998-2016
    Centre for Language and Speech Technology, Radboud University Nijmegen
    Induction of Linguistic Knowledge Research Group, Tilburg University and
    Centre for Dutch Language and Speech, University of Antwerp

**Website:** https://languagemachines.github.io/mbt

Comments and bug-reports are welcome on our issue tracker at
https://github.com/LanguageMachines/mbt/issues, or by mailing
lamasoftware (at) science.ru.nl .

Mbt is distributed under the GNU Public Licence v3 (see the file COPYING)

This software has been tested on:
- Intel platform running several versions of Linux, including Ubuntu, Debian,
  Arch Linux, Fedora (both 32 and 64 bits)
- Apple platform running Mac OS X 10.10

Compilers:
- GCC. It is highly recommended to upgrade to at least GCC 4.8
- Clang

Contents of this distribution:
- sources
- Licensing information ( ``COPYING`` )
- Installation instructions ( ``INSTALL`` )
- Build system based on GNU Autotools
- example data files ( in the ``demos/`` directory )
- documentation ( in the ``docs/`` directory )

Dependencies:
To be able to succesfully build Timbl from the tarball, you need the
following pakages:
- pkg-config
- [timbl](https://github.com/LanguageMachines/timbl)
- [ticcutils](https://github.com/LanguageMachines/ticcutils)


To install Mbt, first consult whether your distribution's package manager has an up-to-date package.

To compile and install manually from source instead:

    $ bash bootstrap.sh
    $ ./configure
    $ make
    $ make install

This requires you have all the necessary dependencies. If you want to
automatically download and install the latest stable versions of the required
dependencies, then run `./build-deps.sh` prior to the above. You can pass a
target directory prefix as first argument and you may need to prepend `sudo` to
ensure you can install there.

A `Dockerfile` for a container build is also available, specify `--build-arg VERSION=development` if you want the latest
development version rather than the latest stable release as shipped with Alpine Linux.
