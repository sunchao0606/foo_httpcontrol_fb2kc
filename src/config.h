#ifndef __FOO_CONFIG_H__
#define __FOO_CONFIG_H__

#include "state.h"

#define HTTPC_PLAYLIST_ITEMS_PER_PAGE_DEFAULT 30
#define HTTPC_PLAYLIST_ITEMS_PER_PAGE_MAX_DEFAULT 16384

#define HTTPC_QUERY_PLAYLIST_NAME "Query (http)"
#define HTTPC_QUERY_AUTOPLAYLIST_CLIENT_NAME "http Query"

//externs
namespace httpc{

	namespace control {
		extern volatile bool listener_started;
		extern volatile bool listener_error;
		extern volatile bool listener_stop;
		extern volatile int running_threads;
		extern pfc::string8 listener_info;
		extern void set_active(bool activate);
		extern bool is_active();
	}
}

class config_main {
public:
	pfc::string8	ip;
	t_size			port;
	bool			startserver;
	bool			albumart_embedded_retrieve;	/* dump completely in future */
	bool			retrieve_playlist;			/* dump completely in future */
	bool			hide_nonplayables;
	bool			log_access;
	pfc::string8	server_root;
	pfc::string8	control_ip;
	bool			control_credentials;
	pfc::string8	control_credentials_username;
	pfc::string8	control_credentials_password;
	pfc::string8	restrict_to_path;
	bool			allow_commandline;			/* dump completely in future */
	bool			stop_after_queue_enable;
	bool			gzip_enable;
	pfc::string8	extra_formats;
	pfc::string8	ignored_formats;
	pfc::string8	allowed_protocols;

	config_main() { reset(); }
	config_main(const config_main &cfg) { copy(cfg); }
	void get_data_raw(stream_writer * p_stream,abort_callback & p_abort);
	void set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort);
	config_main &operator = (const config_main &cfg) { copy(cfg); return *this; }
	bool operator == (const config_main &c);
	void copy(const config_main &cfg);
	void reset();
};

class config_misc {
public:
	pfc::string8	last_browse_dir;
	pfc::string8	query_playlist_name;
	bool			stop_after_queue;

	config_misc() { reset(); }
	void get_data_raw(stream_writer * p_stream,abort_callback & p_abort);
	void set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort);
	void reset();
};

class config_query {
public:
	pfc::string8	sortpattern;
	pfc::string8	step1;
	pfc::string8	step2;
	pfc::string8	step3;
	bool			sendtodedicated;

	config_query() { reset(); }
	config_query(const config_query &cfg) {	copy(cfg); }
	void get_data_raw(stream_writer * p_stream,abort_callback & p_abort);
	void set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort);
	config_query &operator = (const config_query &cfg) { copy(cfg); return *this; }
	bool operator == (const config_query &c);
	void copy(const config_query &g_c);
	void reset();
};

class config: public cfg_var {
	virtual void get_data_raw(stream_writer* p_stream, abort_callback& p_abort);
	virtual void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort);

public:
	config_main main;
	config_query query;
	config_misc misc;

	pfc::list_t<pfc::string8> restrict_to_path_list;

	config(const GUID & p_guid) : cfg_var(p_guid) { }
};

extern config cfg;

class art_instance {
public:
	pfc::string name;					// art name
	pfc::string source;					// art source expression
	pfc::string url_art_not_found;		// image url for art not found
	pfc::string url_art_not_available;	// image url for art not available
	GUID embedded_art_type;				// namespace album_art_ids

	art_instance(pfc::string name, GUID album_art_type) : name(name), embedded_art_type(album_art_type) {};
	art_instance() : embedded_art_type(album_art_ids::cover_front) {};
};

class art_container {
public:
	pfc::array_t<art_instance> instances;

	art_container() {
		this->instances.set_size(HTTPC_ART_COUNT);
		this->instances[0] = art_instance(HTTPC_ART_ALBUMART, album_art_ids::cover_front);
		this->instances[1] = art_instance(HTTPC_ART_DISCART, album_art_ids::disc);
	}
};

struct httpc_template {
	pfc::string8 root;
	pfc::string8 url;
	pfc::string8 local_path;
	pfc::string8 playlist_row;
	pfc::array_t<pfc::string8> helpers;
	art_container art;
	t_size playlist_items_per_page;
	t_size playlist_items_per_page_max;
	t_size albumart_limit_size{};
	bool albumart_prefer_embedded{};

	foobar2000_io::t_filetimestamp timestamp{};
	httpc_template() :
		playlist_items_per_page(HTTPC_PLAYLIST_ITEMS_PER_PAGE_DEFAULT),
		playlist_items_per_page_max(HTTPC_PLAYLIST_ITEMS_PER_PAGE_MAX_DEFAULT)
	{
		this->helpers.set_size(HTTPC_HELPER_COUNT);
	};
};

class template_config{
	pfc::list_t<httpc_template> cfg;
	httpc_template cfg_empty;
	t_size cfg_id;
	bool changed{};

public:
	template_config() { /* cfg_id = pfc::infinite_size;*/ cfg += cfg_empty; cfg_id = 0; }

	bool checkrange(t_size c_id) {	if (c_id != pfc::infinite_size  && c_id < cfg.get_count()) return true; else return false; }
	bool checkrange() {	if (cfg_id != pfc::infinite_size  && cfg_id < cfg.get_count()) return true; else return false; }
	bool choose(t_size new_cfg_id) { if (checkrange(new_cfg_id)) { if (cfg_id != new_cfg_id) changed = true; cfg_id = new_cfg_id; return true; } else return false; }
	t_size get_count() { return cfg.get_count(); }
	const t_size get_id() { return cfg_id; }
	void remove(t_size c_id) { if (checkrange(c_id)) { cfg.remove_by_idx(c_id); cfg_id = c_id == 0? c_id : c_id - 1; changed = true; }	}
	void set_size(t_size new_size) { cfg.set_size(new_size); changed = true; }
	bool is_changed() { if (changed) { changed = false; return true; } else return false; }
	t_size find(pfc::string8 &name);
	bool loadtemplate(pfc::string_base &path, pfc::string8 &tpl_name);
	httpc_template &get() {/* if (checkrange(cfg_id))*/ return cfg[cfg_id];/* else return cfg_empty;*/ }
	httpc_template &get(t_size c_id) { /*if (checkrange(c_id)) */return cfg[c_id]; /*else return cfg_empty;*/ }
};

extern template_config tcfg;
#endif /*__FOO_CONFIG_*/