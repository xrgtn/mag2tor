#!/usr/bin/python
#
# Inspired by the Magnet2Torrent script from the
# https://github.com/danfolkes/Magnet2Torrent.git

import libtorrent as lt
import time
import sys
import os.path

def mag2tor_v1(mag_link):
    sess = lt.session()
    prms = {
        'save_path':os.path.abspath(os.path.curdir),
        # 'storage_mode':lt.storage_mode_t(2),
        'paused':False,
        'auto_managed':False,
        'upload_mode':True,
    }
    torr = lt.add_magnet_uri(sess, mag_link, prms)
    dots = 0
    sys.stdout.write('%s: ' % mag_link)
    while not torr.has_metadata():
        dots += 1
        sys.stdout.write('.')
        sys.stdout.flush()
        time.sleep(1)
    if (dots): sys.stdout.write(' ')
    sess.pause()
    tinf = torr.get_torrent_info()
    sys.stdout.write('%s.torrent\n' % tinf.name())
    f = open(tinf.name() + '.torrent', 'wb')
    f.write(lt.bencode(lt.create_torrent(tinf).generate()))
    f.close()
    sess.remove_torrent(torr)

def mag2tor_v2(mag_link):
    sess = lt.session()
    atp = lt.parse_magnet_uri(mag_link)
    atp.save_path = "."
    atp.flags = lt.torrent_flags.upload_mode
    torr = sess.add_torrent(atp)
    dots = 0
    sys.stdout.write('%s: ' % mag_link)
    while not torr.status().has_metadata:
        dots += 1
        sys.stdout.write('.')
        sys.stdout.flush()
        time.sleep(1)
    if (dots): sys.stdout.write(' ')
    sess.pause()
    tinf = torr.torrent_file()
    # Workaround for empty torrent_info.trackers() in
    # libtorrent-rasterbar-2.0.7:
    trn = 0
    for t in tinf.trackers(): trn += 1
    if trn == 0:
        for t in atp.trackers:
            tinf.add_tracker(t)
    sys.stdout.write('%s.torrent\n' % tinf.name())
    f = open(tinf.name() + '.torrent', 'wb')
    f.write(lt.bencode(lt.create_torrent(tinf).generate()))
    f.close()
    sess.remove_torrent(torr)

if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.stderr.write('USAGE: %s <magnet_link>\n' % sys.argv[0])
        sys.exit(1)
    for mag_link in sys.argv[1:]:
        if not hasattr(lt, 'version_major') or lt.version_major < 2:
            mag2tor_v1(mag_link)
        else:
            mag2tor_v2(mag_link)

# vi:set sw=4 et ts=8 tw=71:
