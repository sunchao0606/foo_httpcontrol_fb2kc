#include "stdafx.h"
#include "helpers_xxx.h"

pfc::list_t<t_size> find_playlists(const char * p_name, t_size p_name_length)
{
	const auto plm = playlist_manager::get();
	t_size n, m = plm->get_playlist_count();
	pfc::list_t<t_size> arr;
	pfc::string_formatter temp;
	for (n = 0; n < m; n++) {
		if (!plm->playlist_get_name(n, temp)) break;
		if (stricmp_utf8_ex(temp, temp.length(), p_name, p_name_length) == 0) arr.add_item(n);
	}
	return arr;
}