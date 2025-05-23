[![License](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://gitlab.xfce.org/apps/parole/-/blob/master/COPYING)

# parole

Parole is a modern simple media player based on the GStreamer framework and 
written to fit well in the Xfce desktop. Parole features playback of local 
media files, DVD/CD and live streams.

Parole is extensible via plugins, for a complete how-to on writing a plugin for
Parole, see the Plugins API documentation and the plugins directory which 
contains some useful examples.

----

### Homepage

[Parole documentation](https://docs.xfce.org/apps/parole/start)

### Changelog

See [NEWS](https://gitlab.xfce.org/apps/parole/-/blob/master/NEWS) for details on changes and fixes made in the current release.

### Source Code Repository

[Parole source code](https://gitlab.xfce.org/apps/parole)

### Download a Release Tarball

[Parole archive](https://archive.xfce.org/src/apps/parole)
    or
[Parole tags](https://gitlab.xfce.org/apps/parole/-/tags)

### Installation

From source code repository: 

    % cd parole
    % meson setup build
    % meson compile -C build
    % meson install -C build

From release tarball:

    % tar xf parole-<version>.tar.xz
    % cd parole-<version>
    % meson setup build
    % meson compile -C build
    % meson install -C build

### Reporting Bugs

Visit the [reporting bugs](https://docs.xfce.org/apps/parole/bugs) page to view currently open bug reports and instructions on reporting new bugs or submitting bugfixes.

