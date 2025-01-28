#include "stdafx.h"

#include <helpers/atl-misc.h>

template_config tcfg;

// {555F213A-FBF5-44c5-AFD1-7B2307F92674}
static const GUID guid_cfg = { 0x555f213a, 0xfbf5, 0x44c5, { 0xaf, 0xd1, 0x7b, 0x23, 0x7, 0xf9, 0x26, 0x74 } };
config cfg(guid_cfg);

void config_main::copy(const config_main &cfg)
{
	ip = cfg.ip;
	port = cfg.port;
	startserver = cfg.startserver;
	albumart_embedded_retrieve = cfg.albumart_embedded_retrieve;
	retrieve_playlist = cfg.retrieve_playlist;
	hide_nonplayables = cfg.hide_nonplayables;
	log_access = cfg.log_access;
	server_root = cfg.server_root;
	control_ip = cfg.control_ip;
	control_credentials = cfg.control_credentials;
	control_credentials_username = cfg.control_credentials_username;
	control_credentials_password = cfg.control_credentials_password;
	restrict_to_path = cfg.restrict_to_path;
	allow_commandline = cfg.allow_commandline;
	stop_after_queue_enable = cfg.stop_after_queue_enable;
	gzip_enable = cfg.gzip_enable;
	extra_formats = cfg.extra_formats;
	ignored_formats = cfg.ignored_formats;
	allowed_protocols = cfg.allowed_protocols;
}

void config_main::reset()
{
	ip = "0.0.0.0";
	port = 8888;
	startserver = true;
	albumart_embedded_retrieve = false;
	retrieve_playlist = true;
	hide_nonplayables = false;
	log_access = false;
	server_root = "";
	control_ip = "0.0.0.0";
	control_credentials = false;
	control_credentials_username = "";
	control_credentials_password = "";
	restrict_to_path = "";
	allow_commandline = false;
	stop_after_queue_enable = false;
	gzip_enable = false;
	extra_formats = "zip|rar";
	ignored_formats = "";
	allowed_protocols = "http|https|3dydfy|fy+https|fy+http";
}

void config_main::get_data_raw(stream_writer * p_stream,abort_callback & p_abort)
{
	p_stream->write_lendian_t(albumart_embedded_retrieve, p_abort);
	p_stream->write_lendian_t(allow_commandline, p_abort);
	p_stream->write_lendian_t(control_credentials, p_abort);
	p_stream->write_string(control_credentials_password, p_abort);
	p_stream->write_string(control_credentials_username, p_abort);
	p_stream->write_string(control_ip, p_abort);
	p_stream->write_lendian_t(hide_nonplayables, p_abort);
	p_stream->write_string(ip, p_abort);
	p_stream->write_lendian_t(log_access, p_abort);
	p_stream->write_lendian_t(port, p_abort);
	p_stream->write_string(restrict_to_path, p_abort);
	p_stream->write_lendian_t(retrieve_playlist, p_abort);
	p_stream->write_string(server_root, p_abort);
	p_stream->write_lendian_t(startserver, p_abort);
	p_stream->write_lendian_t(stop_after_queue_enable, p_abort);
	p_stream->write_lendian_t(gzip_enable, p_abort);
	p_stream->write_string(extra_formats, p_abort);
	p_stream->write_string(ignored_formats, p_abort);
	p_stream->write_string(allowed_protocols, p_abort);
}

void config_main::set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort)
{
	p_stream->read_lendian_t(albumart_embedded_retrieve, p_abort);
	p_stream->read_lendian_t(allow_commandline, p_abort);
	p_stream->read_lendian_t(control_credentials, p_abort);
	p_stream->read_string(control_credentials_password, p_abort);
	p_stream->read_string(control_credentials_username, p_abort);
	p_stream->read_string(control_ip, p_abort);
	p_stream->read_lendian_t(hide_nonplayables, p_abort);
	p_stream->read_string(ip, p_abort);
	p_stream->read_lendian_t(log_access, p_abort);
	p_stream->read_lendian_t(port, p_abort);
	p_stream->read_string(restrict_to_path, p_abort);
	p_stream->read_lendian_t(retrieve_playlist, p_abort);
	p_stream->read_string(server_root, p_abort);
	p_stream->read_lendian_t(startserver, p_abort);
	p_stream->read_lendian_t(stop_after_queue_enable, p_abort);
	p_stream->read_lendian_t(gzip_enable, p_abort);
	p_stream->read_string(extra_formats, p_abort);
	p_stream->read_string(ignored_formats, p_abort);
	p_stream->read_string(allowed_protocols, p_abort);
}

bool config_main::operator == (const config_main &c)
{
	return ((c.ip == ip)
		&&(c.port == port)
		&&(c.startserver == startserver)
		&&(c.albumart_embedded_retrieve == albumart_embedded_retrieve)
		&&(c.retrieve_playlist == retrieve_playlist)
		&&(c.hide_nonplayables == hide_nonplayables)
		&&(c.log_access == log_access)
		&&(c.server_root == server_root)
		&&(c.control_ip == control_ip)
		&&(c.control_credentials == control_credentials)
		&&(c.control_credentials_username == control_credentials_username)
		&&(c.control_credentials_password == control_credentials_password)
		&&(c.restrict_to_path == restrict_to_path)
		&&(c.allow_commandline == allow_commandline)
		&&(c.stop_after_queue_enable == stop_after_queue_enable)
		&&(c.gzip_enable == gzip_enable)
		&&(c.extra_formats == extra_formats)
		&&(c.ignored_formats == ignored_formats)
		&&(c.allowed_protocols == allowed_protocols));
}

void config_misc::reset()
{
	stop_after_queue = false;
	last_browse_dir = "";
	query_playlist_name = "Query (http)";
}

void config_misc::get_data_raw(stream_writer * p_stream,abort_callback & p_abort)
{
	p_stream->write_lendian_t(stop_after_queue, p_abort);
	p_stream->write_string(last_browse_dir, p_abort);
	p_stream->write_string(query_playlist_name, p_abort);
}

void config_misc::set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort)
{
	p_stream->read_lendian_t(stop_after_queue, p_abort);
	p_stream->read_string(last_browse_dir, p_abort);
	p_stream->read_string(query_playlist_name, p_abort);
}

void config_query::copy(const config_query &cfg)
{
	sortpattern = cfg.sortpattern;
	step1 = cfg.step1;
	step2 = cfg.step2;
	step3 = cfg.step3;
	sendtodedicated = cfg.sendtodedicated;
}

void config_query::reset()
{
	sortpattern = "%date%|%artist%|%album%|%tracknumber%";
	step1 = "%genre%";
	step2 = "%artist%";
	step3 = "%album%";
	sendtodedicated = true;
}

void config_query::get_data_raw(stream_writer * p_stream,abort_callback & p_abort)
{
	p_stream->write_lendian_t(sendtodedicated, p_abort);
	p_stream->write_string(sortpattern, p_abort);
	p_stream->write_string(step1, p_abort);
	p_stream->write_string(step2, p_abort);
	p_stream->write_string(step3, p_abort);
}

void config_query::set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort)
{
	p_stream->read_lendian_t(sendtodedicated, p_abort);
	p_stream->read_string(sortpattern, p_abort);
	p_stream->read_string(step1, p_abort);
	p_stream->read_string(step2, p_abort);
	p_stream->read_string(step3, p_abort);
}

bool config_query::operator == (const config_query &c)
{
	return ((c.sortpattern == sortpattern)
		&& (c.step1 == step1)
		&& (c.step2 == step2)
		&& (c.step3 == step3)
		&& (c.sendtodedicated == sendtodedicated));
}

void config::get_data_raw(stream_writer * p_stream,abort_callback & p_abort)
{
	cfg.main.get_data_raw(p_stream, p_abort);
	cfg.query.get_data_raw(p_stream, p_abort);
	cfg.misc.get_data_raw(p_stream, p_abort);
}


void config::set_data_raw(stream_reader * p_stream, t_size p_sizehint,abort_callback & p_abort)
{
	try
	{
		cfg.main.set_data_raw(p_stream, p_sizehint, p_abort);
		cfg.query.set_data_raw(p_stream, p_sizehint, p_abort);
		cfg.misc.set_data_raw(p_stream, p_sizehint, p_abort);
	}
	catch (...)
	{
		throw;
	}
}

t_size template_config::find(pfc::string8 &name)
{
	if (name.get_length())
	{
		t_size l = cfg.get_count();
		for (t_size i = 0; i < l; ++i)
			if (pfc::stringCompareCaseInsensitive(name, get(i).root) == 0)
				return i;
	}

	return pfc::infinite_size;
}

bool template_config::loadtemplate(pfc::string_base &path, pfc::string8 &tpl_name)
{
	if (tpl_name.get_length() == 0)
		return false;

	foobar2000_io::t_filetimestamp timestamp;

	try
	{
		foobar2000_io::t_filestats filestats;
		foobar2000_io::abort_callback_dummy abort_c;
		bool is_writeable;

		foobar2000_io::filesystem::g_get_stats(path, filestats, is_writeable, abort_c);

		timestamp = filestats.m_timestamp;
	}
	catch (...)
	{
		return false;
	}

	t_size n = find(tpl_name);

	if (n != pfc::infinite_size && (timestamp == get(n).timestamp)) // template already loaded and config timestamp didn't change
	{
		choose(n);
		return true;
	}

	// template is not loaded or config timestamp changed;

	pfc::string8 tmp;
	pfc::stringcvt::string_wide_from_utf8 path_w(path);

	if (n == pfc::infinite_size )
	{
		set_size(cfg.get_count() + 1);
		n = cfg.get_count() - 1;
	}

	choose(n);

	get().timestamp = timestamp;
	get().root = tpl_name;
	read_ini_key("url", get().url, path_w);
	read_ini_key("playlist_row", get().playlist_row, path_w);

	for (size_t i = 0; i < HTTPC_HELPER_COUNT; ++i)
		read_ini_key(pfc::string_printf("helper%i", i+1), get().helpers[i], path_w);

	for (auto &art : get().art.instances)
	{
		read_ini_key(art.name, art.source, path_w);

		read_ini_key(pfc::string_formatter(art.name) << "_not_found", art.url_art_not_found, path_w);
		if (!art.url_art_not_found.startsWith('/')) art.url_art_not_found.insert_chars(0, "/");

		read_ini_key(pfc::string_formatter(art.name) << "_not_available", art.url_art_not_available, path_w);
		if (!art.url_art_not_available.startsWith('/')) art.url_art_not_available.insert_chars(0, "/");
	}

	get().playlist_items_per_page_max = HTTPC_PLAYLIST_ITEMS_PER_PAGE_MAX_DEFAULT;
	if (read_ini_key("playlist_items_per_page_max", tmp, path_w))
		get().playlist_items_per_page_max = atoi(tmp) == 0 ? pfc::infinite_size : atoi(tmp);

	get().playlist_items_per_page = HTTPC_PLAYLIST_ITEMS_PER_PAGE_DEFAULT;
	if (read_ini_key("playlist_items_per_page", tmp, path_w))
		get().playlist_items_per_page = strtoul(tmp, NULL, 10) > get().playlist_items_per_page_max ? get().playlist_items_per_page_max : strtoul(tmp, NULL, 10);

	read_ini_key("albumart_limit_size", tmp, path_w);
	get().albumart_limit_size = atoi(tmp);

	read_ini_key("albumart_prefer_embedded", tmp, path_w);
	get().albumart_prefer_embedded = atoi(tmp) == 1? true : false;

	changed = true;

	httpc::choose_srv_home_dir();

	return true;
}