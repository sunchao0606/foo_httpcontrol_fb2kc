#ifndef __httpcontrol_state_H__
#define __httpcontrol_state_H__

#define _CRT_SECURE_NO_DEPRECATE 1

// Number of next/previous pages to create in playlist page switcher
#define HTTPC_PLAYLIST_PAGES_LEFTRIGHT 4

// Number of HELPERn/HELPERnX template macros to support
#define HTTPC_HELPER_COUNT 9

// Artwork names
#define HTTPC_ART_ALBUMART "albumart"
#define HTTPC_ART_DISCART "discart"

// Number of albumart instances to support
#define HTTPC_ART_COUNT 2


namespace httpc {
	extern pfc::readWriteLock cs;

	extern service_ptr_t<titleformat_object> script_playlist_row;
	extern pfc::array_t<service_ptr_t<titleformat_object>> script_helpers;

	enum foo_state_enum { FOO_STOPPED = 0, FOO_PLAYING = 1, FOO_PAUSED = 2};
	enum foo_last_action { FLC_START = 0, FLC_ENQUEUE = 1, FLC_DEQUEUE = 2, FLC_FOCUS = 3, FLC_REMOVE = 4, FLC_SELECT = 5, FLC_SHIFT = 6};

	struct playlist_entry
	{
		bool	inqueue = false;
		pfc::string8 numinqueue;
		pfc::string8 title;
		pfc::string8 title_queue;
	};

	struct playlist_info
	{
		pfc::string8 name;		// playlist name
		size_t items = 0;		// playlist item count
		bool locked = false;	// is playlist locked
	};

	enum foo_state_changed {
		FSC_NONE		= 0,	// nothing changed
		FSC_PLAYBACK	= 1,	// playback state (start, stop, seek, next track etc)
		FSC_PLAYLISTS	= 2,	// playlists state (names, order, removal, addition etc)
		FSC_PLAYLIST	= 4,	// playlist content (items addition, removal, modify, reorder etc)
		FSC_VOLUME		= 8,	// volume
		FSC_PLAYBACK_ORDER = 16	// playback order
	};

	enum albumart_status {
		AS_NO_INFO		= 0,	// no playing item so no albumart info
		AS_FILE			= 1,	// albumart in file
		AS_MEMORY		= 2,	// albumart in memory
		AS_NOT_FOUND	= 4,	// albumart not found anywhere
	};

	extern bool	enqueueing;

	extern size_t		pb_state;
	extern size_t		last_action;
	extern size_t		state_changed;
	extern float		volume;
	extern pfc::array_t<pfc::string8> pb_helpers;
	extern pfc::array_t<pfc::string8> pb_helpersx;

	class pb_art_instance {
	public:
		pfc::string source;			// art source expression
		pfc::string request_crc32;	// art request url including crc32
		pfc::string lookup;			// titleformatted art source expression
		pfc::string embedded_file;	// path to file with embedded artwork
		albumart_status status;		// art status
		album_art_data_ptr embedded_ptr;
		service_ptr_t<titleformat_object> script_lookup;	// titleformatting script of path

		void empty() {
			source.reset();
			request_crc32.reset();
			lookup.reset();
			embedded_file.reset();
			embedded_ptr.release();
			status = httpc::AS_NO_INFO;
		};

		pb_art_instance() : status(httpc::AS_NO_INFO) {};
	};

	class pb_art_container {
	public:
		pfc::array_t<pb_art_instance> instances;

		pb_art_container() {
			this->instances.set_count(HTTPC_ART_COUNT);
		};

		void empty() {
			for (auto &pb_art : this->instances)
				pb_art.empty();

			this->set_no_info();
		}

		void set_no_info();
	};

	extern pb_art_container pb_art;

	extern size_t		pb_item;
	extern double		pb_length;
	extern double		pb_time;
	extern size_t		pb_playlist;
	extern size_t		pb_playlist_item_count;
	extern size_t		pb_item_prev;
	extern size_t		pb_playlist_prev;
	extern size_t		playlist_item_focused;
	extern size_t		playlist_page;
	extern size_t		active_playlist;
	extern size_t		active_playlist_item_count;
	extern size_t		query_playlist;
	extern bool			playlist_page_switched;
	extern bool			should_focus_on_playing;
	extern bool			should_update_playlist;
	extern bool			should_update_playlist_total_time;
	extern bool			should_update_playlists_list;
	extern bool			should_update_queue;
	extern pfc::list_t<playlist_entry,pfc::alloc_fast_aggressive> playlist_view;
	extern pfc::string_simple	playlist_total_time;
	extern pfc::string_simple	queue_total_time;
	extern bool			sac;
	extern bool			reset_sac;
	extern bool			active_playlist_is_undo_available;
	extern bool			active_playlist_is_redo_available;
	extern bool			active_playlist_is_locked;
	extern pfc::string_simple control_credentials_auth_hash;
	extern pfc::map_t<pfc::string, pfc::string, pfc::string::comparatorCaseInsensitive> registered_extensions;
	extern pfc::avltree_t<pfc::string, pfc::string::comparatorCaseInsensitive> allowed_protocols;
	extern pfc::string8	restrict_mask;
	extern pfc::list_t<playlist_info> playlist_list;
	extern pfc::string8 fb2k_profile_path;
	extern pfc::string8 fb2k_path;
	extern pfc::string8 srv_home_dir;
	extern pfc::string_simple autoplaylist_request;

	extern pfc::hires_timer timer;

	extern void enqueue(pfc::list_t<t_size> &list);
	extern void dequeue(pfc::list_t<t_size> &list);
	extern void empty_info();

	extern void refresh_volume(float p_volume = 0);
	extern void set_volume(t_size pecent);
	extern byte get_volume();
	extern void retrieve_albumart(metadb_handle_ptr pb_item_ptr);
	extern void refresh_playing_info();
	extern void refresh_playlist_view();
	extern void refresh_playlist_total_time();
	extern void refresh_playback_queue();
	extern void refresh_playlist_list();

	extern void update_previouslyplayed();
	extern void empty_previouslyplayed();

	extern void get_registered_extensions();
	extern void set_allowed_protocols();
	extern bool get_registered_extension_name(const char* path, pfc::string_base& name);
	extern bool is_extension_registered(const char* path);
	extern bool is_protocol_allowed(const char *path);
	extern bool is_path_allowed(const char *path);

	extern void choose_srv_home_dir();
	extern void build_restrict_to_path_list();

	extern void control_credentials_auth_hash_update();

	extern void titleformat_compile();
};

#endif /*__httpcontrol_state_H__*/