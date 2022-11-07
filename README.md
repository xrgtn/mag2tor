`ORG.NBEI:`
![GitHub release (latest by date)](https://img.shields.io/github/v/release/Network-BEncode-inside/mag2tor)
![GitHub Release Date](https://img.shields.io/github/release-date/Network-BEncode-inside/mag2tor)
![GitHub repo size](https://img.shields.io/github/repo-size/Network-BEncode-inside/mag2tor)
![GitHub all releases](https://img.shields.io/github/downloads/Network-BEncode-inside/mag2tor/total)
![GitHub](https://img.shields.io/github/license/Network-BEncode-inside/mag2tor)  

# mag2tor
Convert magnet link to torrent file.

## USAGE
```sh
mag2tor.py <magnet_link1> [<magnet_link2> ...]
```
or
```sh
mag2tor <magnet_link>
```

mag2tor tries to get .torrent file for each magnet link specified on
its commandline and write it into the current directory. For this it
needs working Internet connection and at least one active
seeder/leecher for the magnet/torrent. NOTE: waiting for magnet may
take days or even forever.

The mag2tor.py script has been inspired by the Magnet2Torrent from
https://github.com/danfolkes/Magnet2Torrent.git. It requires Python,
libtorrent-rasterbar, libboost and Python bindings for
libtorrent-rasterbar.

C++ version of mag2tor works almost the same way as mag2tor.py but
doesn't require Python and Python bindings for libtorrent-rasterbar.

* https://www.rasterbar.com/products/libtorrent/tutorial-ref.html
* https://www.rasterbar.com/products/libtorrent/manual-ref.html
* https://github.com/arvidn/libtorrent/releases/tag/libtorrent-1_2_5
* https://github.com/arvidn/libtorrent/releases/download/libtorrent-1_2_5/libtorrent-rasterbar-1.2.5.tar.gz
