#include "stdafx.h"
#include <jnetlib/webserver.h>
#include <foobar2000/helpers/filetimetools.h>
#include "utils.h"

void foo_info(const char *msg)
{
	if (cfg.main.log_access)
		console::formatter() << VER_PRODUCTNAME_STR << ": " << msg;
};

void foo_error(const char *msg)
{
	console::formatter() << VER_PRODUCTNAME_STR << " error: " << msg;
};

void str_to_bitarray(pfc::string_base &str, bit_array_bittable &arr) // helper string to bit_array
{
	pfc::string8 number;

	t_size size = 0;

	t_size l = str.length();
	for (t_size i = 0; i < l; ++i)
	{
		if (isdigit(str[i]))
			number.add_char(str[i]);
		else
			if (number.get_length() > 0)
			{
				unsigned int item = atoi(number.get_ptr());

				if (item + 1 > size)
				{
					size = item + 1;
					arr.resize(size);
				}

				arr.set(item, true);

				number.reset();
			}
	}

	if (number.get_length() > 0)
	{
		t_size item = atoi(number.get_ptr());

		if (item + 1 > size)
		{
			arr.resize(item + 1);
			size = item + 1;
		}

		arr.set(item, true);
	}
};

void str_to_list(pfc::string_base &str, pfc::list_t<t_size> &list)
{
	pfc::string8 number;

	list.remove_all();

	t_size l = str.length();

	for (t_size i = 0; i < l; ++i)
	{
		if (isdigit(str[i]))
			number.add_char(str[i]);
		else
			if (number.get_length() > 0)
			{
				list.add_item(atoi(number.get_ptr()));
				number.reset();
			}
	}

	if (number.get_length() > 0)
		list.add_item(atoi(number.get_ptr()));
};

void url_decode(pfc::string_base &in, pfc::string_base &out)
{
	t_size maxlen = in.length();;
	char *buf = new char[maxlen + 2];
	ZeroMemory(buf, maxlen + 2);
	WebServerBaseClass::url_decode(const_cast<char *>(in.toString()), buf, (int)maxlen+1);
	out = buf;
	delete[] buf;
}

pfc::string8 trim(pfc::string8 &str)
{
	pfc::string8 out = str;

	size_t i = 0;

	while (i < out.length() && out[i] == ' ')
		++i;

	if (i == out.length())
		return pfc::string8("");

	out.remove_chars(0, i);

	i = out.length() - 1;

	while (i > 0 && out[i] == ' ')
		--i;

	if (i < out.length())
		out.truncate(i+1);

	return out;
}

void get_list(pfc::string8 &str, pfc::list_t<pfc::string8> &list, char separator, bool fix_dir)
{
	if (str.length() == 0)
		return;

	size_t i = 0;
	pfc::string8 p_str;

	while (i < str.length())
	{
		if (str[i] == separator)
		{
			p_str = trim(p_str);
			if (_stricmp(p_str.operator const char *(), "") != 0)
			{
				if (fix_dir && p_str[p_str.get_length() - 1] != '\\')
					p_str.add_char('\\');

				if (!p_str.is_empty())
					list.add_item(p_str);
			}
			p_str.reset();
		}
		else
			p_str.add_byte(str[i]);

		++i;
	}

	if (p_str.length() > 0)
	{
		p_str = trim(p_str);
		if (_stricmp(p_str.operator const char *(), "") != 0)
		{
			if (fix_dir && p_str[p_str.get_length() - 1] != '\\')
				p_str.add_char('\\');
			if (!p_str.is_empty())
				list.add_item(p_str);
		}
	}
}

long calcCRC(const char *data, t_size data_len)
{
	long crc;
	long q;
	byte c;
	crc = 0;
	for (size_t i=0; i< data_len; i++)
	{
			c = data[i];
			q = (crc ^ c) & 0x0f;
			crc = (crc >> 4) ^ (q * 0x1081);
			q = (crc ^ (c >> 4)) & 0xf;
			crc = (crc >> 4) ^ (q * 0x1081);
	}
	return crc;
}

pfc::string8 xml_friendly_string(pfc::string_base &str, char p_replace)
{
	pfc::string8 out;
	t_uint32 c;

	size_t l = str.get_length();
	for (size_t i = 0; i < l; ++i)
	{
		c = str[i];

		switch (c) {
			case '&':
				out += "&amp;"; break;
			case '<':
				out += "&lt;"; break;
			case'>':
				out += "&gt;"; break;
			case '"':
				out += "&quot;"; break;
			case '\'':
				out += "&#39;"; break;
			case '\\':
				out += "&#92;"; break;
			case '\r':
			case '\n':
			case '\t':
				out.add_char(p_replace); break;
			default:
				out.add_byte(c);
		}
	}

	return out;
}

pfc::stringLite replace_nontext_chars(pfc::string_base& str, char p_replace)
{
	pfc::stringLite out = str;
	out.replace_nontext_chars(p_replace);
	return out;
}

bool is_fileexists(pfc::string8 &fullpathfile, DWORD &content_length, pfc::string8 &lastwritetime_str)
{
	HANDLE inFile;

	pfc::stringcvt::string_wide_from_utf8 path_w(fullpathfile);
	inFile = CreateFileW(path_w, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (inFile == INVALID_HANDLE_VALUE)
		return false;
	else
	{
		FILETIME lastwritetime;
		content_length = GetFileSize(inFile, &content_length);
		GetFileTime(inFile, NULL, NULL, &lastwritetime);
		format_time(&lastwritetime, lastwritetime_str, 1);
	}

	CloseHandle(inFile);

	return true;
}

void format_time(LPFILETIME filetime, pfc::string8& out, byte format)
{
	static const char* wMonth[] = { "Jan" /* 1 */, "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	static const char* wDayOfWeek[] = { "Sun" /* 0 */, "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	SYSTEMTIME stUTC;

	if (FileTimeToSystemTime(filetime, &stUTC))
		switch (format)
		{
		case 0:
			SYSTEMTIME stLocal;
			SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);
			pfc::string_printf_here(out, "%02d.%02d.%d %02d:%02d",
				stLocal.wDay, stLocal.wMonth, stLocal.wYear, stLocal.wHour, stLocal.wMinute
			);
			break;
		case 1:	// RFC2616 14.29 Last-Modified HTTP-date
			pfc::string_printf_here(out, "%s, %d %s %d %02d:%02d:%02d GMT",
				wDayOfWeek[stUTC.wDayOfWeek], stUTC.wDay, wMonth[stUTC.wMonth - 1], stUTC.wYear, stUTC.wHour, stUTC.wMinute, stUTC.wSecond
			);
			break;
		}
	else
		out.reset();
}

bool is_path_separator_my(unsigned c)
{
	return c=='\\' || c=='/';
}

void remove_trailing_path_separator(pfc::string8 &path)
{
	t_size i = path.get_length();

	while (i > 0 && is_path_separator_my(path[i-1]))
		--i;

	path.truncate(i);
}

pfc::string8 string8_safe(const char *p_string)
{
	if (p_string)
		return p_string;
	else
		return "";
}

bool read_ini_key(const char *key, pfc::string_base &value, const wchar_t *path)
{
	pfc::array_t<wchar_t> buf;
	buf.set_size_fill(HTTPC_INI_READER_BUFFER_SIZE, 0);

	value.reset();

	if (GetPrivateProfileString(_T("foo_httpcontrol"), pfc::stringcvt::string_wide_from_utf8(key),_T(""), buf.get_ptr(), HTTPC_INI_READER_BUFFER_SIZE-1, path))
	{
		value = trim(pfc::string(pfc::stringcvt::string_utf8_from_wide(buf.get_ptr())));
		return true;
	}

	return false;
}

/* foobar2000 SDK / metadb_handle_list.cpp / wchar_t * makeSortString(const char * in) */
wchar_t * utf8_to_wide(const char * in)
{
	wchar_t * out = new wchar_t[pfc::stringcvt::estimate_utf8_to_wide(in) + 1];
	out[0] = ' ';//StrCmpLogicalW bug workaround.
	pfc::stringcvt::convert_utf8_to_wide_unchecked(out + 1, in);
    return out;
}

int compare_natural_utf8(const char *p1, const char *p2)
{
	wchar_t *wp1 = utf8_to_wide(p1);
	wchar_t *wp2 = utf8_to_wide(p2);

	int result = StrCmpLogicalW(wp1, wp2);

	delete[] wp1;
	delete[] wp2;

	return result;
}

t_size query_select_or_create_playlist(bool dedicated = true, const char *playlist_name = HTTPC_QUERY_PLAYLIST_NAME)
{
	const auto plm = playlist_manager::get();
	const auto apm = autoplaylist_manager::get();
	t_size playlist;

	if (dedicated)
	{
		pfc::string idx_playlist_client, idx_playlist_name;
		auto found = false;
		auto playlists_count = plm->get_playlist_count();

		for (auto idx = 0; idx < playlists_count; ++idx) {
			plm->playlist_get_name(idx, idx_playlist_name);

			// Verifying playlist name
			if (idx_playlist_name != playlist_name) continue;

			// Verifying autoplaylist client name
			if (!apm->is_client_present(idx)) continue;

			plm->playlist_lock_query_name(idx, idx_playlist_client);
			if (idx_playlist_client == HTTPC_QUERY_AUTOPLAYLIST_CLIENT_NAME)
			{
				playlist = idx;
				found = true;
				break;
			}
		}

		if (!found)
			playlist = plm->find_or_create_playlist_unlocked(playlist_name);
	}
	else
	{
		playlist = plm->get_active_playlist();

		// Trying to remove autoplaylist client
		if (apm->is_client_present(playlist))
		{
			apm->remove_client(playlist);
			if (apm->is_client_present(playlist))
				// Client remains, creating new playlist instead
				foo_error(
					pfc::string_formatter() << "Cannot remove active playlist autoplaylist client, creating new playlist"
				);
				playlist = plm->find_or_create_playlist_unlocked(playlist_name);

		}
	}

	plm->set_active_playlist(playlist);

	if (apm->is_client_present(playlist))
		apm->remove_client(playlist);

	return playlist;
}

pfc::string get_extension(const char* path)
{
	pfc::string extension = pfc::io::path::getFileExtension(path);

	if (!extension.is_empty())
		// strip leading dot
		extension.remove_chars(0, 1);

	return extension;
}