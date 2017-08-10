#include <stddef.h>	// NULL
#include <unistd.h>	// optind
#include <getopt.h>	// getopt_long()
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

static int usage(const char *argv0) {
	std::cerr << "usage: [options] " << argv0 << " <magnet-url>" <<
		std::endl;
	std::cerr << "where options are:" << std::endl;
	std::cerr << "  -a, --anonymous  anonymous mode" << std::endl;
	std::cerr << "  -t, --tcp        TCP mode" << std::endl;
	std::cerr << "  -u, --udp        UDP mode (specifying both"
		<< " -t and -u enables both protocols" << std::endl;
	std::cerr << "                   with UDP trackers preferred)"
		<< std::endl;
	return 1;
}

namespace lt = libtorrent;
int main(int argc, char *argv[]) {
	std::set_terminate(__gnu_cxx::__verbose_terminate_handler);

	lt::session_settings sset;
	lt::add_torrent_params atp;
	int tcp_udp = 0;
	char *magnet_url = NULL;

	for (;;) {
		static struct option lopts[] = {
			{"anonymous", no_argument, NULL, 'a'},
			{"tcp",       no_argument, NULL, 't'},
			{"udp",       no_argument, NULL, 'u'},
		};
		int loptind = -1;
		int c = getopt_long(argc, argv, "atu", lopts, &loptind);
		if (c == -1) break;
		switch (c) {
			case 'a': sset.anonymous_mode = true; break;
			case 't': tcp_udp |= 1; break;
			case 'u': tcp_udp |= 2; break;
			default: return usage(argv[0]);;
		}
	}
	if (argc - optind != 1) return usage(argv[0]);
	atp.url = argv[optind];
	switch (tcp_udp) {
		case 0:
			sset.prefer_udp_trackers = false;
			sset.enable_outgoing_tcp = true;
			sset.enable_incoming_tcp = true;
			sset.enable_outgoing_utp = true;
			sset.enable_incoming_utp = true;
			break;
		case 1:
			sset.prefer_udp_trackers = false;
			sset.enable_outgoing_tcp = true;
			sset.enable_incoming_tcp = true;
			sset.enable_outgoing_utp = false;
			sset.enable_incoming_utp = false;
			break;
		case 2:
			sset.prefer_udp_trackers = true;
			sset.enable_outgoing_tcp = false;
			sset.enable_incoming_tcp = false;
			sset.enable_outgoing_utp = true;
			sset.enable_incoming_utp = true;
			break;
		case 3:
			sset.prefer_udp_trackers = true;
			sset.enable_outgoing_tcp = true;
			sset.enable_incoming_tcp = true;
			sset.enable_outgoing_utp = true;
			sset.enable_incoming_utp = true;
			break;
	}

	lt::session sess;
	sess.set_settings(sset);
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
