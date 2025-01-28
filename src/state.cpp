#include "stdafx.h"

#include "state.h"
#include "config.h"
#include "browsefiles.h"
#include "httpserver_strings.h"
#define _CRT_RAND_S
#include <stdlib.h>

namespace httpc {
	void pb_art_container::set_no_info() {
		for (size_t i = 0; i < HTTPC_ART_COUNT; ++i)
		{
			this->instances[i].request_crc32 = tcfg.get().art.instances[i].url_art_not_available;
			this->instances[i].status = AS_NO_INFO;
		}
	};

	service_ptr_t<titleformat_object> script_playlist_row;
	pfc::array_t<service_ptr_t<titleformat_object>> script_helpers;

	pfc::readWriteLock cs;

	bool	enqueueing;	// if enqueueing is in progress;

	size_t		pb_state;				// current foobar state
	size_t		last_action;			// previously selected playlist action
	size_t		state_changed;			// previous state change
	float		volume;

	pfc::array_t<pfc::string8> pb_helpers;
	pfc::array_t<pfc::string8> pb_helpersx;
	pb_art_container pb_art;

	size_t		pb_item;				// playing item
	double		pb_length;				// playing item length
	double		pb_time;				// playing item position
	size_t		pb_playlist;			// playing playlist
	size_t		pb_playlist_item_count;	// playing playlist item count
	size_t		pb_item_prev;			// previously played item
	size_t		pb_playlist_prev;		// previously played playlist
	size_t		playlist_item_focused;	// focused item of current playlist
	size_t		playlist_page;			// page number of current playlist
	size_t		active_playlist;
	size_t		active_playlist_item_count;
	size_t		query_playlist;
	bool		playlist_page_switched;
	bool		should_focus_on_playing;
	bool		should_update_playlist;
	bool		should_update_playlist_total_time;
	bool		should_update_playlists_list;
	bool		should_update_queue;
	pfc::list_t<playlist_entry,pfc::alloc_fast_aggressive> playlist_view;
	pfc::string_simple	playlist_total_time;
	pfc::string_simple	queue_total_time;
	bool		sac;							// stop after current flag
	bool		reset_sac;						// reset stop after current after track stop flag;
	bool		active_playlist_is_undo_available;
	bool		active_playlist_is_redo_available;
	bool		active_playlist_is_locked;
	pfc::string_simple control_credentials_auth_hash;
	pfc::map_t<pfc::string, pfc::string, pfc::string::comparatorCaseInsensitive> registered_extensions;
	pfc::avltree_t<pfc::string, pfc::string::comparatorCaseInsensitive> allowed_protocols;
	pfc::string8	restrict_mask;					// restrict mask based on registered extensions
	pfc::list_t<playlist_info> playlist_list;		// list of playlists
	pfc::string8 fb2k_profile_path;
	pfc::string8 fb2k_path;
	pfc::string8 srv_home_dir;
	pfc::string_simple autoplaylist_request;
	pfc::hires_timer timer;

	class albumart_apev2_extractor : public album_art_extractor_impl_stdtags {
		albumart_apev2_extractor();
	};


	void update_previouslyplayed()
	{
		if (pb_item != pfc::infinite_size  && pb_playlist != pfc::infinite_size)
		{
			pb_item_prev = pb_item;
			pb_playlist_prev = pb_playlist;
		}
	}

	void enqueue(pfc::list_t<t_size> &list)
	{
		auto plm = playlist_manager::get();

		if (list.get_count())
			plm->activeplaylist_set_focus_item(list[list.get_count()-1]);

		for (t_size i = 0; i < list.get_count(); ++i)
			plm->queue_add_item_playlist(plm->get_active_playlist(), list.get_item(i));

		state_changed |= FSC_PLAYLIST;
	}

	void dequeue(pfc::list_t<t_size> &list)
	{
		auto plm = playlist_manager::get();

		if (list.get_count())
			plm->activeplaylist_set_focus_item(list[list.get_count()-1]);

		for (t_size j = 0; j < list.get_count(); ++j)
		{
			auto queue_item = pfc::infinite_size;

			pfc::list_t<t_playback_queue_item> queue;
			plm->queue_get_contents(queue);

			if (queue.get_count() && plm->activeplaylist_get_item_count() > list.get_item(j))
			for (t_size i = 0; i < queue.get_count() ; ++i)
			{
				auto &qi = queue[i];

				if (qi.m_item == list.get_item(j)
					&& qi.m_playlist == active_playlist
					&& plm->activeplaylist_get_item_handle(list.get_item(j)) == qi.m_handle)
				{
					queue_item = i;
					break;
				}
			}

			if (queue_item != pfc::infinite_size)
				plm->queue_remove_mask(bit_array_one(queue_item));
		}

		state_changed |= FSC_PLAYLIST;
	}

	void empty_info()
	{
		pb_state = httpc::FOO_STOPPED;
		pb_time = 0;
		pb_length = 0;

		pb_helpers.set_size(HTTPC_HELPER_COUNT);
		pb_helpersx.set_size(HTTPC_HELPER_COUNT);
		script_helpers.set_size(HTTPC_HELPER_COUNT);
		pb_art.empty();

		pb_helpers.fill("");
		pb_helpersx.fill("");

		pb_item = pfc::infinite_size;
		pb_playlist = pfc::infinite_size;
		pb_playlist_item_count = pfc::infinite_size;

		active_playlist_item_count = pfc::infinite_size;
		state_changed = FSC_NONE;
	}

	void empty_previouslyplayed()
	{
		pb_item_prev = pfc::infinite_size;
		pb_playlist_prev = pfc::infinite_size;
	}

	void refresh_volume(float p_volume)
	{
		if ((abs(p_volume) - 1.0) < 0.001)
			httpc::volume = playback_control::get()->get_volume();
		else
			httpc::volume = p_volume;
	}

	void set_volume(t_size percent)
	{
		auto pc = playback_control::get();

		if (percent > 100)
			percent = 100;

		float scale = (float)(percent / 100.0);
		float gain = (float)(20.0 * log10(scale));
		pc->set_volume(gain);
		httpc::volume = pc->get_volume();
	}

	byte get_volume()
	{
		return (byte)(pow(10.0,httpc::volume / 20.0) * 100.0);
	}

	void retrieve_albumart(metadb_handle_ptr pb_item_ptr)
	{
		for (t_size i = 0; i < HTTPC_ART_COUNT; ++i) {
			auto& pb_art = httpc::pb_art.instances[i];
			auto& art = tcfg.get().art.instances[i];

			pb_art.status = AS_NOT_FOUND;

			// looking for an external albumart by predefined path mask pb_albumart_lookup
			if (pb_art.lookup.get_length() > 0)
			{
				pfc::list_t<pfc::stringLite> lookups;

				get_list(pb_art.lookup, lookups, '|', false);

				HANDLE hFind;
				LPWIN32_FIND_DATAW findFileData;
				findFileData = new WIN32_FIND_DATAW;

				for (t_size i = 0; i < lookups.get_count(); ++i)
				{
					pfc::stringcvt::string_wide_from_utf8 mask(lookups[i]);

					hFind = FindFirstFileW(mask.get_ptr(), findFileData);

					if (hFind != INVALID_HANDLE_VALUE)
					{
						pfc::stringcvt::string_utf8_from_wide filename(findFileData->cFileName);
						auto directory = pfc::string_directory(lookups[i]);

						// todo: check for file size
						t_size filesize = findFileData->nFileSizeHigh * ((long)MAXDWORD + 1) + findFileData->nFileSizeLow;

						pb_art.source = directory;

						if (pb_art.source.get_length())
							pb_art.source.fix_dir_separator('\\');

						pb_art.source  << filename;

						pb_art.request_crc32 = pfc::string_formatter() << "/" << pfc::string(tcfg.get().root) << pfc::string_printf(httpc::server::strings::request::art, art.name.toString()) << calcCRC(pb_art.source, strlen(pb_art.source)) << "." << pfc::string_extension(filename);

						FindClose(hFind);

						if (tcfg.get().albumart_limit_size != 0 && filesize <= tcfg.get().albumart_limit_size || tcfg.get().albumart_limit_size == 0)
							pb_art.status = AS_FILE;
						else
							pb_art.status = AS_NOT_FOUND;

						break;
					}
				}
				delete findFileData;
			}
			if (pb_art.status == AS_NOT_FOUND || tcfg.get().albumart_prefer_embedded)
			{
				abort_callback_dummy p_abort;
				auto aami = album_art_manager_v2::get();
				auto extractor = aami->open(pfc::list_single(pb_item_ptr), pfc::list_single(tcfg.get().art.instances[i].embedded_art_type), p_abort);

				try {
					pb_art.embedded_ptr.release();
					pb_art.embedded_ptr = extractor->query(tcfg.get().art.instances[i].embedded_art_type, p_abort);
					pb_art.status = AS_MEMORY;
				}
				catch (exception_album_art_not_found)
				{
					if (pb_art.status != AS_FILE)
						pb_art.status = AS_NOT_FOUND;
				}

				if (pb_art.status == AS_MEMORY
					&& pb_art.embedded_ptr.is_valid()
					&& pb_art.embedded_ptr->get_size()
					&& ((tcfg.get().albumart_limit_size != 0) && (pb_art.embedded_ptr->get_size() <= tcfg.get().albumart_limit_size) || tcfg.get().albumart_limit_size == 0))
				{
					pb_art.embedded_file = file_path_display(pb_item_ptr->get_path());
					pb_art.request_crc32 = pfc::string_formatter() << "/" << pfc::stringLite(tcfg.get().root) << pfc::string_printf(httpc::server::strings::request::art, art.name.toString()) << calcCRC(pb_art.embedded_file, pb_art.embedded_file.get_length());
				}
				else
				{
					if (pb_art.status != AS_FILE)
						pb_art.status = AS_NOT_FOUND;
				}
			}

			if (pb_art.status == AS_NOT_FOUND)
				pb_art.request_crc32 = art.url_art_not_found;
		}
	}

	void refresh_playing_info()
	{
		if (!httpc::control::is_active() || !tcfg.get_count())
			return;

		auto plm = playlist_manager::get();
		auto plc = playback_control::get();

		metadb_handle_ptr pb_item_ptr;
		t_size index;

		pb_item = pfc::infinite_size;
		pb_playlist = pfc::infinite_size;

		if(plc->get_now_playing(pb_item_ptr))
			if(plm->get_playing_item_location(&pb_playlist, &index))
				pb_item = index;

		if (pb_playlist != pfc::infinite_size)
			pb_playlist_item_count = plm->playlist_get_item_count(pb_playlist);
		else
			pb_playlist_item_count = pfc::infinite_size;

		if (plm->get_playlist_count() == 1)
			plm->set_active_playlist(0);

		active_playlist = plm->get_active_playlist();

		playlist_item_focused = plm->playlist_get_focus_item(plm->get_active_playlist());

		active_playlist_item_count = plm->playlist_get_item_count(plm->get_active_playlist());

		auto page_prev = httpc::playlist_page;

		if (tcfg.get().playlist_items_per_page != 0)
		{
			if (should_focus_on_playing)
			{
				if (active_playlist == pb_playlist && pb_item != pfc::infinite_size)
					playlist_page = (t_size)ceil((((double)pb_item+1)*1.0 / tcfg.get().playlist_items_per_page*1.0));
				else
				if (playlist_item_focused != pfc::infinite_size)
					playlist_page = (t_size)ceil(((double)playlist_item_focused+1)*1.0 / tcfg.get().playlist_items_per_page*1.0);

				should_focus_on_playing = false;
			}

			if (playlist_page == pfc::infinite_size || playlist_page == 0)
				playlist_page = 1;

			if (playlist_page > (t_size)ceil(active_playlist_item_count*1.0 / tcfg.get().playlist_items_per_page*1.0))
				playlist_page = (t_size)ceil(active_playlist_item_count*1.0 / tcfg.get().playlist_items_per_page*1.0);

			if (page_prev != playlist_page)
				httpc::playlist_page_switched = true;
			else
				playlist_page_switched = false;
		}

		if (plc->get_now_playing(pb_item_ptr))
		{
			pb_length = pb_item_ptr->get_length();

			if (pb_playlist == pfc::infinite_size  || pb_item == pfc::infinite_size)
			{
				for (t_size i = 0; i < HTTPC_HELPER_COUNT; ++i)
					pb_item_ptr->format_title(NULL, pb_helpers[i], script_helpers[i], NULL);

				for (auto& artwork : pb_art.instances)
					pb_item_ptr->format_title(NULL, artwork.lookup, artwork.script_lookup, NULL);
			}
			else
			{
				for (t_size i = 0; i < HTTPC_HELPER_COUNT; ++i)
					plm->playlist_item_format_title(pb_playlist, pb_item, NULL, pb_helpers[i], script_helpers[i], NULL, playback_control::display_level_all);

				for (auto &artwork : pb_art.instances)
					plm->playlist_item_format_title(pb_playlist, pb_item, NULL, artwork.lookup, artwork.script_lookup, NULL, playback_control::display_level_all);
			}

			for (t_size i = 0; i < HTTPC_HELPER_COUNT; ++i)
				pb_helpersx[i] = xml_friendly_string(pb_helpers[i]);

			retrieve_albumart(pb_item_ptr);
		}
		else
			pb_art.set_no_info();

		if (plc->is_paused())
			httpc::pb_state = FOO_PAUSED;
		else if (plc->is_playing())
			httpc::pb_state = FOO_PLAYING;
		else
			httpc::pb_state = FOO_STOPPED;
	}

	void refresh_playlist_view()
	{
		if ( tcfg.get().playlist_items_per_page == 0 || !httpc::control::is_active() || tcfg.get_count() <= 1)
			return;

		auto plm = playlist_manager::get();

		active_playlist_is_undo_available = plm->activeplaylist_is_undo_available();
		active_playlist_is_redo_available = plm->activeplaylist_is_redo_available();

		auto apl_count = plm->activeplaylist_get_item_count();

		if (apl_count)
		{
			t_size item_start = (httpc::playlist_page-1) * tcfg.get().playlist_items_per_page;
			t_size item_end = (httpc::playlist_page-1) * tcfg.get().playlist_items_per_page + tcfg.get().playlist_items_per_page - 1;

			if (item_end > apl_count - 1)
				item_end = apl_count - 1;

			metadb_handle_list pl;
			bit_array_range pl_mask(item_start, item_end-item_start+1);
			plm->activeplaylist_get_items(pl, pl_mask);

			auto pl_count = pl.get_count();

			playlist_view.set_size(pl_count);

			for (t_size i = 0; i < pl_count; ++i)
				pl[i]->format_title(NULL, playlist_view[i].title, script_playlist_row, NULL);
		}
		else
			playlist_view.set_size(0);

		playlist_item_focused = plm->activeplaylist_get_focus_item();

		should_update_playlist = false;
	}

	void refresh_playlist_total_time()
	{
		metadb_handle_list pl;

		playlist_manager::get()->activeplaylist_get_all_items(pl);
		playlist_total_time = pfc::format_time_ex(metadb_handle_list_helper::calc_total_duration(pl), 0);

		should_update_playlist_total_time = false;
	}

	void refresh_playback_queue()
	{
		if (tcfg.get().playlist_items_per_page == 0)
			return;

		auto plm = playlist_manager::get();

		auto pl_count = plm->activeplaylist_get_item_count();
		auto pl_count2 = playlist_view.get_count();

		queue_total_time = "";

		pfc::list_t<t_playback_queue_item> queue;
		plm->queue_get_contents(queue);

		if (pl_count)
		{
			t_size item_start = (httpc::playlist_page-1) * tcfg.get().playlist_items_per_page;
			t_size item_end = (httpc::playlist_page-1) * tcfg.get().playlist_items_per_page + tcfg.get().playlist_items_per_page;

			for (t_size i = item_start, j = 0; i < pl_count && i < item_end && j < pl_count2; ++i, ++j)
			{
				playlist_entry &ple = playlist_view[j];
				ple.numinqueue.reset();
				ple.title_queue.reset();
				ple.inqueue = false;

				t_size l = queue.get_count();
				for (t_size y = 0; y < l; ++y)
				{
					t_playback_queue_item &qi = queue[y];

					if (qi.m_playlist == active_playlist
					&& qi.m_item == i
					&& qi.m_handle == plm->activeplaylist_get_item_handle(i))
					{
						plm->playlist_item_format_title(active_playlist,qi.m_item,NULL,ple.title_queue,script_playlist_row,NULL,playback_control::display_level_all);

						ple.inqueue = true;

						if (ple.numinqueue.get_length())
							ple.numinqueue << " " << y+1;
						else
							ple.numinqueue << y+1;
					}
				}
			}

			double queue_pb_time = 0;

			t_size l = queue.get_count();
			for (t_size y = 0; y < l; ++y)
				queue_pb_time+=queue[y].m_handle->get_length();

			queue_total_time = pfc::format_time_ex(queue_pb_time, 0);
		}

		should_update_queue = false;
	}

	void refresh_playlist_list()
	{
		auto plm = playlist_manager::get();

		pfc::stringLite pl_str;
		playlist_info pl_i;

		playlist_list.remove_all();

		auto l = plm->get_playlist_count();

		for (t_size i = 0; i < l; ++i)
			if (plm->playlist_get_name(i, pl_str))
			{
				pl_i.name = xml_friendly_string(pl_str);
				pl_i.items = plm->playlist_get_item_count(i);
				pl_i.locked = plm->playlist_lock_is_present(i);

				playlist_list.add_item(pl_i);
			}

		active_playlist = plm->get_active_playlist();

		if (active_playlist != pfc::infinite_size)
			active_playlist_is_locked = plm->playlist_lock_is_present(active_playlist);
		else
			active_playlist_is_locked = 0;

		should_update_playlists_list = false;
	}

	bool get_registered_extension_name(const char *path, pfc::string_base &name)
	{
		auto path_len = strlen(path);

		if (path_len <= 4 || path[path_len-1] == '\\')
			return false;

		if (strstr(path, "cdda:/") == path)
			return httpc::registered_extensions.query("cda", name);

		pfc::string extension = get_extension(path);
		return httpc::registered_extensions.query(extension, name);
	}

	bool is_extension_registered(const char* path)
	{
		pfc::string name;

		return get_registered_extension_name(path, name);
	}

	bool is_protocol_allowed(const char *path)
	{
		for(auto& allowed_protocol : httpc::allowed_protocols)
			if (matchProtocol(path, allowed_protocol))
				return true;

		return false;
	}

	bool is_path_allowed(const char *path)
	{
		if (cfg.restrict_to_path_list.get_count()) // allowing to browse only specified dirs (if any)
		{
			if (strlen(path) > 0)
			{
				// Allow root browser
				if (strcmp(path, " ") == 0) return true;

				auto l = cfg.restrict_to_path_list.get_count();

				if (strstr(path, "..\\") == NULL && strstr(path, "../") == NULL) // check for stuff like d:\music\..\..\..\temp
					for (size_t i = 0; i < l; ++i)
					{
						pfc::stringLite tmp(path);
						tmp.truncate(cfg.restrict_to_path_list[i].get_length());

						if (pfc::stringCompareCaseInsensitive(tmp, cfg.restrict_to_path_list[i]) == 0)
							return true;
					}

				return false;
			}
		}
		return true;
	}

	void get_registered_extensions()
	{
		pfc::list_t<pfc::string> ignored_extensions;

		pfc::splitStringSimple_toList(ignored_extensions, '|', cfg.main.ignored_formats);
		registered_extensions.remove_all();

		for (auto& ext : ignored_extensions)
			ext = trim(ext);

		for (auto file_type : input_file_type::enumerate())
		{
			pfc::string mask, name;
			unsigned n, m = file_type->get_count();

			for (n = 0; n < m; n++)
			{
				if (file_type->get_mask(n, mask) && file_type->get_name(n, name))
				{
					if (!mask.contains('|'))
					{
						pfc::string extension;
						size_t i = 0;

						while (i < mask.get_length())
						{
							if (mask[i] != '*' && mask[i] != '.' && mask[i] != ';')
								extension.add_char(mask[i]);

							if (mask[i] == ';' || i == mask.get_length() - 1)
							{
								bool found = false;
								for (auto ignored_extension : ignored_extensions)
									if (pfc::stringCompareCaseInsensitive(extension, ignored_extension) == 0)
									{
										found = true;
										break;
									}
								if (!found)
									registered_extensions[extension] = name;
								extension.reset();
							}

							++i;
						}
					}
				}
			}
		}

		pfc::list_t<pfc::string> extra_extensions;
		pfc::splitStringSimple_toList(extra_extensions, '|', cfg.main.extra_formats);
		pfc::string ext;
		for (auto& extension : extra_extensions)
		{
			ext = trim(extension);

			if (!ext.is_empty())
				registered_extensions[ext] = "Extra format";
		}

		for (auto playlist : playlist_loader::enumerate())
		{
			bool found = false;
			for (auto ignored_extension : ignored_extensions)
				if (pfc::stringCompareCaseInsensitive(ignored_extension, playlist->get_extension()) == 0)
				{
					found = true;
					break;
				}
			if (!found)
				registered_extensions[playlist->get_extension()] = "Playlist";
		}

		// build restrict mask
		for (auto& extension : registered_extensions)
		{
			if (!restrict_mask.is_empty())
				restrict_mask << ";";
			restrict_mask << "*." << extension.m_key;
		}
	}

	void set_allowed_protocols()
	{
		pfc::list_t<pfc::stringLite> protocols;
		pfc::splitStringSimple_toList(protocols, '|', cfg.main.allowed_protocols);
		pfc::stringLite protocol;

		httpc::allowed_protocols.remove_all();
		for (t_size i = 0; i < protocols.get_count(); ++i)
		{
			protocol = trim(protocols[i]);

			if (protocol.get_length())
				httpc::allowed_protocols.add_item(protocol);
		}
	}

	void choose_srv_home_dir()
	{
		pfc::stringLite server_root_tmp = cfg.main.server_root;
		remove_trailing_path_separator(server_root_tmp);
		httpc::srv_home_dir = (server_root_tmp.get_length() > 0 ? server_root_tmp : httpc::fb2k_profile_path);

		for (t_size i = 0; i < tcfg.get_count(); ++i)
			if (tcfg.get(i).root.get_length())
					tcfg.get(i).local_path = pfc::string_formatter() << httpc::srv_home_dir << "\\" << tcfg.get(i).root << "\\";
	}

	extern void build_restrict_to_path_list()
	{
		cfg.restrict_to_path_list.remove_all();
		get_list(cfg.main.restrict_to_path, cfg.restrict_to_path_list, '|', true);
	}

	void control_credentials_auth_hash_update()
	{
		pfc::stringLite buf;

		pfc::base64_encode_from_string(
			buf,
			pfc::string_formatter() << cfg.main.control_credentials_username << ":" << cfg.main.control_credentials_password
		);

		control_credentials_auth_hash = pfc::string_formatter() << "Basic " << buf;
	}

	void titleformat_compile()
	{
		script_playlist_row.release();
		titleformat_compiler::get()->compile_safe(script_playlist_row,tcfg.get().playlist_row);

		for (size_t i = 0; i < HTTPC_HELPER_COUNT; ++i) {
			script_helpers[i].release();
			titleformat_compiler::get()->compile_safe(script_helpers[i], tcfg.get().helpers[i]);
		}

		for (size_t i = 0; i < HTTPC_ART_COUNT; ++i) {
			pb_art.instances[i].script_lookup.release();
			titleformat_compiler::get()->compile_safe(pb_art.instances[i].script_lookup, tcfg.get().art.instances[i].source);
		}
	}
}