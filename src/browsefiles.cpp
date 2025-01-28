#include "stdafx.h"

#include "browsefiles.h"


#define MAX_NET_RESOURCES (1024)


void foo_browsefiles::buildpath_list(pfc::string path, pfc::list_t<path_entry_data>& res)
{
	auto path_len = path.get_length();

	if (path_len > 2)
	{
		pfc::string_simple dir_path, dir_path_urlencoded;
		pfc::string_simple dir_name;
		t_size start = 0;

		res.add_item(path_entry_data("%20", "Root:\\"));

		if (path.startsWith("\\\\"))
		{
			start += 2;
			res.add_item(path_entry_data("Network:\\", "Network:\\"));
		}

		for (auto i = start; i < path_len; ++i)
		{
			while (i < path_len && path[i] != '\\')
			{
				dir_name.add_byte(path[i]);
				++i;
			}

			dir_name.add_byte('\\');
			dir_path = path.subString(0, i) << "\\";

			if (httpc::is_path_allowed(dir_path))
			{
				pfc::urlEncode(dir_path_urlencoded, dir_path);
				res.add_item(path_entry_data(dir_path_urlencoded, dir_name));
			}
			else
				res.add_item(path_entry_data("%20", dir_name));

			dir_path.reset();
			dir_name.reset();
		}
	}
}

char *foo_browsefiles::get_path_parent(char *path)
{
	char *result;

	if ( strlen(path) == 0
		|| (strlen(path) <= 3)					// root - nowhere to get upwards to or
		|| strstr(path, "Network") == path )	// some drive root - getting up to root
		return NULL;

	if ( (strlen(path) > 3)	// some path - getting up one level
		)
	{
		char *end = (path + strlen(path) - 2); //skip trailing slash
		while (end >= path && *end-- != '\\');
		end += 2;

		result = new char [strlen(path)];
		memset (result, 0, strlen(path));
		char *result_ptr = result;

		while (path != end)
			*(result_ptr++) = *(path++);

		if (strlen(result) == 2 && result[0] == '\\')
		{
			delete[] result;
			result = new char[16];
			strcpy_s(result, 15, "Network:\\");
		}

		return result;
	}

	return NULL;
}


foo_browsefiles::ENTRY_TYPE foo_browsefiles::get_path_type(char* path)
{
	if (path != NULL) {
		if (strlen(path) < 3)
			return ET_ROOT;

		if (path[strlen(path) - 1] != '\\')
			return ET_FILE;

		if (strcmp(path, "Network:\\") == 0)
			return ET_NETWORK;

		int slashes_count = 0;
		char* path_ptr = path;
		for (unsigned int i = 0; i < strlen(path); ++i)
			if (*path_ptr++ == '\\')
				++slashes_count;

		if ((strstr(path, "\\") == path)
			&& (slashes_count == 3))
			return ET_NETWORK_PC;

		if (path[strlen(path) - 1] == '\\')
			return ET_DIR;
	}
	return ET_UNKNOWN;
}

void foo_browsefiles::scan_network(ENTRY_TYPE searchfor, char *path, NETRESOURCE *pNr)
{
    HANDLE hEnum;
    DWORD dwRes;

	dwRes = WNetOpenEnum( RESOURCE_GLOBALNET, RESOURCETYPE_DISK, 0, pNr, &hEnum);

    if(dwRes!=NO_ERROR)
        return;

	pfc::array_t<NETRESOURCE> NetResource;
	NetResource.set_size(MAX_NET_RESOURCES);
	DWORD dwCount = 0xFFFFFFFF, dwSize = sizeof(NETRESOURCE)*MAX_NET_RESOURCES;

	dwRes = WNetEnumResource(hEnum, &dwCount, (LPVOID*)NetResource.get_ptr(), &dwSize);

    if(dwRes!=NO_ERROR)
	{
		return;
	}

	DWORD dw;
	for(dw=0; dw < dwCount; dw++)
	{
		pfc::string8 remoteName;
		pfc::stringcvt::string_utf8_from_wide buf(NetResource[dw].lpRemoteName);
		remoteName <<  buf << "\\";

		if (   (searchfor == ET_NETWORK_PC)
			&& (NetResource[dw].dwUsage & RESOURCEUSAGE_CONTAINER)
			&& (NetResource[dw].dwDisplayType == RESOURCEDISPLAYTYPE_SERVER) )
		{
			entries.add_item(entry_data(ET_NETWORK_PC, remoteName, "", 0, "", "", ""));
		}

		if (   (searchfor == ET_NETWORK_PC_SHARE &&
			(strstr(remoteName, path) == remoteName)
			)
			&& (NetResource[dw].dwUsage & RESOURCEUSAGE_CONNECTABLE)
			&& (NetResource[dw].dwDisplayType == RESOURCEDISPLAYTYPE_SHARE
			&& NetResource[dw].dwType == RESOURCETYPE_DISK) )
		{
			entries.add_item(entry_data(ET_NETWORK_PC_SHARE, remoteName, "", 0, "", "", ""));
		}

		if(NetResource[dw].dwUsage & RESOURCEUSAGE_CONTAINER)
			scan_network(searchfor, path, &NetResource[dw]);
    }

	WNetCloseEnum(hEnum);
}

bool foo_browsefiles::browse(pfc::string8 path)
{
	entries.remove_all();

	ENTRY_TYPE e_t = get_path_type(const_cast<char *>(path.toString()));

	if (e_t == ET_ROOT)
	{
		if (cfg.restrict_to_path_list.get_count() > 0)
		{
			for (size_t i = 0; i < cfg.restrict_to_path_list.get_count(); ++i)
				entries.add_item(entry_data(ET_DIR, cfg.restrict_to_path_list[i], "", 0, "", "", ""));
		}
		else
		{
			DWORD ld = GetLogicalDrives();

			char drive[] = "A\0";

			for (int i = 0; i <= 26; ++i,ld = ld >> 1, ++drive[0])
				if ((ld & 1) == 1)
				{
					pfc::string8 path;
					path << drive << ":\\";
					pfc::string8 comment;
					UINT d_type = GetDriveTypeA(path.toString());
					switch (d_type)
					{
						case DRIVE_UNKNOWN: comment = "Unknown drive type"; break;
						case DRIVE_NO_ROOT_DIR: comment = "No root directory"; break;
						case DRIVE_REMOVABLE: comment = "Removable"; break;
						case DRIVE_FIXED:  comment = "Fixed"; break;
						case DRIVE_REMOTE: comment = "Remote (network)"; break;
						case DRIVE_CDROM: comment = "CD/DVD"; break;
						case DRIVE_RAMDISK: comment = "RAM"; break;
					}

					char fsname[64] = {0};
					char volname[64] = {0};
					GetVolumeInformationA(path.operator const char *(),
					volname,
					64,
					NULL,
					NULL,
					NULL,
					fsname,
					64
					);

					entries.add_item(entry_data(ET_DIR, path, "", 0, fsname, volname, comment));
				}

			entries.add_item(entry_data(ET_NETWORK, "Network:\\", "", 0, "", "", "Microsoft LAN"));
		}
	}
	else if (e_t == ET_DIR || e_t == ET_NETWORK_PC_SHARE)
	{
		HANDLE hFind;
		LPWIN32_FIND_DATAW findFileData;
		findFileData = new WIN32_FIND_DATAW;

		pfc::string8 mask;
		mask << path << "*";

		pfc::stringcvt::string_wide_from_utf8 mask_w(mask);

		hFind = FindFirstFileW(mask_w, findFileData);

		if (hFind != INVALID_HANDLE_VALUE)
			do
			if (! ( (wcsstr(findFileData->cFileName, TEXT("..")) == findFileData->cFileName || wcsstr(findFileData->cFileName, TEXT(".")) == findFileData->cFileName ) && ( (findFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY))
				)
			{
				entry_data entry;

				pfc::stringcvt::string_utf8_from_wide e_path (findFileData->cFileName);
				entry.path << path << e_path;

				if ( (findFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
					entry.type = ET_DIR;
				else
					entry.type = ET_FILE;

				// format entry modification time
				format_time(&(findFileData->ftLastWriteTime), entry.time, 0);

				// format entry file name
				entry.filename = pfc::string_filename_ext(entry.path);

				// format entry size
				entry.size = ((t_uint64)findFileData->nFileSizeHigh << 8) | findFileData->nFileSizeLow;
				if (entry.type == ET_FILE)
					entry.size_str = pfc::format_file_size_short((DWORD)entry.size, nullptr);

				if (entry.type == ET_DIR)
					entry.path << "\\";

				entries.add_item(entry);
			} while (FindNextFileW(hFind, findFileData) != NULL);
		FindClose(hFind);
		delete findFileData;
		entries.sort_t(sortfunc_natural);
	}
	else if (e_t == ET_NETWORK
		  || e_t == ET_NETWORK_PC)
	{
		if (e_t == ET_NETWORK)
			e_t = ET_NETWORK_PC;
		else
		if (e_t == ET_NETWORK_PC)
			e_t = ET_NETWORK_PC_SHARE;

		scan_network(e_t, const_cast<char *>(path.toString()), NULL);
    }

	return true;
}