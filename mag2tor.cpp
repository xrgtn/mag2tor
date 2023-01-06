#include <stddef.h>	// NULL
#include <unistd.h>	// optind
#include <getopt.h>	// getopt_long()
#include <chrono>
#include <exception>
#include <stdexcept>	// __gnu_cxx::__verbose_terminate_handler
#include <fstream>
#include <iostream>
#include <thread>

#include <libtorrent/version.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/fingerprint.hpp>
#include <libtorrent/session.hpp>
#if LIBTORRENT_VERSION_NUM < 10200
#include <libtorrent/session_settings.hpp>
#else
#include <libtorrent/settings_pack.hpp>
#include <libtorrent/torrent_flags.hpp>	// lt::torrent_flags::paused
#include <libtorrent/magnet_uri.hpp>	// lt::parse_magnet_uri()
// struct lt::aux::session_impl::session_plugin_wrapper:
#include <libtorrent/aux_/session_impl.hpp>
// lt::create_ut_metadata_plugin():
#include <libtorrent/extensions/ut_metadata.hpp>
// lt::create_ut_pex_plugin():
#include <libtorrent/extensions/ut_pex.hpp>
// lt::create_smart_ban_plugin():
#include <libtorrent/extensions/smart_ban.hpp>
#endif
#include <libtorrent/storage_defs.hpp>	// lt::disabled_storage_constructor
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_info.hpp>

#if LIBTORRENT_VERSION_NUM >= 20000
// lt::disabled_disk_io_constructor:
#include <libtorrent/disabled_disk_io.hpp>
// lt::download_priority_t, lt::dont_download:
#include <libtorrent/download_priority.hpp>
#include <libtorrent/units.hpp>		// lt::file_index_t
#include <libtorrent/aux_/vector.hpp>	// lt::aux::vector<>
#endif

static int usage(const char *argv0) {
	std::cerr << "usage: [options] " << argv0 << " <magnet-url>" <<
		std::endl;
	std::cerr << "where options are:" << std::endl;
	std::cerr << "  -a, --anonymous  anonymous mode"
		<< " via socks5://localhost:9050" << std::endl;
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

#if LIBTORRENT_VERSION_NUM < 10200
	lt::session_settings sset;
#else
	lt::settings_pack spack;
#endif
	int sess_tcp_udp = 0;
	int sess_anon    = 0;
	char *magnet_url = NULL;

	// Parse cmdline options and args:
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
			case 'a': sess_anon = 1;	break;
			case 't': sess_tcp_udp |= 1;	break;
			case 'u': sess_tcp_udp |= 2;	break;
			default: return usage(argv[0]);
		}
	}
	if (argc - optind != 1) return usage(argv[0]);
	magnet_url = argv[optind];

	// Init session settings:
#if LIBTORRENT_VERSION_NUM < 10200
	sset.prefer_udp_trackers = sess_tcp_udp & 2  ?  true : false;
	sset.enable_outgoing_tcp = sess_tcp_udp == 2 ? false :  true;
	sset.enable_incoming_tcp = sess_tcp_udp == 2 ? false :  true;
	sset.enable_outgoing_utp = sess_tcp_udp == 1 ? false :  true
	sset.enable_incoming_utp = sess_tcp_udp == 1 ? false :  true;
	sset.anonymous_mode      = sess_anon         ?  true : false;
#else
	spack.set_bool(lt::settings_pack::prefer_udp_trackers,
		sess_tcp_udp & 2  ?  true : false);
	spack.set_bool(lt::settings_pack::enable_outgoing_tcp,
		sess_tcp_udp == 2 ? false :  true);
	spack.set_bool(lt::settings_pack::enable_incoming_tcp,
		sess_tcp_udp == 2 ? false :  true);
	spack.set_bool(lt::settings_pack::enable_outgoing_utp,
		sess_tcp_udp == 1 ? false :  true);
	spack.set_bool(lt::settings_pack::enable_incoming_utp,
		sess_tcp_udp == 1 ? false :  true);
	spack.set_str(lt::settings_pack::peer_fingerprint,
		lt::generate_fingerprint("LT", 0, 0, 0, 0));
	spack.set_str(lt::settings_pack::user_agent,
		"libtorrent/" LIBTORRENT_VERSION);
	if (sess_anon == 1) {
		spack.set_str(lt::settings_pack::proxy_hostname,
			"127.0.0.1");
		spack.set_int(lt::settings_pack::proxy_port, 9050);
		spack.set_int(lt::settings_pack::proxy_type,
			lt::settings_pack::proxy_type_t::socks5);
		// External listen interfaces are not needed when going via
		// 127.0.0.1:9050 (in fact, internal aren't too, but there's
		// no way to stop libtorrent from trying to bind to
		// something):
		spack.set_str(lt::settings_pack::listen_interfaces,
			"127.0.0.1:6881");
		// Connect only via proxy:
		spack.set_bool(lt::settings_pack::anonymous_mode, true);
		spack.set_bool(lt::settings_pack::proxy_peer_connections,
			true);
		spack.set_bool(lt::settings_pack::proxy_tracker_connections,
			true);
		// Resolve DNS via proxy:
		spack.set_bool(lt::settings_pack::proxy_hostnames, true);
	};
	// lt::session::start_default_features flag causes these to be set
	// to true, but neither is necessary for converting magnet: link
	// into .torrent file:
	spack.set_bool(lt::settings_pack::enable_upnp,   false);
	spack.set_bool(lt::settings_pack::enable_natpmp, false);
	spack.set_bool(lt::settings_pack::enable_lsd,    false);
	spack.set_bool(lt::settings_pack::enable_dht,    false);
#endif

#if LIBTORRENT_VERSION_NUM < 10200
	lt::session sess(lt::fingerprint("LT", 0, 0, 0, 0),
			lt::session::add_default_plugins);
	sess.set_settings(sset);
	lt::add_torrent_params atp;
	atp.upload_mode = true;
	atp.auto_managed = false;
	atp.paused = false;
	/*
	// .flags duplicate .upload_mode/auto_managed/paused
	// functionality:
	atp.flags = lt::add_torrent_params::flag_update_subscribe
		| lt::add_torrent_params::flag_upload_mode
		| lt::add_torrent_params::flag_apply_ip_filter;
	*/
#else
	// Create session_params and session. The ut_metadata extension
	// (plugin) is required to get .torrent metadata via 'magnet:' link,
	// but the other two default plugins (ut_pex and smart_ban) aren't.
	using plgnwrp = lt::aux::session_impl::session_plugin_wrapper;
	std::vector<std::shared_ptr<lt::plugin>> exts;
	exts.push_back(
		std::make_shared<plgnwrp>(&lt::create_ut_metadata_plugin));
	lt::session_params sparams(spack, exts);
#if LIBTORRENT_VERSION_NUM >= 20000
	sparams.disk_io_constructor = &lt::disabled_disk_io_constructor;
#endif
	lt::session sess(sparams);
	// sess.add_extension(&lt::create_ut_pex_plugin);
	// sess.add_extension(&lt::create_smart_ban_plugin);
	lt::add_torrent_params atp = lt::parse_magnet_uri(magnet_url);
	// Citing add_torrent_params.hpp:
	//     The add_torrent_params class has a flags field. It can be used
	//     to control what state the new torrent will be added in. Common
	//     flags to want to control are torrent_flags::paused and
	//     torrent_flags::auto_managed. In order to add a magnet link
	//     that will just download the metadata, but no payload, set the
	//     torrent_flags::upload_mode flag.
	// Hopefully 'upload_mode' works as advertized above, but in case it
	// doesn't, use 'disabled_storage_constructor' or long vector with
	// 'dont_download' anyway...
	atp.flags = lt::torrent_flags::upload_mode
		| lt::torrent_flags::auto_managed
		| lt::torrent_flags::paused;
#endif

#if LIBTORRENT_VERSION_NUM < 20000
	// Start loading torrent metadata with "storage == disabled" to avoid
	// pre-allocating any files mentioned in the torrent file on disk:
	atp.storage = lt::disabled_storage_constructor;
#else
	// XXX:kludge:
	// Disable downloading all files in the torrent. But because we don't
	// know beforehand how many files will be there, just disable
	// downloading first 65535 files in it:
	lt::aux::vector<lt::download_priority_t, lt::file_index_t>
		dlprio(65535);
	for (int i = 0; i < 65535; i++)
		dlprio[i] = lt::dont_download;
	atp.file_priorities = dlprio;
#endif
	atp.save_path = "."; // save in current dir

	lt::torrent_handle torh = sess.add_torrent(atp);
	int sol = 1;	// start of line (on std::cout)

	for (;;) {
#if LIBTORRENT_VERSION_NUM < 10200
		std::deque<lt::alert*> alerts;
#else
		std::vector<lt::alert*> alerts;
#endif
		sess.pop_alerts(&alerts);
		for (lt::alert const* a : alerts) {
			if (!sol++) std::cout << std::endl;
			std::cout << a->what() << ": " << a->message()
				<< std::endl;
			// quit on error/finish:
			if (lt::alert_cast<lt::torrent_finished_alert>(a)
			|| lt::alert_cast<lt::torrent_error_alert>(a)) {
				goto done1;
			};
		}
		if (torh.status().has_metadata) {
			sess.pause();
			std::shared_ptr<lt::torrent_info> pti;
#if LIBTORRENT_VERSION_NUM < 10200
			lt::torrent_info ti = torh.get_torrent_info();
			pti = &tinf;
#else
			pti = torh.torrent_file_with_hashes();
#endif
			if (!sol++) std::cout << std::endl;
			std::cout << "retrieved info: " << pti->name()
				<< ".torrent" << std::endl;
			if (!pti->comment().empty())
				std::cout << "       comment: "
					<< pti->comment() << std::endl;
			if (!pti->creator().empty())
				std::cout << "       creator: "
					<< pti->creator() << std::endl;
			/*
			 * XXX: starting with libtorrent-rasterbar-2.0.7,
			 * torrent_info (and subsequently a .torrent file) is
			 * generated with empty trackers() list. As a
			 * workaround, add trackers from atp/magnet to pti:
			 */
			if (pti->trackers().empty()) {
				for (auto &u : atp.trackers) {
					pti->add_tracker(u, 0,
						lt::announce_entry
						::source_magnet_link);
				};
			};
			if (!pti->trackers().empty()) {
				std::cout << "      trackers: ";
				int n = 0;
				for (lt::announce_entry const &ann
				: pti->trackers()) {
					if (n++) std::cout << ", ";
					std::cout << ann.url;
				};
				std::cout << std::endl;
			};
			std::cout.flush();
			std::ofstream ofs;
			ofs.exceptions(std::ofstream::failbit
					| std::ofstream::badbit);
			ofs.open(pti->name() + ".torrent",
					std::ofstream::binary);
			std::ostream_iterator<char> ofsi(ofs);
			lt::bencode(ofsi, lt::create_torrent(*pti)
					.generate());
			ofs.close();
			sess.remove_torrent(torh);
			goto done0;
		};
		std::cout << ".";
		std::cout.flush();
		sol = 0;
		std::this_thread::sleep_for(
			std::chrono::milliseconds(1000));
	}
done0:
	return 0;
done1:
	return 1;
}

// vi: set sw=8 ts=8 noet tw=77:
