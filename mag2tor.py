#!/usr/bin/env python
#
# Inspired by the Magnet2Torrent script from the
# https://github.com/danfolkes/Magnet2Torrent.git

import libtorrent
import time
import sys
import os.path

def mag2tor(mag_link):
    sess = libtorrent.session()
    prms = {
        'save_path':os.path.abspath(os.path.curdir),
        # 'storage_mode':libtorrent.storage_mode_t(2),
        'paused':False,
        'auto_managed':False,
        'upload_mode':True,
    }
    torr = libtorrent.add_magnet_uri(sess, mag_link, prms)
    dots = 0
    # sys.stdout.write('%s: ' % mag_link)
    while not torr.has_metadata():
        dots += 1
        sys.stdout.write('.')
        sys.stdout.flush()
        time.sleep(1)
    if (dots): sys.stdout.write('\n')
    sess.pause()
    tinf = torr.get_torrent_info()
    f = open(tinf.name() + '.torrent', 'wb')
    f.write(libtorrent.bencode(
        libtorrent.create_torrent(tinf).generate()))
    f.close()
    sess.remove_torrent(torr)

if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.stderr.write('USAGE: %s <magnet_link>\n' % sys.argv[0])
        sys.exit(1)
    for mag_link in sys.argv[1:]: mag2tor(mag_link)

# vi:set sw=4 et ts=8 tw=71:
