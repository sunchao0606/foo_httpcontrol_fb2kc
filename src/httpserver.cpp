#include "stdafx.h"
#include <jnetlib/util.h>
#include <zlib/zlib.h>
#include "ui.h"
#include "browsefiles.h"
#include "httpserver.h"
#include "httpserver_strings.h"

mimetypes mime;

namespace httpc {
	namespace control {
		pfc::string8 command_result;
	}
}

void foo_httpserv::log(pfc::string_base &cmd, pfc::string_base &param1, pfc::string_base &param2, pfc::string_base &param3, char *remotehost, pfc::string_base &request_url)
{
	pfc::string8 bufs;

	char buf[32] = { };
	GetDateFormatA(NULL, 0, NULL, "[ dd.MM ", buf,31);
	bufs << buf;
	GetTimeFormatA(NULL, 0, NULL, "HH:mm:ss ] ", buf,31);
	bufs << buf << remotehost << " \t" << request_url;

	if (cmd.get_length()) bufs << "?cmd=" << cmd;
	if (param1.get_length()) bufs << "&param1=" << param1;
	if (param2.get_length()) bufs << "&param2=" << param2;
	if (param3.get_length()) bufs << "&param3=" << param3;

	char *ua = getheader("User-Agent");
	if (ua)
	{
		bufs << "  \t" << ua;
		free(ua);
	}

	foo_info(bufs);
}

size_t foo_httpserv::send_data(char *buf, size_t buf_len)
{
	size_t result_len = buf_len;
	char *result_ptr = buf;

	timeval SendTimeout;
	SendTimeout.tv_sec = 0;
	SendTimeout.tv_usec = 250000; // 250 ms
	fd_set fds;

	size_t run_result = 1;
	size_t cansend;
	size_t select_res = 0;

	while (((result_len > 0 || bytes_inqueue())
		&& run_result > 0 && run_result != 4) && !httpc::control::listener_stop && select_res != SOCKET_ERROR)
	{
		cansend = bytes_cansend();

		if (cansend > result_len)
			cansend = result_len;

		if (cansend > 0)
		{
			write_bytes(result_ptr,(int)cansend);
			result_ptr += cansend;
			result_len -= cansend;
    	}
		else
			Sleep(1);

		run_result = run();

		FD_ZERO(&fds);
		FD_SET(get_con()->m_socket, &fds);
		select_res = select(0, NULL, &fds, NULL, &SendTimeout);
	}

	if (select_res == SOCKET_ERROR)
		return SEND_SOCKET_ERROR;
	else
	if (result_len > 0)
		return SEND_INCOMPLETE;
	else
		return SEND_OK;
}

size_t foo_httpserv::send_data_zlib(char *buf, size_t length)
{
	z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

	unsigned int ret, flush, result_len;

	ret = deflateInit2(&strm, Z_CLEVEL, Z_DEFLATED, 15+16, 8, Z_DEFAULT_STRATEGY);
	if (ret != Z_OK)
		return SEND_ZLIB_ERROR;

	unsigned int buf_len = (unsigned int)length;
	char *buf_ptr = buf;

	size_t send_result;

	do
	{
		strm.avail_in = (unsigned int) (Z_CHUNK > buf_len ? buf_len : Z_CHUNK);
		strm.next_in = (unsigned char *) buf_ptr;

		buf_ptr += strm.avail_in;
		buf_len -= strm.avail_in;

		flush = buf_len > 0? Z_NO_FLUSH : Z_FINISH;

		do {
			strm.avail_out = Z_CHUNK;
			strm.next_out = z_outbuf;

			ret = deflate(&strm, flush);

			if (ret == Z_STREAM_ERROR)	return SEND_ZLIB_ERROR;

			result_len = Z_CHUNK - strm.avail_out;
			unsigned char *result_ptr = z_outbuf;

			send_result = send_data((char *)result_ptr, result_len);

		} while (strm.avail_out == 0 && !httpc::control::listener_stop && send_result == SEND_OK);
	} while ((buf_len > 0) && !httpc::control::listener_stop && send_result == SEND_OK);

	strm.next_in = Z_NULL;
	strm.next_out = Z_NULL;
    (void)deflateEnd(&strm);

	if (send_result != SEND_OK)
		return send_result;
	else
	if (buf_len > 0)
		return SEND_INCOMPLETE;
	else
		return SEND_OK;
}

size_t foo_httpserv::send_file(pfc::string_base &filepath)
{
	size_t send_ret = pfc::infinite_size;

	HANDLE inFile;

	pfc::stringcvt::string_wide_from_utf8 path_w(filepath);
	inFile = CreateFileW(path_w, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (inFile == INVALID_HANDLE_VALUE)
		return SEND_IO_ERROR;
	else
	{
		DWORD bytesread;

		do
		{
			if (!ReadFile(inFile, filebuf, FILE_BUFFER, &bytesread, NULL))
			{
				send_ret = SEND_IO_ERROR;
				break;
			}

			send_ret = send_data(filebuf, bytesread);

			if (bytesread <= 0)
				break;

		} while (bytesread > 0 && send_ret == SEND_OK && !httpc::control::listener_stop);

		CloseHandle(inFile);
	}

	if (send_ret != SEND_OK)
		return send_ret;
	else
		return SEND_OK;
}

size_t foo_httpserv::send_file_zlib(pfc::string_base &filepath)
{
	size_t send_ret = pfc::infinite_size;
	HANDLE inFile;

	pfc::stringcvt::string_wide_from_utf8 path_w(filepath);
	inFile = CreateFileW(path_w, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (inFile == INVALID_HANDLE_VALUE)
		return SEND_IO_ERROR;
	else
	{
		DWORD bytesread;

		z_stream strm;
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;

		int flush, result_len;

		if (deflateInit2(&strm, Z_CLEVEL, Z_DEFLATED, 15+16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
		{
			CloseHandle(inFile);
			return SEND_ZLIB_ERROR;
		}

		do
		{
			if (!ReadFile(inFile, filebuf, FILE_BUFFER, &bytesread, NULL))
			{
				send_ret = SEND_IO_ERROR;
				break;
			}

			strm.avail_in = Z_CHUNK > bytesread ? bytesread : Z_CHUNK;
			strm.next_in = (unsigned char *) filebuf;

			flush = bytesread > 0? Z_NO_FLUSH : Z_FINISH;

			do {
				strm.avail_out = Z_CHUNK;
				strm.next_out = z_outbuf;

				if (deflate(&strm, flush) == Z_STREAM_ERROR)
				{
					CloseHandle(inFile);
					return SEND_ZLIB_ERROR;
				}

				result_len = Z_CHUNK - strm.avail_out;

				send_ret = send_data((char *)z_outbuf, result_len);

			} while (strm.avail_out == 0 && !httpc::control::listener_stop && send_ret == SEND_OK);
		} while (bytesread > 0 && send_ret == SEND_OK && !httpc::control::listener_stop);

		CloseHandle(inFile);
		strm.next_in = Z_NULL;
		strm.next_out = Z_NULL;
	    (void)deflateEnd(&strm);
	}

	if (send_ret != SEND_OK)
		return send_ret;
	else
		return SEND_OK;
}

bool foo_httpserv::is_gzip_encoding_supported()
{
	bool result = false;
	char *accept_encoding = getheader("Accept-Encoding:");
	if (accept_encoding)
	{
		result = strstr(accept_encoding, "gzip") == 0? false : true;
		free(accept_encoding);
	}

	return result;
}

void foo_httpserv::process_request()
{
	pfc::string8 cmd, param1, param2, param3, request_url, fullpathfile, fullpathfileindex, lastwritetime_str, tmp;
	bool query_root{}, gzip_encoding_supported{}, gzip_compressible{}, requested_art{};
	size_t requested_art_id{};
	DWORD content_length = pfc::infinite32;

	timer.start();
	m_err_str.reset();
	t_size r = 0, ret = WSA_WAIT_TIMEOUT;

	r = run();

	if (r == 0)
	{
		HANDLE handle = WSACreateEvent();
		if (WSAEventSelect(m_con->m_socket, handle, FD_READ | FD_CLOSE | FD_OOB) != 0)
			foo_error("WSAEventSelect in process_request failed");

		// sleeping while browser decided do keep stale tcp connection opened
		while (r == 0 && ret != WSA_INVALID_HANDLE && (ret == WSA_WAIT_IO_COMPLETION || ret == WSA_WAIT_TIMEOUT) )
		{
			ret = WSAWaitForMultipleEvents(1, &handle, FALSE, ~1, TRUE);
			WSAResetEvent(handle);
			r = run();
		}

		WSACloseEvent(handle);
	}

	if (r != -1 && r != 0) // got something!
	{
		PFC_INSYNC_WRITE(httpc::cs);
		while (r != -1 && r != 4)
		{
			r = run();

			if (r == 0) // wrong call...
			{
				Sleep(1);
				continue;
			}

			if (r == 2)
			{
				cmd = string8_safe(get_request_parm("cmd"));
				url_decode(string8_safe(get_request_file()), request_url);
				url_decode(string8_safe(get_request_parm("param1")), param1);
				url_decode(string8_safe(get_request_parm("param2")), param2);
				url_decode(string8_safe(get_request_parm("param3")), param3);

				if (param3.get_length())
					if (param3.find_first("..\\") != pfc::infinite_size
						|| param3.find_first("../") != pfc::infinite_size)
						param3.reset(); // defend against level-up

				pfc::list_t<pfc::string8> args;
				pfc::splitStringSimple_toList(args, "/", request_url);
				query_root = args.get_count() > 1 ? false : true;

				// get remote IP address
				char remotehost[16] = {0};
				JNL::addr_to_ipstr(get_con()->get_remote(), remotehost, 15);

				if (cfg.main.log_access)
					log(cmd, param1, param2, param3, remotehost, request_url);

				// remote IP doesn't match the set restriction
				if(cfg.main.control_ip.get_length() && (strcmp(cfg.main.control_ip, "0.0.0.0") != 0) && strcmp(remotehost, cfg.main.control_ip) != 0)
				{
					httpc::ui::generate_html_response(m_err_str, pfc::stringLite(httpc::server::strings::title::c403), pfc::stringLite(httpc::server::strings::body::c403));
					set_reply_string(httpc::server::strings::reply::c403);
					set_reply_header(httpc::server::strings::header::connection_close);
					set_reply_header(httpc::server::strings::header::content_type_text_html);
					send_reply();
					continue;
				}

				// suggest user to authenticate if authentication is set
				if(cfg.main.control_credentials)
				{
					pfc::stringLite auth = string8_safe(getheader("Authorization:"));

					if (auth.get_length() && strcmp(auth, httpc::control_credentials_auth_hash) != 0 || !auth.get_length())
					{
						if (auth.get_length())
							foo_error(pfc::string_formatter() << "AUTH request from " << remotehost << " denied. Auth: " << auth);

						httpc::ui::generate_html_response(m_err_str, pfc::stringLite(httpc::server::strings::title::c403), pfc::stringLite(httpc::server::strings::body::c403));
						set_reply_string(httpc::server::strings::reply::c401);
						set_reply_header(httpc::server::strings::header::connection_close);
						set_reply_header(httpc::server::strings::header::auth_basic);
						set_reply_header(httpc::server::strings::header::content_type_text_html);
						send_reply();
						continue;
					}
				}

				if (args.get_count() >= 1)
				{
					// serve robots.txt
					if (strcmp(request_url, httpc::server::strings::request::robots_txt) == 0)
					{
						m_err_str = pfc::stringLite(httpc::server::strings::body::robots_txt);
						set_reply_string(httpc::server::strings::reply::c200);
						set_reply_header(httpc::server::strings::header::connection_close);
						set_reply_header(httpc::server::strings::header::content_type_text_plain);
						send_reply();
						continue;
					}

					// try loading specific config
					pfc::string8 configpath = pfc::string_formatter() << httpc::srv_home_dir << "\\" << args[0] << "\\config";
					if (!tcfg.loadtemplate(configpath, args[0]))
					{
						foo_error(pfc::string_formatter() << "couldn't load " << configpath);
						httpc::ui::generate_html_response(
							m_err_str,
							pfc::stringLite(httpc::server::strings::title::template_config_error),
							pfc::stringLite(pfc::string_printf(httpc::server::strings::body::template_config_error,	args[0].toString()))
						);
						set_reply_string(httpc::server::strings::reply::c404);
						set_reply_header(httpc::server::strings::header::connection_close);
						set_reply_header(httpc::server::strings::header::content_type_text_html);
						set_reply_header(httpc::server::strings::header::no_cache);
						send_reply();
						continue;
					}
				}
				else
				{
					// serve root (templates list)
					set_reply_string(httpc::server::strings::reply::c200);
					set_reply_header(httpc::server::strings::header::connection_close);
					set_reply_header(httpc::server::strings::header::content_type_text_html);
					set_reply_header(httpc::server::strings::header::no_cache);
					httpc::ui::generate_installed_templates_list(m_err_str);
					send_reply();
					continue;
				}

				// proceeding with user request
				{
					bool request_ok = true;

					fullpathfile = pfc::string_formatter() << httpc::srv_home_dir << request_url;

					if (request_url.get_length() && !query_root)
					{
						for (size_t i = 0; i < HTTPC_ART_COUNT; ++i)
						{
							auto& art = tcfg.get().art.instances[i];
							auto& pb_art = httpc::pb_art.instances[i];

							// requested art?
							if ( strstr(
									request_url,
									pfc::string_formatter() << pfc::string_printf(httpc::server::strings::request::art, art.name.toString())
								) > request_url )
							{
								requested_art = true;
								requested_art_id = i;

								// albumart from file
								if (pb_art.status == httpc::AS_FILE)
								{
									fullpathfile = pb_art.source;
									request_ok = is_fileexists(fullpathfile, content_length, lastwritetime_str);
								}
								else
								// invalid albumart memory
								if (pb_art.status == httpc::AS_MEMORY&& pb_art.embedded_ptr.is_empty())
								{
									fullpathfile = pfc::string_formatter() << tcfg.get().local_path << art.url_art_not_found;
									request_ok = is_fileexists(fullpathfile, content_length, lastwritetime_str);
								}
								else
								// albumart from memory
								if (pb_art.status == httpc::AS_MEMORY && pb_art.embedded_ptr.is_valid())
								{
									request_url.reset();
									is_fileexists(pb_art.embedded_file, content_length, lastwritetime_str);
									content_length = (DWORD) pb_art.embedded_ptr->get_size();
								}
								else
								{
									requested_art = false;
									request_ok = false;
									break;
								}
							}
						}

						if (!requested_art)
						{
							request_ok = false;

							if (fullpathfile.find_first("..\\") == pfc::infinite_size
								&& fullpathfile.find_first("../") == pfc::infinite_size) { // don't do this, dudley :-)
								request_ok = is_fileexists(fullpathfile, content_length, lastwritetime_str);

								if (!request_ok && !fullpathfile.lowerCase().endsWith(".html")) { // ensure fullpathname doesn't already end in ".html"
									fullpathfileindex = pfc::string_formatter() << httpc::srv_home_dir << request_url << ".html";
									request_ok = is_fileexists(fullpathfileindex, content_length, lastwritetime_str);
									if (request_ok) {
										fullpathfile = fullpathfileindex;
										request_url = request_url << ".html";
									}
								}
							}
						}
					}

					if (request_ok)
					{
						set_reply_string(httpc::server::strings::reply::c200);
						set_reply_header(httpc::server::strings::header::connection_close);

						pfc::stringLite content_type = "application/octet-stream";	// default content type
						pfc::stringLite p3_mime = mime.get_content_type(param3, gzip_compressible);

						if ((request_url.get_length() == 1 || query_root) && !p3_mime.get_length())
							content_type = mime.get_content_type(pfc::string8("x.htm"), gzip_compressible);
						else if (requested_art && (httpc::pb_art.instances[requested_art_id].status == httpc::AS_FILE))
							content_type = mime.get_content_type(httpc::pb_art.instances[requested_art_id].source, gzip_compressible);
						else if (p3_mime.get_length())
							content_type = p3_mime;
						else
						{
							p3_mime = mime.get_content_type(request_url, gzip_compressible);

							if (p3_mime.get_length())
								content_type = p3_mime;
						}

						set_reply_header(pfc::string_printf(httpc::server::strings::header::content_type, content_type.toString()));

						gzip_encoding_supported = is_gzip_encoding_supported();

						if (gzip_compressible && gzip_encoding_supported && cfg.main.gzip_enable)
							set_reply_header(httpc::server::strings::header::content_encoding_gzip);

						// returning generated response
						if (!request_url.get_length() || query_root)
						{
							if (!httpc::control::process_command_event)
								httpc::control::process_command_event = CreateEvent(NULL,FALSE,FALSE,NULL);

							ResetEvent(httpc::control::process_command_event);
							httpc::control::process_command(cmd, param1, param2, param3);
							WaitForSingleObject(httpc::control::process_command_event, 15000);

							set_reply_header(httpc::server::strings::header::no_cache);
						}
						else
						// returning requested file
						{
							if (lastwritetime_str.get_length())
								set_reply_header(pfc::string_printf(httpc::server::strings::header::last_modified, lastwritetime_str.toString()));

							if (!(gzip_compressible && gzip_encoding_supported && cfg.main.gzip_enable))
								set_reply_header(pfc::string_printf(httpc::server::strings::header::content_length, content_length));
						}

						send_reply();
					}
					else
					{
						foo_error(pfc::string_formatter() << "couldn't load " << fullpathfile);
						httpc::ui::generate_html_response(
							m_err_str,
							pfc::stringLite(httpc::server::strings::title::c404),
							pfc::string_printf(httpc::server::strings::body::c404, request_url.toString())
						);
						set_reply_string(httpc::server::strings::reply::c404);
						set_reply_header(httpc::server::strings::header::connection_close);
						set_reply_header(httpc::server::strings::header::content_type_text_html);
						set_reply_header(httpc::server::strings::header::no_cache);
						send_reply();
					}
				}
			}
			if (r == 3)
			{
				t_size send_result = pfc_infinite;

				if (m_err_str.get_length()) // sending error msg if available
				{
					send_result = send_data(const_cast<char *>(m_err_str.toString()), m_err_str.get_length());
					m_err_str.reset();
				}
				else // no error msg, proceeding
				{
					pfc::string8_fast_aggressive show = " ";

					// sending albumart
					if ( requested_art
						&& httpc::pb_art.instances[requested_art_id].status == httpc::AS_MEMORY
						&& httpc::pb_art.instances[requested_art_id].embedded_ptr.is_valid() )
						send_result = send_data(
							static_cast<char *>(
								const_cast<void *>(httpc::pb_art.instances[requested_art_id].embedded_ptr->get_ptr())
							), httpc::pb_art.instances[requested_art_id].embedded_ptr->get_size()
						);
					else
					// returning actual file
					if (request_url.get_length() > 1 && !query_root)
						if (gzip_compressible && gzip_encoding_supported && cfg.main.gzip_enable)
							send_result = send_file_zlib(fullpathfile);
						else
							send_result = send_file(fullpathfile);
					else
					{
						// returning generated response
						if (strcmp(cmd, "Parse") == 0)
						{
							httpc::ui::set_buffer_from_string(param1);
							httpc::ui::parse_buffer_controls(show, timer);
						}
						else if (httpc::control::command_result.get_length())
						{
							show = httpc::control::command_result;
							httpc::control::command_result.reset();
						}
						else
						if (strcmp(cmd, "Browse") == 0)
						{
    						// strip file name from path if the user clicked on file
							bool is_file = false;

							bool is_url = httpc::is_protocol_allowed(param1);

							// if no path specified or adding url, use last browse directory
							if (param1.get_length() == 0 || is_url)
								param1 = cfg.misc.last_browse_dir;

							if (!is_url)
								if (!httpc::is_path_allowed(param1)) // allowing to browse only specified dirs (if any)
									param1 = cfg.misc.last_browse_dir;

							if (param1.length() > 3 && !param1.ends_with('\\') && !is_url)
							{
								char *stripped_filename = foo_browsefiles::get_path_parent((char *)param1.operator const char *());

								if (stripped_filename && strlen(stripped_filename) > 2)
								{
									param1 = stripped_filename;
									delete[] stripped_filename;
								}
								else // invalid request, displaying last browse dir
									param1 = cfg.misc.last_browse_dir;

    							is_file = true;
							}

							if (!(strcmp(param3, "NoResponse") == 0))
							{
								httpc::ui::set_buffer_from_file(param3, 2);
								httpc::ui::parse_buffer_browser(param1, show, timer);
							}

							if (param1.get_length() != 0 && !is_file && !is_url)
								cfg.misc.last_browse_dir = param1;
						}
    					else
						{
							if (!(strcmp(param3, "NoResponse") == 0))
							{
								httpc::ui::set_buffer_from_file(param3, 1);
								httpc::ui::parse_buffer_controls(show, timer);
							}
						}

						if (gzip_compressible && gzip_encoding_supported && cfg.main.gzip_enable)
							send_result = send_data_zlib(const_cast<char *>(show.get_ptr()), show.get_length());
						else
							send_result = send_data(const_cast<char *>(show.get_ptr()), show.get_length());
					}
				}

				if (send_result != 0)
					foo_error(pfc::string_formatter() << "send: " << send_result << " on request: " << request_url);

				r = 4;

				close(0);
			}
		}
	}

}
