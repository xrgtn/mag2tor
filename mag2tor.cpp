#include <chrono>
#include <exception>
#include <stdexcept>	// __gnu_cxx::__verbose_terminate_handler
#include <fstream>
#include <iostream>
#include <thread>

#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/session_settings.hpp>
#include <libtorrent/storage_defs.hpp>	// lt::disabled_storage_constructor
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_info.hpp>

namespace lt = libtorrent;
int main(int argc, char *argv[]) {
	std::set_terminate(__gnu_cxx::__verbose_terminate_handler);

	lt::session_settings sset;
	sset.prefer_udp_trackers = false;

	if (argc != 2) {
		std::cerr << "usage: " << argv[0] << " <magnet-url>" <<
			std::endl;
		return 1;
	}

	lt::session sess;
	sess.set_settings(sset);
	lt::add_torrent_params atp;
	atp.url = argv[1];
	atp.upload_mode = true;
	atp.auto_managed = false;
	atp.paused = false;
	// Start with "storage == disabled" to avoid pre-allocating any files
	// mentioned in the torrent file on disk:
	atp.storage = lt::disabled_storage_constructor;
	atp.save_path = "."; // save in current dir
	/*
	// .flags duplicate .upload_mode/auto_managed/paused
	// functionality:
	atp.flags = lt::add_torrent_params::flag_update_subscribe
		| lt::add_torrent_params::flag_upload_mode
		| lt::add_torrent_params::flag_apply_ip_filter;
	*/
	lt::torrent_handle torh = sess.add_torrent(atp);
	std::cout << atp.url << ":";
	for (;;) {
		std::deque<lt::alert*> alerts;
		sess.pop_alerts(&alerts);
		for (lt::alert const* a : alerts) {
			std::cout << std::endl << a->message() << std::endl;
			// quit on error/finish:
			if (lt::alert_cast<lt::torrent_finished_alert>(a)
			|| lt::alert_cast<lt::torrent_error_alert>(a)) {
				goto done1;
			};
		}
		if (torh.status().has_metadata) {
			sess.pause();
			lt::torrent_info tinf =
				torh.get_torrent_info();
			std::cout << tinf.name() << std::endl;
			std::cout.flush();
			std::ofstream ofs;
			ofs.exceptions(std::ofstream::failbit
					| std::ofstream::badbit);
			ofs.open(tinf.name() + ".torrent",
					std::ofstream::binary);
			std::ostream_iterator<char> ofsi(ofs);
			lt::bencode(ofsi, lt::create_torrent(tinf)
					.generate());
			ofs.close();
			sess.remove_torrent(torh);
			goto done0;
		};
		std::cout << ".";
		std::cout.flush();
		std::this_thread::sleep_for(
			std::chrono::milliseconds(1000));
	}
done0:
	return 0;
done1:
	return 1;
}

// vi: set sw=8 ts=8 noet tw=77:
